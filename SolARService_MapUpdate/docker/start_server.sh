#!/bin/sh

export LD_LIBRARY_PATH=/SolARServiceMapUpdate:/SolARServiceMapUpdate/modules/
cd /SolARServiceMapUpdate
./SolARService_MapUpdate -m /.xpcf/SolARService_MapUpdate_modules.xml -p /.xpcf/SolARService_MapUpdate_properties.xml

