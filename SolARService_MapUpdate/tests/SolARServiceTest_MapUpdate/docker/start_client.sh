#!/bin/sh

## Detect MAPUPDATE_SERVICE_URL var and use its value 
## to set the Map Update service URL in XML configuration file

cd /SolARServiceMapUpdateClient

if [ -z "$MAPUPDATE_SERVICE_URL" ]
then
    echo "Error: You must define MAPUPDATE_SERVICE_URL env var with the MapUpdate Service URL"
    exit 1 
else
    echo "MAPUPDATE_SERVICE_URL defined: $MAPUPDATE_SERVICE_URL"
fi

echo "Try to replace the MapUpdate Service URL in the XML configuration file..."

sed -i -e "s/MAPUPDATE_SERVICE_URL/$MAPUPDATE_SERVICE_URL/g" /.xpcf/SolARServiceTest_MapUpdate_conf.xml

echo "XML configuration file ready"

## Set application log level
## Log level expected: DEBUG, CRITICAL, ERROR, INFO, TRACE, WARNING
export SOLAR_LOG_LEVEL=INFO

export LD_LIBRARY_PATH=.:./modules/

## Start client
./SolARServiceTest_MapUpdate -f /.xpcf/SolARServiceTest_MapUpdate_conf.xml

