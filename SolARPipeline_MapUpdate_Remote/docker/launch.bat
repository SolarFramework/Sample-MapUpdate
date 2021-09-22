ECHO off

REM Set application log level
REM Log level expected: DEBUG, CRITICAL, ERROR, INFO, TRACE, WARNING
SET SOLAR_LOG_LEVEL=INFO

docker rm -f solarpipelinemapupdateremote
docker run -d -p 50053:8080 -e SOLAR_LOG_LEVEL -e "SERVICE_NAME=SolARPipelineMapUpdateRemote" --log-opt max-size=50m -e "SERVICE_TAGS=SolAR" --name solarpipelinemapupdateremote artwin/solar/pipeline/map-update-remote:latest
