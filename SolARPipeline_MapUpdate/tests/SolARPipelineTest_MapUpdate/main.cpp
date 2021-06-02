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

#include <iostream>
#include <chrono>
#include <boost/log/core.hpp>
#include <xpcf/xpcf.h>
#include "core/Log.h"
#include "api/pipeline/IMapUpdatePipeline.h"
#include "api/storage/IMapManager.h"
#include "api/display/I3DPointsViewer.h"
#include "api/input/devices/IARDevice.h"

#define INDEX_USE_CAMERA 0

using namespace SolAR;
using namespace SolAR::api;
using namespace SolAR::datastructure;
namespace xpcf=org::bcom::xpcf;

/* This sample is to test map update pipeline. 
*  The producer loads two prebuilt maps and then it transfers them to the map update pipeline in order to update the global map.
*/

int main(int argc, char ** argv)
{
#if NDEBUG
    boost::log::core::get()->set_logging_enabled(false);
#endif

	LOG_ADD_LOG_TO_CONSOLE();
	try {
		// Load components of producer	
		LOG_INFO("Start creating producer components");
		SRef<xpcf::IComponentManager> producerComponentMgr = xpcf::getComponentManagerInstance();		
		if (producerComponentMgr->load("SolARPipelineTest_MapUpdate_Producer_conf.xml") != xpcf::_SUCCESS) {
			LOG_ERROR("The file SolARPipelineTest_MapUpdate_Producer_conf.xml has an error");
		}
		auto gArDevice = producerComponentMgr->resolve<input::devices::IARDevice>();
		auto gMapManager1 = producerComponentMgr->resolve<storage::IMapManager>("Map1");
		auto gMapManager2 = producerComponentMgr->resolve<storage::IMapManager>("Map2");
		auto gViewer3D = producerComponentMgr->resolve<display::I3DPointsViewer>();
		CameraParameters gCamParams = gArDevice->getParameters(INDEX_USE_CAMERA);
		LOG_INFO("Producer components loaded");
		
		// Load components of the map update processing	
		LOG_INFO("Start creating map update process components");
		SRef<xpcf::IComponentManager> processorComponentMgr = xpcf::getComponentManagerInstance();
		if (processorComponentMgr->load("SolARPipelineTest_MapUpdate_Processing_conf.xml") != xpcf::_SUCCESS) {
			LOG_ERROR("The file SolARPipelineTest_MapUpdate_Processing_conf.xml has an error");
		}
		auto mapUpdatePipeline = processorComponentMgr->resolve<pipeline::IMapUpdatePipeline>();
		LOG_INFO("Map update process components loaded");

		// Init map update pipeline
        if (mapUpdatePipeline->init() != FrameworkReturnCode::_SUCCESS)
		{
			LOG_INFO("Cannot init map update pipeline");
			return 0;
		}
			
		// Set camera parameters
		if (mapUpdatePipeline->setCameraParameters(gCamParams) != FrameworkReturnCode::_SUCCESS) {
			LOG_INFO("Cannot set camera parameters for map update pipeline");
			return 0;
		}

		// Start pipeline
		if (mapUpdatePipeline->start() != FrameworkReturnCode::_SUCCESS) {
			LOG_INFO("Cannot start map update pipeline");
			return 0;
		}

		// Load two maps
		if (gMapManager1->loadFromFile() != FrameworkReturnCode::_SUCCESS) {
			LOG_INFO("Cannot load local map 1");
			return -1;
		}
		if (gMapManager2->loadFromFile() != FrameworkReturnCode::_SUCCESS) {
			LOG_INFO("Cannot load local map 2");
			return -1;
		}

		// Map update request
		SRef<Map> map;
		gMapManager1->getMap(map);
		LOG_INFO("Nb points: {}", map->getConstPointCloud()->getNbPoints());
		mapUpdatePipeline->mapUpdateRequest(map);
		std::this_thread::sleep_for(std::chrono::seconds(10));

		gMapManager2->getMap(map);
		LOG_INFO("Nb points: {}", map->getConstPointCloud()->getNbPoints());
		mapUpdatePipeline->mapUpdateRequest(map);
		std::this_thread::sleep_for(std::chrono::seconds(10));
		
		// Stop pipeline
		mapUpdatePipeline->stop();

		// Display the global map
		auto globalMapManager = processorComponentMgr->resolve<storage::IMapManager>();	
		SRef<Map> globalMap;
		globalMapManager->getMap(globalMap);
		std::vector<SRef<Keyframe>> globalKeyframes;
		std::vector<SRef<CloudPoint>> globalPointCloud;
		globalMap->getConstKeyframeCollection()->getAllKeyframes(globalKeyframes);
		globalMap->getConstPointCloud()->getAllPoints(globalPointCloud);
		std::vector<Transform3Df> globalKeyframesPoses;
		for (const auto &it : globalKeyframes)
			globalKeyframesPoses.push_back(it->getPose());

		while (true) {
			if (gViewer3D->display(globalPointCloud, {}, {}, {}, {}, globalKeyframesPoses) == FrameworkReturnCode::_STOP)
				break;
		}
	}
	catch (xpcf::InjectableNotFoundException e)
	{
		LOG_ERROR("The following exception in relation to a unfound injectable has been catched: {}", e.what());
		return -1;
	}
	catch (xpcf::Exception e)
	{
		LOG_ERROR("The following exception has been catched: {}", e.what());
		return -1;
	}

    return 0;
}
