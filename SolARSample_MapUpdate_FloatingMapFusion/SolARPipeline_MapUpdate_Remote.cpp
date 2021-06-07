/**
 * @copyright Copyright (c) 2017 B-com http://www.b-com.com/
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <boost/log/core.hpp>
#include "xpcf/xpcf.h"
#include "core/Log.h"
#include "api/input/devices/IARDevice.h"
#include "api/display/I3DPointsViewer.h"
#include "api/solver/map/IBundler.h"
#include "api/storage/IMapManager.h"
#include "api/loop/IOverlapDetector.h"
#include "api/solver/map/IMapFusion.h"

using namespace SolAR;
using namespace SolAR::datastructure;
using namespace SolAR::api;
using namespace SolAR::api::storage;
namespace xpcf  = org::bcom::xpcf;

#define INDEX_USE_CAMERA 0

int main(int argc, char *argv[])
{

#if NDEBUG
    boost::log::core::get()->set_logging_enabled(false);
#endif

    LOG_ADD_LOG_TO_CONSOLE();
	try {
		/* instantiate component manager*/
		/* this is needed in dynamic mode */
		SRef<xpcf::IComponentManager> xpcfComponentManager = xpcf::getComponentManagerInstance();

        std::string configxml = std::string("SolARSample_MapUpdate_FloatingMapFusion_conf.xml");
		if (argc == 2)
			configxml = std::string(argv[1]);

		if (xpcfComponentManager->load(configxml.c_str()) != org::bcom::xpcf::_SUCCESS)
		{
			LOG_ERROR("Failed to load the configuration file {}", configxml.c_str());
			return -1;
		}

		// declare and create components
		LOG_INFO("Start creating components");
		auto arDevice = xpcfComponentManager->resolve<input::devices::IARDevice>();
		auto viewer3D = xpcfComponentManager->resolve<display::I3DPointsViewer>();
		auto globalMapManager = xpcfComponentManager->resolve<storage::IMapManager>("globalMapper");
		auto floatingMapManager = xpcfComponentManager->resolve<storage::IMapManager>("floatingMapper");
		auto fusionMapManager = xpcfComponentManager->resolve<storage::IMapManager>("fusionMapper");
		auto mapOverlapDetector = xpcfComponentManager->resolve<loop::IOverlapDetector>();
		auto mapFusion = xpcfComponentManager->resolve<solver::map::IMapFusion>();
		auto globalBundler = xpcfComponentManager->resolve<api::solver::map::IBundler>();

		// Load camera intrinsics parameters
		CameraParameters camParams = arDevice->getParameters(INDEX_USE_CAMERA);

		if (globalMapManager->loadFromFile() != FrameworkReturnCode::_SUCCESS) {
			LOG_INFO("Cannot load global map");
			return -1;
		}
		if (floatingMapManager->loadFromFile() != FrameworkReturnCode::_SUCCESS) {
			LOG_INFO("Cannot load floating map");
			return -1;
		}

		// get map
		SRef<Map> floatingMap, globalMap;
		floatingMapManager->getMap(floatingMap);
		globalMapManager->getMap(globalMap);
		LOG_INFO("Global map");
		LOG_INFO("Number of point cloud: {}", globalMap->getConstPointCloud()->getNbPoints());
		LOG_INFO("Number of keyframes: {}", globalMap->getConstKeyframeCollection()->getNbKeyframes());
		LOG_INFO("Local map");
		LOG_INFO("Number of point cloud: {}", floatingMap->getConstPointCloud()->getNbPoints());
		LOG_INFO("Number of keyframes: {}", floatingMap->getConstKeyframeCollection()->getNbKeyframes());

		Transform3Df sim3Transform;
		std::vector<std::pair<uint32_t, uint32_t>>overlapsIndices;		
		// detect overlap from global/floating map and extract sim3
		LOG_INFO("Overlap detection: ");
		if (mapOverlapDetector->detect(globalMap, floatingMap, sim3Transform, overlapsIndices) == FrameworkReturnCode::_SUCCESS) {
			LOG_INFO("	->overlap sim3 from {} overlap cloud points: \n{}", overlapsIndices.size(), sim3Transform.matrix());
			// map fusion
			uint32_t nbMatches;
			float error;
			if (mapFusion->merge(floatingMap, globalMap, sim3Transform, nbMatches, error) == FrameworkReturnCode::_ERROR_) {
				LOG_INFO("Cannot merge two maps");
				return 0;
			}
			LOG_INFO("The refined Transformation matrix: \n{}", sim3Transform.matrix());
			LOG_INFO("Number of matched cloud points: {}", nbMatches);
			LOG_INFO("Error: {}", error);

			// global bundle adjustment
			globalBundler->setMap(globalMap);
			double error_bundle  = globalBundler->bundleAdjustment(camParams.intrinsic, camParams.distortion);
			LOG_INFO("Error after bundler: {}", error_bundle);

			// pruning
            globalMapManager->pointCloudPruning();
            globalMapManager->keyframePruning();

			// get keyframes and point cloud to display		
			std::vector<SRef<Keyframe>> globalKeyframes;
			std::vector<SRef<CloudPoint>> globalPointCloud;
			globalMap->getConstKeyframeCollection()->getAllKeyframes(globalKeyframes);
			globalMap->getConstPointCloud()->getAllPoints(globalPointCloud);

			std::vector<Transform3Df> globalKeyframesPoses;
			for (const auto &it : globalKeyframes)
				globalKeyframesPoses.push_back(it->getPose());

			while (true) {
                if (viewer3D->display(globalPointCloud, {}, {}, {}, {}, globalKeyframesPoses) == FrameworkReturnCode::_STOP)
					break;
			}

			// save the fusion map
			fusionMapManager->setMap(globalMap);
			fusionMapManager->saveToFile();
		}
		else {
			LOG_INFO(" no overlaps detected")
				return -1;
		}
	}
	catch (xpcf::Exception e){
		LOG_ERROR("The following exception has been catch : {}", e.what());
		return -1;
	}
    return 0;
}
