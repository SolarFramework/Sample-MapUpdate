#!/bin/sh

export LD_LIBRARY_PATH=/SolARPipelineMapUpdateRemote:/SolARPipelineMapUpdateRemote/modules/
cd /SolARPipelineMapUpdateRemote
./SolARPipeline_MapUpdate_Remote -m /.xpcf/SolARPipeline_MapUpdate_Remote_modules.xml -p /.xpcf/SolARPipeline_MapUpdate_Remote_properties.xml

