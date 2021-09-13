#!/bin/bash

## Set application gRPC server url
export XPCF_GRPC_SERVER_URL=0.0.0.0:50053
## Set application gRPC max receive message size (-1 for max value)
export XPCF_GRPC_MAX_RECV_MSG_SIZE=-1
## Set application gRPC max send message size (-1 for max value)
export XPCF_GRPC_MAX_SEND_MSG_SIZE=-1

## Set application log level
## Log level expected: DEBUG, CRITICAL, ERROR, INFO, TRACE, WARNING
export SOLAR_LOG_LEVEL=INFO

./SolARPipeline_MapUpdate_Remote -m SolARPipeline_MapUpdate_Remote_modules.xml -p SolARPipeline_MapUpdate_Remote_properties.xml

