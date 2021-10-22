#!/bin/bash

export REMAKENROOT=$HOME/.remaken/packages/linux-gcc
export XPCF_MODULE_ROOT=$HOME/.remaken/packages/linux-gcc
export PKG_CONFIG_PATH=/home/linuxbrew/.linuxbrew/opt/openssl/lib/pkgconfig:/home/linuxbrew/.linuxbrew/lib/pkgconfig/
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/linuxbrew/.linuxbrew/lib:.

## Set application gRPC server url
export XPCF_GRPC_SERVER_URL=0.0.0.0:50053
## Set application gRPC max receive message size (-1 for max value)
export XPCF_GRPC_MAX_RECV_MSG_SIZE=-1
## Set application gRPC max send message size (-1 for max value)
export XPCF_GRPC_MAX_SEND_MSG_SIZE=-1

## Set application log level
## Log level expected: DEBUG, CRITICAL, ERROR, INFO, TRACE, WARNING
export SOLAR_LOG_LEVEL=DEBUG

# include dependencies path to ld_library_path
ld_library_path="./"
if [ -f "$PWD/SolARService_MapUpdate_modules.xml" ]; then
        for modulePath in $(grep -o "\$XPCF_MODULE_ROOT.*lib" SolARService_MapUpdate_modules.xml)
        do
           modulePath=${modulePath/"\$XPCF_MODULE_ROOT"/${XPCF_MODULE_ROOT}}
           if ! [[ $ld_library_path =~ "$modulePath/x86_64/shared/debug" ]]
           then
                  ld_library_path=$ld_library_path:$modulePath/x86_64/shared/debug
           fi
        done
fi

echo LD_LIBRARY_PATH $ld_library_path

LD_LIBRARY_PATH=$ld_library_path ./SolARService_MapUpdate -m SolARService_MapUpdate_modules.xml -p SolARService_MapUpdate_properties.xml

