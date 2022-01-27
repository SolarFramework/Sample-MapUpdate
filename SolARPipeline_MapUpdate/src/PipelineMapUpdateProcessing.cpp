/**
 * @copyright Copyright (c) 2020 All Right Reserved, B-com http://www.b-com.com/
 *
 * This file is subject to the B<>Com License.
 * All other rights reserved.
 *
 * THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 */

#include "PipelineMapUpdateProcessing.h"
#include "core/Log.h"

namespace xpcf  = org::bcom::xpcf;

namespace SolAR {
using namespace datastructure;
namespace PIPELINES {

PipelineMapUpdateProcessing::PipelineMapUpdateProcessing():ConfigurableBase(xpcf::toUUID<PipelineMapUpdateProcessing>())
{    
    declareInterface<api::pipeline::IMapUpdatePipeline>(this);
	declareInjectable<api::storage::IMapManager>(m_mapManager);
	declareInjectable<api::loop::IOverlapDetector>(m_mapOverlapDetector);
	declareInjectable<api::solver::map::IMapFusion>(m_mapFusion);
	declareInjectable<api::solver::map::IMapUpdate>(m_mapUpdate);
	declareInjectable<api::solver::map::IBundler>(m_bundler);
	declareInjectable<api::reloc::IKeyframeRetriever>(m_kfRetriever);
	declareProperty("nbKeyframeSubmap", m_nbKeyframeSubmap);
	LOG_DEBUG("PipelineMapUpdateProcessing constructor");

    // create map update thread
    auto getMapUpdateThread = [this]() { processMapUpdate(); };
    m_mapUpdateTask = new xpcf::DelegateTask(getMapUpdateThread, false);
}

PipelineMapUpdateProcessing::~PipelineMapUpdateProcessing() 
{
    LOG_DEBUG("PipelineMapUpdateProcessing destructor");
    delete m_mapUpdateTask;
}

FrameworkReturnCode PipelineMapUpdateProcessing::init()
{
    LOG_DEBUG("PipelineMapUpdateProcessing init");

    if (!m_init) {
        m_init = true;
    }
    else {
        LOG_DEBUG("Pipeline Map Update already initialized");
    }

    return FrameworkReturnCode::_SUCCESS;
}

FrameworkReturnCode PipelineMapUpdateProcessing::setCameraParameters(const CameraParameters & cameraParams) {

    LOG_DEBUG("PipelineMapUpdateProcessing setCameraParameters");

	m_cameraParams = cameraParams;
	m_mapOverlapDetector->setCameraParameters(cameraParams.intrinsic, cameraParams.distortion);
    m_mapUpdate->setCameraParameters(cameraParams);
    m_setCameraParameters = true;

    return FrameworkReturnCode::_SUCCESS;
}

FrameworkReturnCode PipelineMapUpdateProcessing::start()
{
    LOG_DEBUG("PipelineMapUpdateProcessing start");

    if (!m_init)
	{
		LOG_WARNING("Must initialize before starting");
        return FrameworkReturnCode::_ERROR_;
	}

    if (!m_setCameraParameters)
    {
        LOG_WARNING("Must set camera parameters before starting");
        return FrameworkReturnCode::_ERROR_;
    }

    if (!m_startedOK) {

        // start map update thread
        m_mapUpdateTask->start();

        LOG_INFO("Map update thread started");

        m_startedOK = true;
    }
    else {
        LOG_DEBUG("Pipeline Map Update already started");
    }

    return FrameworkReturnCode::_SUCCESS;
}

FrameworkReturnCode PipelineMapUpdateProcessing::stop() 
{
    LOG_DEBUG("PipelineMapUpdateProcessing stop");

	if (!m_startedOK)
	{
        LOG_DEBUG("Pipeline Map Update already stopped");
    }
    else {
        if (m_mapUpdateTask != nullptr)
            m_mapUpdateTask->stop();

        m_startedOK = false;

        LOG_INFO("Map update pipeline has stopped");
    }

    return FrameworkReturnCode::_SUCCESS;
}

FrameworkReturnCode PipelineMapUpdateProcessing::mapUpdateRequest(const SRef<datastructure::Map> map)
{
    LOG_DEBUG("PipelineMapUpdateProcessing mapUpdateRequest");

    if (!m_startedOK)
    {
        LOG_WARNING("Try to use a pipeline that has not been started");
        return FrameworkReturnCode::_ERROR_;
    }

	m_inputMapBuffer.push(map);

	return FrameworkReturnCode::_SUCCESS;
}

FrameworkReturnCode PipelineMapUpdateProcessing::getMapRequest(SRef<SolAR::datastructure::Map> & map) const
{
    LOG_DEBUG("PipelineMapUpdateProcessing getMapRequest");

	std::unique_lock<std::mutex> lock(m_mutex);

    // Load current map from file
    if (m_mapManager->loadFromFile() != FrameworkReturnCode::_SUCCESS) {
        LOG_DEBUG("No current map saved");

        return FrameworkReturnCode::_ERROR_;
    }

    m_mapManager->getMap(map);

    lock.unlock();

    return FrameworkReturnCode::_SUCCESS;
}

FrameworkReturnCode PipelineMapUpdateProcessing::getSubmapRequest(const SRef<SolAR::datastructure::Frame> frame, SRef<SolAR::datastructure::Map>& map) const
{
	LOG_DEBUG("PipelineMapUpdateProcessing getSubmapRequest");

	std::unique_lock<std::mutex> lock(m_mutex);

	// Load current map from file
	if (m_mapManager->loadFromFile() != FrameworkReturnCode::_SUCCESS) {
		LOG_DEBUG("No current map saved");
		return FrameworkReturnCode::_ERROR_;
	}

	// keyframes retrieval
	std::vector <uint32_t> retKeyframesId;
	if (m_kfRetriever->retrieve(frame, retKeyframesId) == FrameworkReturnCode::_SUCCESS) {
		// get submap
		m_mapManager->getSubmap(retKeyframesId[0], m_nbKeyframeSubmap, map);
		return FrameworkReturnCode::_SUCCESS;
	}
	return FrameworkReturnCode::_ERROR_;
}

void PipelineMapUpdateProcessing::processMapUpdate()
{
    if (!m_startedOK || m_inputMapBuffer.empty()) {
		xpcf::DelegateTask::yield();
		return;
	}

	SRef<Map> map;
	m_inputMapBuffer.pop(map);

	if (map == nullptr)
		return;

	std::unique_lock<std::mutex> lock(m_mutex);

    // Load current map from file
    if (m_mapManager->loadFromFile() == FrameworkReturnCode::_ERROR_) {
		LOG_INFO("Initialize global map from scratch");
		m_mapManager->setMap(map);
		m_mapManager->saveToFile();
		return;
	}

    SRef<datastructure::Map> current_map;
    m_mapManager->getMap(current_map);

	const SRef<CoordinateSystem>& localMapCoordinateSystem = map->getConstCoordinateSystem();
	Transform3Df sim3Transform;
	if (localMapCoordinateSystem->isFloating()) {
		std::vector<std::pair<uint32_t, uint32_t>>overlapsIndices;
		LOG_INFO("Try to overlap detection");
        if (m_mapOverlapDetector->detect(current_map, map, sim3Transform, overlapsIndices) == FrameworkReturnCode::_SUCCESS) {
			LOG_INFO("Number of overlap cloud points: {}", overlapsIndices.size());
			localMapCoordinateSystem->setParentTransform(sim3Transform);
		}
		else {
			LOG_INFO("No overlap detected -> cannot perform map update");
			return;
		}
	}
	else
		sim3Transform = localMapCoordinateSystem->getParentTransform();

	LOG_INFO("Transformation matrix: \n{}", sim3Transform.matrix());
	// map fusion
	uint32_t nbMatches;
	float error;
    if (m_mapFusion->merge(map, current_map, sim3Transform, nbMatches, error) == FrameworkReturnCode::_ERROR_) {
		LOG_INFO("Cannot merge two maps");
		return;
	}
	LOG_INFO("The refined transformation matrix: \n{}", sim3Transform.matrix());
	LOG_INFO("Number of matched cloud points: {}", nbMatches);
	LOG_INFO("Error: {}", error);

	// get new keyframes
	std::vector<SRef<Keyframe>> newKeyframes;
	map->getConstKeyframeCollection()->getAllKeyframes(newKeyframes);
	std::vector<uint32_t> newKeyframeIds;
	for (const auto& itKf : newKeyframes)
		newKeyframeIds.push_back(itKf->getId());
	LOG_INFO("Number of new keyframes: {}", newKeyframeIds.size());

	// Map update
    m_mapUpdate->update(current_map, newKeyframeIds);

    // global bundle adjustment
    m_bundler->setMap(current_map);
    double error_bundle = m_bundler->bundleAdjustment(m_cameraParams.intrinsic, m_cameraParams.distortion);
	LOG_INFO("Error after bundler: {}", error_bundle);

	// pruning
	m_mapManager->pointCloudPruning();
	m_mapManager->keyframePruning();
	m_mapManager->saveToFile();

    lock.unlock();

}

}
}
