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
#include <signal.h>

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

        // Check if log level is defined in environment variable SOLAR_LOG_LEVEL
        char * log_level = getenv("SOLAR_LOG_LEVEL");
        std::string str_log_level = "INFO(default)";

        if (log_level != nullptr) {
            str_log_level = std::string(log_level);

            if (str_log_level == "DEBUG"){
                LOG_SET_DEBUG_LEVEL();
            }
            else if (str_log_level == "CRITICAL"){
                LOG_SET_CRITICAL_LEVEL();
            }
            else if (str_log_level == "ERROR"){
                LOG_SET_ERROR_LEVEL();
            }
            else if (str_log_level == "INFO"){
                LOG_SET_INFO_LEVEL();
            }
            else if (str_log_level == "TRACE"){
                LOG_SET_TRACE_LEVEL();
            }
            else if (str_log_level == "WARNING"){
                LOG_SET_WARNING_LEVEL();
            }
            else {
                LOG_ERROR ("'SOLAR_LOG_LEVEL' environment variable: invalid value");
                LOG_ERROR ("Expected values are: DEBUG, CRITICAL, ERROR, INFO, TRACE or WARNING");
            }

            LOG_DEBUG("Environment variable SOLAR_LOG_LEVEL={}", str_log_level);
        }

        LOG_INFO("Get component manager instance");
        SRef<xpcf::IComponentManager> componentManager = xpcf::getComponentManagerInstance();

        string file = options["file"].as<string>();
        LOG_INFO("Load Client Remote Map Update Pipeline configuration file: {}", file);

        if (componentManager->load(file.c_str()) != org::bcom::xpcf::_SUCCESS)
        {
            LOG_ERROR("Failed to load Client Remote Map Update Pipeline configuration file: {}", file);
            return -1;
        }

        LOG_INFO("Resolve IMapUpdatePipeline interface");
        SRef<pipeline::IMapUpdatePipeline> mapUpdatePipeline = componentManager->resolve<SolAR::api::pipeline::IMapUpdatePipeline>();

        LOG_INFO("Resolve other components");
        auto gARDevice = componentManager->resolve<input::devices::IARDevice>();
        auto gMapManager1 = componentManager->resolve<storage::IMapManager>("Map1");
        auto gMapManager2 = componentManager->resolve<storage::IMapManager>("Map2");
        LOG_INFO("Client components loaded");

        CameraRigParameters camRigParams = gARDevice->getCameraParameters();
        CameraParameters camParams = camRigParams.cameraParams[INDEX_USE_CAMERA];

        LOG_INFO("Initialize map update pipeline");

        if (mapUpdatePipeline->init() != FrameworkReturnCode::_SUCCESS)
        {
            LOG_ERROR("Cannot init map update pipeline");
            return -1;
        }

        LOG_INFO("Set camera parameters");

        if (mapUpdatePipeline->setCameraParameters(camParams) != FrameworkReturnCode::_SUCCESS) {
            LOG_ERROR("Cannot set camera parameters for map update pipeline");
            return -1;
        }

        LOG_INFO("Start map update pipeline");

        if (mapUpdatePipeline->start() != FrameworkReturnCode::_SUCCESS) {
            LOG_ERROR("Cannot start map update pipeline");
            return -1;
        }

        // Display the initial global map

        auto gViewer3D = componentManager->resolve<display::I3DPointsViewer>();

        LOG_INFO("Try to get initial global map from Map Update remote pipeline");

        SRef<Map> globalMap;
        std::vector<SRef<Keyframe>> globalKeyframes;
        std::vector<SRef<CloudPoint>> globalPointCloud;
        std::vector<Transform3Df> globalKeyframesPoses;

        mapUpdatePipeline->getMapRequest(globalMap);
        globalMap->getConstKeyframeCollection()->getAllKeyframes(globalKeyframes);
        globalMap->getConstPointCloud()->getAllPoints(globalPointCloud);

        if (globalPointCloud.size() > 0) {
            for (const auto &it : globalKeyframes)
                globalKeyframesPoses.push_back(it->getPose());

            LOG_INFO("\n==> Display initial global map\n");

            gViewer3D->display(globalPointCloud, {}, {}, {}, {}, globalKeyframesPoses);
        }
        else {
            LOG_INFO("Initial global map is empty!");
        }

        LOG_INFO("Load local map 1");

        if (gMapManager1->loadFromFile() != FrameworkReturnCode::_SUCCESS) {
            LOG_ERROR("Cannot load local map 1");
            return -1;
        }

        LOG_INFO("Send map request for local map 1");

        SRef<Map> map1;
        SRef<Map> globalMap1;
        std::vector<SRef<Keyframe>> globalKeyframes1;
        std::vector<SRef<CloudPoint>> globalPointCloud1;
        std::vector<Transform3Df> globalKeyframesPoses1;

        gMapManager1->getMap(map1);
        LOG_INFO("Nb points: {}", map1->getConstPointCloud()->getNbPoints());
        mapUpdatePipeline->mapUpdateRequest(map1);
        std::this_thread::sleep_for(std::chrono::seconds(20));

        // Display the new global map
        LOG_INFO("Request to get new global map");
        mapUpdatePipeline->getMapRequest(globalMap1);
        globalMap1->getConstKeyframeCollection()->getAllKeyframes(globalKeyframes1);
        globalMap1->getConstPointCloud()->getAllPoints(globalPointCloud1);

        if (globalPointCloud1.size() > 0) {
            globalKeyframesPoses1.clear();
            for (const auto &it : globalKeyframes1)
                globalKeyframesPoses1.push_back(it->getPose());

            LOG_INFO("\n==> Display new global map (after local map 1 processing)\n");

            gViewer3D->display(globalPointCloud1, {}, {}, {}, {}, globalKeyframesPoses1);
        }
        else {
            LOG_INFO("New global map is empty!");
        }

        LOG_INFO("Load local map 2");

        if (gMapManager2->loadFromFile() != FrameworkReturnCode::_SUCCESS) {
            LOG_ERROR("Cannot load local map 2");
            return -1;
        }

        LOG_INFO("Send map request for map 2");

        SRef<Map> map2;
        SRef<Map> globalMap2;
        std::vector<SRef<Keyframe>> globalKeyframes2;
        std::vector<SRef<CloudPoint>> globalPointCloud2;
        std::vector<Transform3Df> globalKeyframesPoses2;

        gMapManager2->getMap(map2);
        LOG_INFO("Nb points: {}", map2->getConstPointCloud()->getNbPoints());
        mapUpdatePipeline->mapUpdateRequest(map2);
        std::this_thread::sleep_for(std::chrono::seconds(20));

        // Display the final global map
        LOG_INFO("Request to get new global map");
        mapUpdatePipeline->getMapRequest(globalMap2);
        globalMap2->getConstKeyframeCollection()->getAllKeyframes(globalKeyframes2);
        globalMap2->getConstPointCloud()->getAllPoints(globalPointCloud2);

        if (globalPointCloud2.size() > 0) {
            globalKeyframesPoses2.clear();
            for (const auto &it : globalKeyframes2)
                globalKeyframesPoses2.push_back(it->getPose());

            LOG_INFO("==> Display final global map (after map 2 processing): press ESC on the map display window to end test");

            while (true)
            {
                if (gViewer3D->display(globalPointCloud2, {}, {}, {}, {}, globalKeyframesPoses2) == FrameworkReturnCode::_STOP)
                    break;
            }
        }
        else {
            LOG_INFO("Final global map is empty!");
        }

        LOG_INFO("Stop map update pipeline");

        mapUpdatePipeline->stop();

    }
    catch (xpcf::Exception & e) {
        LOG_INFO("The following exception has been caught: {}", e.what());
        return -1;
    }

    return 0;
}
