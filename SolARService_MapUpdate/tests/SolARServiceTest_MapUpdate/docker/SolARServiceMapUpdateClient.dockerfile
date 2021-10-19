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

## Copy SolARServiceMapUpdateClient app files
RUN mkdir SolARServiceMapUpdateClient

## Data files (maps, hololens configuration)
RUN mkdir SolARServiceMapUpdateClient/data
RUN mkdir SolARServiceMapUpdateClient/data/calibrations
ADD data/calibrations/* /SolARServiceMapUpdateClient/data/calibrations/
RUN mkdir SolARServiceMapUpdateClient/data/maps
RUN mkdir SolARServiceMapUpdateClient/data/maps/mapA
ADD data/maps/mapA/* /SolARServiceMapUpdateClient/data/maps/mapA/
RUN mkdir SolARServiceMapUpdateClient/data/maps/mapB
ADD data/maps/mapB/* /SolARServiceMapUpdateClient/data/maps/mapB/

## Libraries and modules
RUN mkdir SolARServiceMapUpdateClient/modules
ADD modules/* /SolARServiceMapUpdateClient/modules/

## Project files
ADD SolARServiceTest_MapUpdate /SolARServiceMapUpdateClient/
RUN chmod +x /SolARServiceMapUpdateClient/SolARServiceTest_MapUpdate
RUN mkdir .xpcf
ADD *.xml /.xpcf/
ADD docker/start_client.sh .
RUN chmod +x start_client.sh

## Set application log level
## Log level expected: DEBUG, CRITICAL, ERROR, INFO, TRACE, WARNING
ENV SOLAR_LOG_LEVEL=INFO

## Run Server
CMD [ "./start_client.sh"  ]
