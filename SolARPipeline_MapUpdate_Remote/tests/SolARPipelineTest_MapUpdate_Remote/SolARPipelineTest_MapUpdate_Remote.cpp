// Copyright (C) 2017-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <iostream>

#include <unistd.h>
#include <cxxopts.hpp>

#include <xpcf/xpcf.h>
#include <xpcf/api/IComponentManager.h>
#include <xpcf/core/helpers.h>
#include <boost/log/core.hpp>

#include "core/Log.h"
#include "api/pipeline/IMapUpdatePipeline.h"
#include "api/storage/IMapManager.h"
#include "api/display/I3DPointsViewer.h"
#include "api/input/devices/IARDevice.h"

using namespace std;
using namespace SolAR;
using namespace SolAR::api;
using namespace SolAR::datastructure;
namespace xpcf=org::bcom::xpcf;

#define INDEX_USE_CAMERA 0

// print help options
void print_help(const cxxopts::Options& options)
{
    cout << options.help({""}) << std::endl;
}

// print error message
void print_error(const string& msg)
{
    cerr << msg << std::endl;
}

int main(int argc, char* argv[])
{
    #if NDEBUG
        boost::log::core::get()->set_logging_enabled(false);
    #endif

    LOG_ADD_LOG_TO_CONSOLE();

    cxxopts::Options option_list("SolARPipelineTest_MapUpdate_Remote",
                                 "SolARPipelineTest_MapUpdate_Remote - The commandline interface to the xpcf grpc client test application.\n");
    option_list.add_options()
            ("h,help", "display this help and exit")
            ("v,version", "display version information and exit")
            ("f,file", "xpcf grpc client configuration file",
             cxxopts::value<string>());

    auto options = option_list.parse(argc, argv);
    if (options.count("help")) {
        print_help(option_list);
        return 0;
    }
    else if (options.count("version"))
    {
        std::cout << "SolARPipelineTest_MapUpdate_Remote version " << MYVERSION << std::endl << std::endl;
        return 0;
    }
    else if (!options.count("file") || options["file"].as<string>().empty()) {
        print_error("missing one of file or database dir argument");
        return -1;
    }

    try {

        LOG_INFO("Get component manager instance");
        SRef<xpcf::IComponentManager> componentManager = xpcf::getComponentManagerInstance();

        string file = options["file"].as<string>();
        LOG_INFO("Load Client Remote Map Update Pipeline configuration file: {}", file);

        if (componentManager->load(file.c_str()) != org::bcom::xpcf::_SUCCESS)
        {
            LOG_INFO("Failed to load Client Remote Map Update Pipeline configuration file: {}", file);
            return -1;
        }

        LOG_INFO("Resolve IMapUpdatePipeline interface");
        SRef<pipeline::IMapUpdatePipeline> mapUpdatePipeline = componentManager->resolve<SolAR::api::pipeline::IMapUpdatePipeline>();

        LOG_INFO("Resolve other components");
        auto gArDevice = componentManager->resolve<input::devices::IARDevice>();
        auto gMapManager1 = componentManager->resolve<storage::IMapManager>("Map1");
        auto gMapManager2 = componentManager->resolve<storage::IMapManager>("Map2");
        auto gViewer3D = componentManager->resolve<display::I3DPointsViewer>();
        LOG_INFO("Client components loaded");

        CameraParameters gCamParams = gArDevice->getParameters(INDEX_USE_CAMERA);

        LOG_INFO("Initialize map update pipeline");

        if (mapUpdatePipeline->init() != FrameworkReturnCode::_SUCCESS)
        {
            LOG_INFO("Cannot init map update pipeline");
            return -1;
        }

        LOG_INFO("Set camera parameters");

        if (mapUpdatePipeline->setCameraParameters(gCamParams) != FrameworkReturnCode::_SUCCESS) {
            LOG_INFO("Cannot set camera parameters for map update pipeline");
            return -1;
        }

        LOG_INFO("Start map update pipeline");

        if (mapUpdatePipeline->start() != FrameworkReturnCode::_SUCCESS) {
            LOG_INFO("Cannot start map update pipeline");
            return -1;
        }

        // Display the initial global map
        SRef<Map> globalMap;
        mapUpdatePipeline->getMapRequest(globalMap);
        std::vector<SRef<Keyframe>> globalKeyframes;
        std::vector<SRef<CloudPoint>> globalPointCloud;
        globalMap->getConstKeyframeCollection()->getAllKeyframes(globalKeyframes);
        globalMap->getConstPointCloud()->getAllPoints(globalPointCloud);
        std::vector<Transform3Df> globalKeyframesPoses;
        if (globalPointCloud.size() > 0) {
            for (const auto &it : globalKeyframes)
                globalKeyframesPoses.push_back(it->getPose());

            LOG_INFO("==> Display initial global map");

            gViewer3D->display(globalPointCloud, {}, {}, {}, {}, globalKeyframesPoses);
        }
        else {
            LOG_INFO("Initial global map is empty!");
        }

        std::this_thread::sleep_for(std::chrono::seconds(3));

        LOG_INFO("Load 1st map");

        if (gMapManager1->loadFromFile() != FrameworkReturnCode::_SUCCESS) {
            LOG_INFO("Cannot load local map 1");
            return -1;
        }

        LOG_INFO("Send map request for map 1");

        SRef<Map> map;
        gMapManager1->getMap(map);
        LOG_INFO("Nb points: {}", map->getConstPointCloud()->getNbPoints());
        mapUpdatePipeline->mapUpdateRequest(map);
        std::this_thread::sleep_for(std::chrono::seconds(10));
/*
        // Display the intermediate global map
        mapUpdatePipeline->getMapRequest(globalMap);
        globalMap->getConstKeyframeCollection()->getAllKeyframes(globalKeyframes);
        globalMap->getConstPointCloud()->getAllPoints(globalPointCloud);
        if (globalPointCloud.size() > 0) {
            globalKeyframesPoses.clear();
            for (const auto &it : globalKeyframes)
                globalKeyframesPoses.push_back(it->getPose());

            LOG_INFO("==> Display intermediate global map (after Map1 processing)");

            gViewer3D->display(globalPointCloud, {}, {}, {}, {}, globalKeyframesPoses);
        }
        else {
            LOG_INFO("Intermediate global map is empty!");
        }
*/

        LOG_INFO("Load 2nd map");

        if (gMapManager2->loadFromFile() != FrameworkReturnCode::_SUCCESS) {
            LOG_INFO("Cannot load local map 2");
            return -1;
        }

        LOG_INFO("Send map request for map 2");

        gMapManager2->getMap(map);
        LOG_INFO("Nb points: {}", map->getConstPointCloud()->getNbPoints());
        mapUpdatePipeline->mapUpdateRequest(map);
        std::this_thread::sleep_for(std::chrono::seconds(10));
/*
        // Display the final global map
        mapUpdatePipeline->getMapRequest(globalMap);
        globalMap->getConstKeyframeCollection()->getAllKeyframes(globalKeyframes);
        globalMap->getConstPointCloud()->getAllPoints(globalPointCloud);
        if (globalPointCloud.size() > 0) {
            globalKeyframesPoses.clear();
            for (const auto &it : globalKeyframes)
                globalKeyframesPoses.push_back(it->getPose());

            LOG_INFO("==> Display final global map (after Map2 processing)");

            while (true) {
                if (gViewer3D->display(globalPointCloud, {}, {}, {}, {}, globalKeyframesPoses) == FrameworkReturnCode::_STOP)
                    break;
            }
        }
        else {
            LOG_INFO("Final global map is empty!");
        }
*/
        LOG_INFO("Stop map update pipeline");

        mapUpdatePipeline->stop();

    }
    catch (xpcf::Exception & e) {
        LOG_INFO("The following exception has been caught: {}", e.what());
        return -1;
    }

    return 0;
}
