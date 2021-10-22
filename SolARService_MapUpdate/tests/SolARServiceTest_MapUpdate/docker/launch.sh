#!/bin/sh

# Get Map Update Service URL from parameters
if [ "$1" ]
then
    echo "Map Update Service URL = $1"
else
    echo "You need to give Map Update Service URL as parameter!"
    exit 1
fi

# Set MapUpdate Service URL
export MAPUPDATE_SERVICE_URL=$1

# Get host IP for display
if [ "$2" ]
then
    echo "Display IP = $2"
else
    echo "You need to give host IP address for display as second parameter!"
    exit 1
fi

# Set Display IP
export DISPLAY=$2:0.0

# Set application log level
# Log level expected: DEBUG, CRITICAL, ERROR, INFO, TRACE, WARNING
export SOLAR_LOG_LEVEL=INFO

docker rm -f solarservicemapupdateclient
docker run -it -d -e DISPLAY -e MAPUPDATE_SERVICE_URL -e SOLAR_LOG_LEVEL -e "SERVICE_NAME=SolARServiceMapUpdateClient" -v /tmp/.X11-unix:/tmp/.X11-unix --net=host --log-opt max-size=50m -e "SERVICE_TAGS=SolAR" --name solarservicemapupdateclient artwin/solar/services/map-update-client:latest
