FROM ubuntu:18.04
MAINTAINER Christophe Cutullic christophe.cutullic@b-com.com

## Configure Ubuntu environment
RUN apt-get update -y
RUN apt-get install -y libgtk-3-0
RUN apt-get install -y libva-dev
RUN apt-get install -y libvdpau-dev

# Manage graphical display
RUN apt-get install -y xterm
RUN useradd -ms /bin/bash xterm

## Copy SolARPipelineMapUpdateClient app files
RUN mkdir SolARPipelineMapUpdateClient

## Data files (maps, hololens configuration)
RUN mkdir SolARPipelineMapUpdateClient/data
RUN mkdir SolARPipelineMapUpdateClient/data/calibrations
ADD data/calibrations/* /SolARPipelineMapUpdateClient/data/calibrations/
RUN mkdir SolARPipelineMapUpdateClient/data/maps
RUN mkdir SolARPipelineMapUpdateClient/data/maps/mapA
ADD data/maps/mapA/* /SolARPipelineMapUpdateClient/data/maps/mapA/
RUN mkdir SolARPipelineMapUpdateClient/data/maps/mapB
ADD data/maps/mapB/* /SolARPipelineMapUpdateClient/data/maps/mapB/

## Libraries and modules
RUN mkdir SolARPipelineMapUpdateClient/modules
ADD modules/* /SolARPipelineMapUpdateClient/modules/

## Project files
ADD SolARPipelineTest_MapUpdate_Remote /SolARPipelineMapUpdateClient/
RUN chmod +x /SolARPipelineMapUpdateClient/SolARPipelineTest_MapUpdate_Remote
RUN mkdir .xpcf
ADD *.xml /.xpcf
ADD docker/start_client.sh .
RUN chmod +x start_client.sh

## Set application log level
## Log level expected: DEBUG, CRITICAL, ERROR, INFO, TRACE, WARNING
ENV SOLAR_LOG_LEVEL=INFO

## Run Server
CMD [ "./start_client.sh"  ]
