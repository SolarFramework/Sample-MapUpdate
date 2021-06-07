// Copyright (C) 2017-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <iostream>

#include <cxxopts.hpp>
#include <boost/log/core.hpp>

#include <xpcf/api/IComponentManager.h>
#include <xpcf/core/helpers.h>
#include "GrpcServerManager.h"
#include <cstdlib>
#include <boost/filesystem.hpp>
#include <boost/filesystem/detail/utf8_codecvt_facet.hpp>
#include "core/Log.h"

using namespace SolAR;

namespace fs = boost::filesystem;

namespace xpcf = org::bcom::xpcf;

// print help options
void print_help(const cxxopts::Options& options)
{
    std::cout << options.help({""}) << std::endl;
}

// print error message
void print_error(const std::string& msg)
{
    std::cerr << msg << std::endl;
}

int main(int argc, char* argv[])
{
#if NDEBUG
    boost::log::core::get()->set_logging_enabled(false);
#endif

    LOG_ADD_LOG_TO_CONSOLE();

    fs::detail::utf8_codecvt_facet utf8;
    SRef<xpcf::IComponentManager> cmpMgr = xpcf::getComponentManagerInstance();
    cmpMgr->bindLocal<xpcf::IGrpcServerManager,xpcf::GrpcServerManager>();
    std::string configSrc;
    fs::path currentPath(boost::filesystem::initial_path().generic_string(utf8));
    configSrc = currentPath.generic_string(utf8);

    cxxopts::Options option_list("SolARPipeline_MapUpdate_Remote",
                                 "SolARPipeline_MapUpdate_Remote - The commandline interface to the xpcf grpc server application.\n");
    option_list.add_options()
            ("h,help", "display this help and exit")
            ("v,version", "display version information and exit")
            ("m,modules", "XPCF modules configuration file",
             cxxopts::value<std::string>())
            ("p,properties", "XPCF properties configuration file",
             cxxopts::value<std::string>());

    auto options = option_list.parse(argc, argv);
    if (options.count("help")) {
        print_help(option_list);
        return 0;
    }
    else if (options.count("version"))
    {
        std::cout << "SolARPipeline_MapUpdate_Remote version " << MYVERSION << std::endl << std::endl;
        return 0;
    }
    else if ((!options.count("modules") || options["modules"].as<std::string>().empty())
          || (!options.count("properties") || options["properties"].as<std::string>().empty())) {
        print_error("missing one of modules (-m) or properties (-p) argument");
        return -1;
    }

    configSrc = options["modules"].as<std::string>();

    std::cout << "Load modules configuration file: " << configSrc << std::endl;

    if (cmpMgr->load(configSrc.c_str()) != org::bcom::xpcf::_SUCCESS) {
        std::cout << "Failed to load modules configuration file: " << configSrc << std::endl;
        return -1;
    }

    configSrc = options["properties"].as<std::string>();

    std::cout << "Load properties configuration file: " << configSrc << std::endl;

    if (cmpMgr->load(configSrc.c_str()) != org::bcom::xpcf::_SUCCESS) {
        std::cout << "Failed to load properties configuration file: " << configSrc << std::endl;
        return -1;
    }

    auto serverMgr = cmpMgr->resolve<xpcf::IGrpcServerManager>();

    // Check if server port is defined in environment variable XPCF_GRPC_SERVER_PORT

    char * serverPort = getenv("XPCF_GRPC_SERVER_PORT");

    if (serverPort != nullptr) {
        serverMgr->bindTo<xpcf::IConfigurable>()->getProperty("server_port")->setStringValue(serverPort);

        std::cout << "'XPCF_GRPC_SERVER_PORT' environment variable found: set server port to "
                  << serverMgr->bindTo<xpcf::IConfigurable>()->getProperty("server_port")->getStringValue() << std::endl;
    }
    else {
        std::cout<<"No 'XPCF_GRPC_SERVER_PORT' environment variable found: set server port to default value ("
                 << serverMgr->bindTo<xpcf::IConfigurable>()->getProperty("server_port")->getStringValue() << ")" << std::endl;
    }

    // Check if log level is defined in environment variable SOLAR_LOG_LEVEL
    char * log_level = getenv("SOLAR_LOG_LEVEL");

    if (log_level != nullptr) {
        std::string str_log_level(log_level);

        std::cout << "'SOLAR_LOG_LEVEL' environment variable found with value: " << log_level << std::endl;

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
            std::cout << "*********************************************************************" << std::endl;
            std::cout << "'SOLAR_LOG_LEVEL' environment variable: invalid value" << std::endl;
            std::cout << "Expected values are: DEBUG, CRITICAL, ERROR, INFO, TRACE or WARNING" << std::endl;
            std::cout << "Set log level to default value" << std::endl;
            std::cout << "*********************************************************************" << std::endl;
        }
    }
    else {
        std::cout << "No 'SOLAR_LOG_LEVEL' environment variable found: set log level to default value (INFO)" << std::endl;
    }

    serverMgr->runServer();

    return 0;
}
