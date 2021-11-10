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

void tryConfigureServer(SRef<xpcf::IGrpcServerManager> server, const std::string & propName, const std::string & envVarName)
{
    char * envValue = getenv(envVarName.c_str());
    if (envValue != nullptr) {
        LOG_DEBUG("Environment variable {}: {}", envVarName, envValue);
        xpcf::IProperty::PropertyType type = server->bindTo<xpcf::IConfigurable>()->getProperty(propName.c_str())->getType();
        switch (type) {
        case xpcf::IProperty::PropertyType::IProperty_CHARSTR:
            server->bindTo<xpcf::IConfigurable>()->getProperty(propName.c_str())->setStringValue(envValue);
            break;

        case xpcf::IProperty::PropertyType::IProperty_UINTEGER:
            server->bindTo<xpcf::IConfigurable>()->getProperty(propName.c_str())->setUnsignedIntegerValue(std::atoi(envValue));
            break;

        case xpcf::IProperty::PropertyType::IProperty_LONG:
            server->bindTo<xpcf::IConfigurable>()->getProperty(propName.c_str())->setLongValue(std::atol(envValue));
        break;
        default:
            LOG_DEBUG("GrpcServerManager Property type not handled");
            break;
        }
    }
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
        LOG_ERROR("missing one of modules (-m) or properties (-p) argument");
        return -1;
    }

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

    configSrc = options["modules"].as<std::string>();

    LOG_INFO("Load modules configuration file: {}", configSrc);

    if (cmpMgr->load(configSrc.c_str()) != org::bcom::xpcf::_SUCCESS) {
        LOG_ERROR("Failed to load modules configuration file: {}", configSrc);
        return -1;
    }

    configSrc = options["properties"].as<std::string>();

    LOG_INFO("Load properties configuration file: {}", configSrc);

    if (cmpMgr->load(configSrc.c_str()) != org::bcom::xpcf::_SUCCESS) {
        LOG_ERROR("Failed to load properties configuration file: {}", configSrc);
        return -1;
    }

    auto serverMgr = cmpMgr->resolve<xpcf::IGrpcServerManager>();

    // Check environment variables
    tryConfigureServer(serverMgr, "server_address", "XPCF_GRPC_SERVER_URL");
    tryConfigureServer(serverMgr, "server_credentials", "XPCF_GRPC_CREDENTIALS");
    tryConfigureServer(serverMgr, "max_receive_message_size", "XPCF_GRPC_MAX_RECV_MSG_SIZE");
    tryConfigureServer(serverMgr, "max_send_message_size", "XPCF_GRPC_MAX_SEND_MSG_SIZE");

    LOG_INFO ("LOG LEVEL: {}", str_log_level);
    LOG_INFO ("GRPC SERVER ADDRESS: {}",
              serverMgr->bindTo<xpcf::IConfigurable>()->getProperty("server_address")->getStringValue());
    LOG_INFO ("GRPC SERVER CREDENTIALS: {}",
              serverMgr->bindTo<xpcf::IConfigurable>()->getProperty("server_credentials")->getUnsignedIntegerValue());
    uint64_t max_msg_size = serverMgr->bindTo<xpcf::IConfigurable>()->getProperty("max_receive_message_size")->getLongValue();
    if (max_msg_size == 0) {
        LOG_INFO ("GRPC MAX RECEIVED MESSAGE SIZE: 4000000 (default) {}", max_msg_size);
    }
    else {
        LOG_INFO ("GRPC MAX RECEIVED MESSAGE SIZE: {}", max_msg_size);
    }
    max_msg_size = serverMgr->bindTo<xpcf::IConfigurable>()->getProperty("max_send_message_size")->getLongValue();
    if (max_msg_size == 0) {
        LOG_INFO ("GRPC MAX SENT MESSAGE SIZE: 4000000 (default)");
    }
    else {
        LOG_INFO ("GRPC MAX SENT MESSAGE SIZE: {}", max_msg_size);
    }

    LOG_INFO ("XPCF gRPC server listens on: {}",
              serverMgr->bindTo<xpcf::IConfigurable>()->getProperty("server_address")->getStringValue())

    serverMgr->runServer();

    return 0;
}
