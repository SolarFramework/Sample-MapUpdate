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

    cxxopts::Options option_list("SolARServiceTest_MapUpdate_DisplayMap",
                                 "SolARServiceTest_MapUpdate_DisplayMap - The commandline interface to the xpcf grpc client test application.\n");
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
        std::cout << "SolARServiceTest_MapUpdate_DisplayMap version " << MYVERSION << std::endl << std::endl;
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

        // Display the current global map

        auto gViewer3D = componentManager->resolve<display::I3DPointsViewer>();

        LOG_INFO("Try to get current global map from Map Update remote service");

        SRef<Map> globalMap;
        std::vector<SRef<Keyframe>> globalKeyframes;
        std::vector<SRef<CloudPoint>> globalPointCloud;
        std::vector<Transform3Df> globalKeyframesPoses;

        mapUpdatePipeline->getMapRequest(globalMap);

        LOG_INFO("Map Update request terminated");

        globalMap->getConstKeyframeCollection()->getAllKeyframes(globalKeyframes);
        globalMap->getConstPointCloud()->getAllPoints(globalPointCloud);

        if (globalPointCloud.size() > 0) {
            for (const auto &it : globalKeyframes)
                globalKeyframesPoses.push_back(it->getPose());

            LOG_INFO("==> Display current global map: press ESC on the map display window to end test");

            while (true)
            {
                if (gViewer3D->display(globalPointCloud, {}, {}, {}, {}, globalKeyframesPoses) == FrameworkReturnCode::_STOP)
                    break;
            }
        }
        else {
            LOG_INFO("Current global map is empty!");
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
