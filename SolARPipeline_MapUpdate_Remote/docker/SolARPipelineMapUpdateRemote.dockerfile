FROM ubuntu:18.04
MAINTAINER Christophe Cutullic christophe.cutullic@b-com.com

## Configure Ubuntu environment
RUN apt-get update -y
RUN apt-get install -y libgtk-3-0
RUN apt-get install -y libva-dev
RUN apt-get install -y libvdpau-dev

## Copy SolARPipelineMapUpdateRemote app files
RUN mkdir SolARPipelineMapUpdateRemote

## Data files (fbow vocabulary)
RUN mkdir SolARPipelineMapUpdateRemote/data
RUN mkdir SolARPipelineMapUpdateRemote/data/fbow_voc
ADD data/fbow_voc/* /SolARPipelineMapUpdateRemote/data/fbow_voc/
RUN mkdir SolARPipelineMapUpdateRemote/data/maps
RUN mkdir SolARPipelineMapUpdateRemote/data/maps/globalMap

## Libraries and modules
RUN mkdir SolARPipelineMapUpdateRemote/modules
ADD modules/* /SolARPipelineMapUpdateRemote/modules/

## Project files
ADD SolARPipeline_MapUpdate_Remote /SolARPipelineMapUpdateRemote/
RUN chmod +x /SolARPipelineMapUpdateRemote/SolARPipeline_MapUpdate_Remote
RUN mkdir .xpcf
ADD *.xml /.xpcf
ADD docker/start_server.sh .
RUN chmod +x start_server.sh

## Set application gRPC server url
ENV XPCF_GRPC_SERVER_URL=0.0.0.0:8080
## Set application gRPC max receive message size (-1 for max value)
ENV XPCF_GRPC_MAX_RECV_MSG_SIZE=-1
## Set application gRPC max send message size (-1 for max value)
ENV XPCF_GRPC_MAX_SEND_MSG_SIZE=-1
## Set application log level
## Log level expected: DEBUG, CRITICAL, ERROR, INFO, TRACE, WARNING
ENV SOLAR_LOG_LEVEL=INFO

## Run Server
CMD [ "./start_server.sh"  ]
