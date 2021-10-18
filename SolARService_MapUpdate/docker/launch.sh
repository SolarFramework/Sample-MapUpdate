#!/bin/sh

# Set application log level
# Log level expected: DEBUG, CRITICAL, ERROR, INFO, TRACE, WARNING
export SOLAR_LOG_LEVEL=INFO

docker rm -f solarservicemapupdate
docker run -d -p 50053:8080 -e SOLAR_LOG_LEVEL -e "SERVICE_NAME=SolARServiceMapUpdate" --log-opt max-size=50m -e "SERVICE_TAGS=SolAR" --name solarservicemapupdate artwin/solar/services/map-update-service:latest
