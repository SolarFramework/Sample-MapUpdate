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
    if (m_mapUpdateTask == nullptr) {
        auto fnMapUpdateProcessing = [&]() {
            processMapUpdate();
        };

        m_mapUpdateTask = new xpcf::DelegateTask(fnMapUpdateProcessing);
    }
}

PipelineMapUpdateProcessing::~PipelineMapUpdateProcessing() 
{
    LOG_DEBUG("PipelineMapUpdateProcessing destructor");

    if (m_mapUpdateTask != nullptr) {
        m_mapUpdateTask->stop();
        delete m_mapUpdateTask;
    }

    std::unique_lock<std::mutex> lock(m_map_mutex);

    LOG_DEBUG("Remove map data from memory");

    // Unload current map (free memory)
    m_mapManager->setMap(xpcf::utils::make_shared<Map>());
}

FrameworkReturnCode PipelineMapUpdateProcessing::init()
{
    LOG_DEBUG("PipelineMapUpdateProcessing init");

    if (!m_init) {

        std::unique_lock<std::mutex> lock(m_map_mutex);

        // Load current map from file
        if (m_mapManager->loadFromFile() == FrameworkReturnCode::_ERROR_) {
            LOG_INFO("Initialize global map from scratch");
            m_emptyMap = true;
        }
        else
            m_emptyMap = false;

        // start map update thread
        if (m_mapUpdateTask != nullptr)
            m_mapUpdateTask->start();

        m_init = true;
    }
    else {
        LOG_DEBUG("Pipeline Map Update already initialized");
    }

    return FrameworkReturnCode::_SUCCESS;
}

FrameworkReturnCode PipelineMapUpdateProcessing::setCameraParameters(const CameraParameters & cameraParams)
{
    LOG_DEBUG("PipelineMapUpdateProcessing setCameraParameters");
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

    return FrameworkReturnCode::_SUCCESS;
}

FrameworkReturnCode PipelineMapUpdateProcessing::stop() 
{
    LOG_DEBUG("PipelineMapUpdateProcessing stop");

    return FrameworkReturnCode::_SUCCESS;
}

FrameworkReturnCode PipelineMapUpdateProcessing::mapUpdateRequest(const SRef<datastructure::Map> map)
{
    LOG_DEBUG("PipelineMapUpdateProcessing mapUpdateRequest");

    if (!m_init)
    {
        LOG_WARNING("Try to use a pipeline that has not been initialized");
        return FrameworkReturnCode::_ERROR_;
    }

	m_inputMapBuffer.push(map);

	return FrameworkReturnCode::_SUCCESS;
}

FrameworkReturnCode PipelineMapUpdateProcessing::getMapRequest(SRef<SolAR::datastructure::Map> & map) const
{
    LOG_DEBUG("PipelineMapUpdateProcessing getMapRequest");

    if (!m_init)
    {
        LOG_WARNING("Try to use a pipeline that has not been initialized");
        return FrameworkReturnCode::_ERROR_;
    }

    std::unique_lock<std::mutex> lock(m_map_mutex);

    m_mapManager->getMap(map);

    return FrameworkReturnCode::_SUCCESS;
}

FrameworkReturnCode PipelineMapUpdateProcessing::getSubmapRequest(const SRef<SolAR::datastructure::Frame> frame, SRef<SolAR::datastructure::Map>& map) const
{
	LOG_DEBUG("PipelineMapUpdateProcessing getSubmapRequest");

    if (!m_init)
    {
        LOG_WARNING("Try to use a pipeline that has not been initialized");
        return FrameworkReturnCode::_ERROR_;
    }

    // keyframes retrieval
	std::vector <uint32_t> retKeyframesId;

	if (m_kfRetriever->retrieve(frame, retKeyframesId) == FrameworkReturnCode::_SUCCESS) {

        std::unique_lock<std::mutex> lock(m_map_mutex);

        // get submap
		m_mapManager->getSubmap(retKeyframesId[0], m_nbKeyframeSubmap, map);

		return FrameworkReturnCode::_SUCCESS;
	}

	return FrameworkReturnCode::_ERROR_;
}

FrameworkReturnCode PipelineMapUpdateProcessing::resetMap()
{
    LOG_DEBUG("PipelineMapUpdateProcessing resetMap");

    std::unique_lock<std::mutex> lock(m_map_mutex);

    if (m_mapManager->deleteFile() == FrameworkReturnCode::_SUCCESS) {

        // Unload current map (free memory)
        m_mapManager->setMap(xpcf::utils::make_shared<Map>());

        m_emptyMap = true;

        LOG_INFO("Map reset ok");

        return FrameworkReturnCode::_SUCCESS;
    }
    else {
        LOG_WARNING("Map reset failed");

        return FrameworkReturnCode::_ERROR_;
    }
}

void PipelineMapUpdateProcessing::processMapUpdate()
{
    if (!m_init || m_inputMapBuffer.empty()) {
		xpcf::DelegateTask::yield();
		return;
	}

	SRef<Map> map;
	m_inputMapBuffer.pop(map);

	if (map == nullptr)
		return;

    std::unique_lock<std::mutex> lock_process(m_process_mutex);

    std::unique_lock<std::mutex> lock_map(m_map_mutex);

    if (m_emptyMap) {
        LOG_INFO("Initialize global map from scratch");

        m_mapManager->setMap(map);
        m_mapManager->saveToFile();
        m_emptyMap = false;

        return;
    }

    SRef<datastructure::Map> current_map;
    m_mapManager->getMap(current_map);

    lock_map.unlock();

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
        LOG_WARNING("Cannot merge two maps");
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
    double error_bundle = m_bundler->bundleAdjustment();
	LOG_INFO("Error after bundler: {}", error_bundle);
	
	// check error of global BA to discard noisy map
	if (error_bundle > 10) {
        LOG_WARNING("Map update failed");
		return;
	}

    lock_map.lock();

	// pruning
	m_mapManager->pointCloudPruning();
	m_mapManager->keyframePruning();
	m_mapManager->saveToFile();
}

}
}
