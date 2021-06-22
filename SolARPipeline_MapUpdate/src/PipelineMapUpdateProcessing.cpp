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
	LOG_DEBUG("PipelineMapUpdateProcessing constructor");
}

PipelineMapUpdateProcessing::~PipelineMapUpdateProcessing() 
{
    LOG_DEBUG("PipelineMapUpdateProcessing destructor");
    delete m_mapUpdateTask;
}

FrameworkReturnCode PipelineMapUpdateProcessing::init()
{
    if (!m_init) {
        LOG_DEBUG("PipelineMapUpdateProcessing init");
        m_mapManager->loadFromFile();
        m_mapManager->getMap(m_globalMap);
        m_init = true;
    }

    m_stopFlag = false;
    m_startedOK = false;

    return FrameworkReturnCode::_SUCCESS;
}

FrameworkReturnCode PipelineMapUpdateProcessing::setCameraParameters(const CameraParameters & cameraParams) {

    LOG_DEBUG("PipelineMapUpdateProcessing setCameraParameters");
	m_cameraParams = cameraParams;
	m_mapOverlapDetector->setCameraParameters(cameraParams.intrinsic, cameraParams.distortion);
	m_mapUpdate->setCameraParameters(cameraParams.intrinsic, cameraParams.distortion);
    return FrameworkReturnCode::_SUCCESS;
}

FrameworkReturnCode PipelineMapUpdateProcessing::start()
{
	if (m_stopFlag)
	{
		LOG_WARNING("Must initialize before starting");
		return FrameworkReturnCode::_ERROR_;
	}
	// create and start map update thread
	auto getMapUpdateThread = [this]() { processMapUpdate(); };
	m_mapUpdateTask = new xpcf::DelegateTask(getMapUpdateThread);
	m_mapUpdateTask->start();
	LOG_INFO("Map update thread started");
	m_startedOK = true;
    return FrameworkReturnCode::_SUCCESS;
}

FrameworkReturnCode PipelineMapUpdateProcessing::stop() 
{
	m_stopFlag = true;	
	if (m_mapUpdateTask != nullptr)
		m_mapUpdateTask->stop();

	if (!m_startedOK)
	{
		LOG_WARNING("Try to stop a pipeline that has not been started");
		return FrameworkReturnCode::_ERROR_;
	}
	LOG_INFO("Map update pipeline has stopped");
    return FrameworkReturnCode::_SUCCESS;
}

FrameworkReturnCode PipelineMapUpdateProcessing::mapUpdateRequest(const SRef<datastructure::Map> map)
{
	if (m_stopFlag || !m_startedOK)
		return FrameworkReturnCode::_ERROR_;
	m_inputMapBuffer.push(map);
	return FrameworkReturnCode::_SUCCESS;
}

FrameworkReturnCode PipelineMapUpdateProcessing::getMapRequest(SRef<SolAR::datastructure::Map> & map) const
{
    LOG_INFO("Get global map from context");

    map = m_globalMap;

    return FrameworkReturnCode::_SUCCESS;
}

void PipelineMapUpdateProcessing::processMapUpdate()
{
	if (m_stopFlag || !m_startedOK || m_inputMapBuffer.empty()) {
		xpcf::DelegateTask::yield();
		return;
	}
	SRef<Map> map;
	m_inputMapBuffer.pop(map);

	if (map == nullptr)
		return;

	if (m_globalMap->getConstPointCloud()->getNbPoints() == 0) {
		LOG_INFO("Initialize global map from scratch");
		m_mapManager->setMap(map);
		m_mapManager->getMap(m_globalMap);
		m_mapManager->saveToFile();
		return;
	}

	const SRef<CoordinateSystem>& localMapCoordinateSystem = map->getConstCoordinateSystem();
	Transform3Df sim3Transform;
	if (localMapCoordinateSystem->isFloating()) {
		std::vector<std::pair<uint32_t, uint32_t>>overlapsIndices;
		LOG_INFO("Try to overlap detection");
		if (m_mapOverlapDetector->detect(m_globalMap, map, sim3Transform, overlapsIndices) == FrameworkReturnCode::_SUCCESS) {
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
	if (m_mapFusion->merge(map, m_globalMap, sim3Transform, nbMatches, error) == FrameworkReturnCode::_ERROR_) {
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
	m_mapUpdate->update(m_globalMap, newKeyframeIds);

	// global bundle adjustment
	m_bundler->setMap(m_globalMap);
	double error_bundle = m_bundler->bundleAdjustment(m_cameraParams.intrinsic, m_cameraParams.distortion);
	LOG_INFO("Error after bundler: {}", error_bundle);

	// pruning
	m_mapManager->pointCloudPruning();
	m_mapManager->keyframePruning();
	m_mapManager->saveToFile();
}

}
}
