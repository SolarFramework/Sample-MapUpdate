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
#include "api/input/devices/IARDevice.h"
#include "api/storage/IMapManager.h"
#include "api/display/I3DPointsViewer.h"

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
//    LOG_SET_DEBUG_LEVEL();

	try {
        SRef<xpcf::IComponentManager> xpcfComponentManager = xpcf::getComponentManagerInstance();
		std::string configxml = std::string("SolARPipelineTest_MapUpdate_conf.xml");
		if (argc == 2)
			configxml = std::string(argv[1]);
		if (xpcfComponentManager->load(configxml.c_str()) != org::bcom::xpcf::_SUCCESS) {
			LOG_ERROR("Failed to load the configuration file {}", configxml.c_str());
			return -1;
		}
		// Load components of the map update processing	
		LOG_INFO("Start creating components");
        auto gMapUpdatePipeline = xpcfComponentManager->resolve<pipeline::IMapUpdatePipeline>();
        std::vector<SRef<storage::IMapManager>> gMapManagers(2);
		gMapManagers[0] = xpcfComponentManager->resolve<storage::IMapManager>("Map1");
		gMapManagers[1] = xpcfComponentManager->resolve<storage::IMapManager>("Map2");
		auto gViewer3D = xpcfComponentManager->resolve<display::I3DPointsViewer>();
		LOG_INFO("All components loaded");

		// Init map update pipeline
        if (gMapUpdatePipeline->init() != FrameworkReturnCode::_SUCCESS)
		{
			LOG_ERROR("Cannot init map update pipeline");
			return -1;
		}
		
		// Start pipeline
		if (gMapUpdatePipeline->start() != FrameworkReturnCode::_SUCCESS) {
			LOG_ERROR("Cannot start map update pipeline");
			return -1;
		}

		// get data for visualization
		auto getDataForVisualization = [](const SRef<Map>& map, std::vector<SRef<CloudPoint>> &pointCloud,
			std::vector<Transform3Df> &keyframesPoses) {
			pointCloud.clear();
			keyframesPoses.clear();
			std::vector<SRef<Keyframe>> keyframes;
			map->getConstKeyframeCollection()->getAllKeyframes(keyframes);
			map->getConstPointCloud()->getAllPoints(pointCloud);
			for (const auto& it : keyframes)
				keyframesPoses.push_back(it->getPose());
		};

		SRef<Map> map;
		SRef<Map> globalMap;
		std::vector<SRef<CloudPoint>> globalPointCloud;
		std::vector<Transform3Df> globalKeyframesPoses;
		for (int i = 0; i < 2; ++i) {
			// Load map
			if (gMapManagers[i]->loadFromFile() != FrameworkReturnCode::_SUCCESS) {
				LOG_ERROR("Cannot load local map {}", i+1);
				return -1;
			}
			// Merge map to global map			
			gMapManagers[i]->getMap(map);
			LOG_INFO("Merge map {} including {} cloudpoints and {} keyframes to global map", i+1,
				map->getConstPointCloud()->getNbPoints(), map->getConstKeyframeCollection()->getNbKeyframes());
			gMapUpdatePipeline->mapUpdateRequest(map);
			std::this_thread::sleep_for(std::chrono::seconds(10));
            // Display the global map after merging map
            globalMap.reset();
            if (gMapUpdatePipeline->getMapRequest(globalMap) == FrameworkReturnCode::_SUCCESS) {
                getDataForVisualization(globalMap, globalPointCloud, globalKeyframesPoses);
                gViewer3D->display(globalPointCloud, {}, {}, {}, {}, globalKeyframesPoses);
            }
		}
		// display final global map		
		while (true) {
			getDataForVisualization(globalMap, globalPointCloud, globalKeyframesPoses);
			if (gViewer3D->display(globalPointCloud, {}, {}, {}, {}, globalKeyframesPoses) == FrameworkReturnCode::_STOP)
				break;
		}
		LOG_INFO("The global map has {} cloudpoints and {} keyframes.", globalPointCloud.size(), globalKeyframesPoses.size());
        // Stop pipeline
		gMapUpdatePipeline->stop();
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
