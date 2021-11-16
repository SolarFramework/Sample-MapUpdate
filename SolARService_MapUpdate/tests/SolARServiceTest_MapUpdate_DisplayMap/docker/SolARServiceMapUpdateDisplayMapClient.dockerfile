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

## Copy SolARServiceMapUpdateDisplayMapClient app files
RUN mkdir SolARServiceMapUpdateDisplayMapClient

## Data files (hololens configuration)
RUN mkdir SolARServiceMapUpdateDisplayMapClient/data
RUN mkdir SolARServiceMapUpdateDisplayMapClient/data/calibrations
ADD data/calibrations/* /SolARServiceMapUpdateDisplayMapClient/data/calibrations/

## Libraries and modules
RUN mkdir SolARServiceMapUpdateDisplayMapClient/modules
ADD modules/* /SolARServiceMapUpdateDisplayMapClient/modules/

## Project files
ADD SolARServiceTest_MapUpdate_DisplayMap /SolARServiceMapUpdateDisplayMapClient/
RUN chmod +x /SolARServiceMapUpdateDisplayMapClient/SolARServiceTest_MapUpdate_DisplayMap
RUN mkdir .xpcf
ADD *.xml /.xpcf/
ADD docker/start_client.sh .
RUN chmod +x start_client.sh

## Set application log level
## Log level expected: DEBUG, CRITICAL, ERROR, INFO, TRACE, WARNING
ENV SOLAR_LOG_LEVEL=INFO

## Run Server
CMD [ "./start_client.sh"  ]
