# ARCloud Map Udpate Service 
[![License](https://img.shields.io/github/license/SolARFramework/SolARModuleTools?style=flat-square&label=License)](https://www.apache.org/licenses/LICENSE-2.0)

## Introduction

The purpose of this documentation is to present the *ARCloud Map Update Service*: a computer vision pipeline dedicated to Augmented Reality (AR) applications and ready to be deployed on a Cloud architecture.

First, we will present the map udpate pipeline on which this remote service is based. Then, we will describe how to configure and deploy the service on a Cloud architecture. And finally, we will explain how to test this map update service using a sample application.

## Contents of the Map Udpate Service package

The Map Udpate Service is delivered as a package containing all the files you need to deploy and test this computer vision pipeline on your own Cloud architecture.

This package includes:

* the **Docker image** of the remote service, available on a public docker repository (https://artifact.b-com.com/webapp/#/artifacts/browse/tree/General/solar-docker-local/mapupdate/0.10.0/mapupdate-service/latest)

* the **Kubernetes manifest file** to deploy the service on your own Cloud architecture ( https://artifact.b-com.com/webapp/#/artifacts/browse/tree/General/solar-helm-virtual/mapupdate-service-manifest.yaml)

* **a test client applications**, grouped in a compressed file (https://artifact.b-com.com/webapp/#/artifacts/browse/tree/General/solar-generic-local/mapupdate-service-tests/mapupdate_service_test_sample.tar.gz) which contains:

  * the **client application** : _SolARServiceTest_MapUpdate_
  * the **client configuration** : _SolARServiceTest_MapUpdate_conf.xml_
  * a **script to launch the client application**: _start_client.sh_
  * all the **libraries needed by the test application**, stored in the _modules_ folder
  * a **script to install the data** needed by the two test applications: _install_data.sh_

> :information_source: The complete projects of these two test applications are available on the **SolAR Framework GitHub**:
> * https://github.com/SolarFramework/Sample-MapUpdate/tree/develop/SolARService_MapUpdate/tests/SolARServiceTest_MapUpdate

> :warning: The Map Update Service Docker image, based on Ubuntu 18.04, is completely independant from other external resources, and can be deployed on any Cloud infrastructure that supports Docker containers.

> :warning: The test client application were built on a **Linux** operating system (Ubuntu 18.04) and must be used on that system only.


## The Map Update Pipeline 

This computer vision pipeline has been developed using the **SolAR Framework**, an open-source framework under [Apache v2 licence](https://www.apache.org/licenses/LICENSE-2.0) dedicated to Augmented Reality. 

The objective of this map udpate pipeline is to build and maintain a **global map** from all local maps provided by each AR device, using the **Mapping Service**.

This pipeline is able to process **sparse maps** made up of the following data:

* identification: to uniquely identify a map
* coordinate system: the coordinate system used by the map
* point cloud: set of 3D points constituting the sparse map
* keyframes: set of keyframes selected among captured images to approximately cover all viewpoints of the navigation space
* covisibility graph: graph consisting of keyframes as nodes so that edges between keyframes exist if they share a minimum of common map points
* keyframe retriever: set of nearest images (the most similar to a reference image) used to accelerate feature matching

This data is defined in a specific data structure **Map** that is detailed on the SolAR web site, in the API part: https://solarframework.github.io/create/api/

For each local map, the processing of this pipeline is divided into <u>three main steps</u>:

* The **overlap detection**: If no relative transformation is known to go from the local map coordinate system to the global map coordinate system (in this case, the local map is considered as a **floating map**), the pipeline tries to detect overlap area between the two maps, in order to be able to calculate the transformation to be applied to process the next step.
* The **map fusion**: This step allows to merge the local map into the global map, using the same coordinate system. For that, the pipeline performs the following treatments:

  * change local map coordinate system to that of the global map
  * detect of duplicated cloud points between the two maps
  * merge cloud points and fuse duplicated points
  * merge keyframes and update the covisibility graph
  * merge keyframe retriever models
  * process a global bundle adjustment
* The **map update**: This step ensures maximum consistency between the global map and the current state of the real world, by regularly updating the map to adjust to real-world evolutions (taking into account lighting conditions, moving objects, etc.).

To initialize the map update pipeline processing, a device must give **the characteristics of the camera** it uses (resolution, focal).

Then, the pipeline is able to process maps. To do this, the **local map** calculated from the images and poses captured by the camera of the device must be sent to it.

At any time, the current **global map** can be requested from the Map Update pipeline, which then returns a map data structure.

To facilitate the use of this pipeline by any client application embedded in a device, it offers a simple interface based on the _SolAR::api::pipeline::IMapUpdatePipeline_ class (see https://solarframework.github.io/create/api/ for interface definition and data structures).

This interface is defined as follows:

```cpp
  /// @brief Initialization of the pipeline
    /// @return FrameworkReturnCode::_SUCCESS if the init succeed, else FrameworkReturnCode::_ERROR_
    FrameworkReturnCode init() override;
```

```cpp
    /// @brief Set the camera parameters
    /// @param[in] cameraParams: the camera parameters (its resolution and its focal)
    /// @return FrameworkReturnCode::_SUCCESS if the camera parameters are correctly set, else FrameworkReturnCode::_ERROR_
    FrameworkReturnCode setCameraParameters(const datastructure::CameraParameters & cameraParams) override;
```

```cpp
    /// @brief Start the pipeline
    /// @return FrameworkReturnCode::_SUCCESS if the stard succeed, else FrameworkReturnCode::_ERROR_
    FrameworkReturnCode start() override;
```

```cpp
    /// @brief Stop the pipeline.
    /// @return FrameworkReturnCode::_SUCCESS if the stop succeed, else FrameworkReturnCode::_ERROR_
    FrameworkReturnCode stop() override;
```

```cpp
    /// @brief Request to the map update pipeline to update the global map from a local map
    /// @param[in] map: the input local map to process
    /// @return FrameworkReturnCode::_SUCCESS if the data are ready to be processed, else FrameworkReturnCode::_ERROR_
    FrameworkReturnCode mapUpdateRequest(const SRef<datastructure::Map> map) override;
```

```cpp
    /// @brief Request to the map update pipeline to get the global map
    /// @param[out] map: the output global map
    /// @return FrameworkReturnCode::_SUCCESS if the global map is available, else FrameworkReturnCode::_ERROR_
    FrameworkReturnCode getMapRequest(SRef<SolAR::datastructure::Map> & map) const override;
```

The map update pipeline project is available on the **SolAR Framework GitHub**: https://github.com/SolarFramework/Sample-MapUpdate/tree/develop/SolARService_MapUpdate


## The Map Update Service

> :warning: In order to be able to perform the instructions presented in this part, you must have installed the Kubernetes command-line tool, **_kubectl_**, on your computer: https://kubernetes.io/docs/tasks/tools/

The **Docker image** of this service is available on this public Docker repository: _solar-docker-local.artifact.b-com.com/mapupdate/0.10.0/mapupdate-service:latest_

A **Kubernetes manifest file sample** is available at this public URL: https://artifact.b-com.com/webapp/#/artifacts/browse/tree/General/solar-helm-virtual/mapupdate-service-manifest.yaml

So, you can adapt this manifest sample (in YAML format) to deploy the service:

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: map-update-service
spec:
  replicas: 1
  selector:
    matchLabels:
      app: map-update-service
  template:
    metadata:
      labels:
        app: map-update-service
    spec:
      imagePullSecrets:
      - name: regcredsupra
      containers:
      - name: map-update-service
        image: solar-docker-local.artifact.b-com.com/mapupdate/0.10.0/mapupdate-service:latest
        env:
        - name: SOLAR_LOG_LEVEL
          value: INFO
        - name: XPCF_GRPC_MAX_RECV_MSG_SIZE
          value: "-1"
        - name: XPCF_GRPC_MAX_SEND_MSG_SIZE
          value: "-1"
---
kind: Service
apiVersion: v1
metadata:
  name: map-update-service
  labels:
    app: map-update-service
spec:
  type: NodePort
  selector:
    app: map-update-service
  ports:
  - name: http
    port: 80
    targetPort: 8080
    nodePort: 31888
---
apiVersion: v1
kind: ServiceAccount
metadata:
  name: map-update-service
---
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: map-update-service-rolebinding
roleRef:
  apiGroup: rbac.authorization.k8s.io
  kind: ClusterRole
  name: cluster-admin
subjects:
- kind: ServiceAccount
  name: map-update-service
  namespace: map-update-service.artwin.b-com.com:q
---
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  annotations:
    ingress.kubernetes.io/force-ssl-redirect: "true"
    kubernetes.io/ingress.class: nginx
    nginx.ingress.kubernetes.io/backend-protocol: "GRPC"
  name:
    map-update-service
spec:
  tls:
  - hosts:
      - map-update-service.artwin.b-com.com
  rules:
  - host: map-update-service.artwin.b-com.com
    http:
      paths:
      - backend:
          service:
            name: map-update-service
            port:
              number: 80
        path: /
        pathType: Prefix
```

<u> **Deployment part** (_kind: Deployment_) </u>

In this part of the manifest document, you need to define the service deployment parameters:

* name of the deployment
```yaml
metadata:
  name: map-update-service
```

* number of replica pods for this deployment
```yaml
  replicas: 1
```

* label linked to the deployment object, used for identification
```yaml
  selector:
    matchLabels:
      app: map-update-service
  template:
    metadata:
      labels:
        app: map-update-service
```

* **container parameters**: this part is important because it defines the Docker image you want to deploy (_image: solar-docker-local.artifact.b-com.com/mapupdate/0.10.0/mapupdate-service:latest_), 
and some parameters of the Docker container used at runtime:

  * SOLAR_LOG_LEVEL: the log level of the map update service (DEBUG, CRITICAL, ERROR, INFO, TRACE, WARNING)
  * XPCF_GRPC_MAX_RECV_MSG_SIZE: the maximum size, in bytes, of gRPC received messages ("-1" for maximum value)
  * XPCF_GRPC_MAX_SEND_MSG_SIZE: the maximum size, in bytes, of gRPC sent messages ("-1" for maximum value)

```yaml
      containers:
      - name: map-update-service
        image: solar-docker-local.artifact.b-com.com/mapupdate/0.10.0/mapupdate-service:latest
        env:
        - name: SOLAR_LOG_LEVEL
          value: INFO
        - name: XPCF_GRPC_MAX_RECV_MSG_SIZE
          value: "-1"
        - name: XPCF_GRPC_MAX_SEND_MSG_SIZE
          value: "-1"
```

<u> *Service part* (_kind: Service_)</u>

This part defines the name of the service, and, more important, **the service node port**, which will be used by client applications to access its interface (in this example _31888_). The service port and target port must not be changed because they correspond to the definition of the ports of the Docker container (respectively _80_ and _8080_).

```yaml
kind: Service
apiVersion: v1
metadata:
  name: map-update-service
  labels:
    app: map-update-service
spec:
  type: NodePort
  selector:
    app: map-update-service
  ports:
  - name: http
    port: 80
    targetPort: 8080
    nodePort: 31888
```

<u>*Ingress part* (_kind: Ingress_) </u>

This last part is dedicated to the Ingress object that manages external access to the service. It defines important parameters such as **backend protocol** (_GRPC_ for the map update service), **host name** (_map_update-service.artwin.b-com.com_ in this example) and **service port number** as defined in the _service_ part.

```yaml
kind: Ingress
metadata:
  annotations:
    ingress.kubernetes.io/force-ssl-redirect: "true"
    kubernetes.io/ingress.class: nginx
    nginx.ingress.kubernetes.io/backend-protocol: "GRPC"
  name:
    map-update-service
spec:
  tls:
  - hosts:
      - map-update-service.artwin.b-com.com
  rules:
  - host: map-update-service.artwin.b-com.com
    http:
      paths:
      - backend:
          service:
            name: map-update-service
            port:
              number: 80
        path: /
        pathType: Prefix
```

TIP: You can test this service locally on your computer by following these steps:

1. Download the Service image to your own Docker engine: 

```cmd
docker pull solar-docker-local.artifact.b-com.com/mapupdate/0.10.0/mapupdate-service:latest
```

2. Get the launch script corresponding to your Operating System (Windows or Linux) from SolAR GitHub: 

**Linux**: https://github.com/SolarFramework/Sample-MapUpdate/blob/develop/SolARService_MapUpdate/docker/launch.sh

**Windows**: https://github.com/SolarFramework/Sample-MapUpdate/blob/develop/SolARService_MapUpdate/docker/launch.bat

3. Execute the launch script (by default, the log level is set to **"INFO" and the service port is set to *"50053"**: you can modify this values inside the script file)

4. Check the logs of the service: 

```cmd
docker logs solarservicemapupdate
```

You should see something like this: 

```cmd
[2021-09-22 07:24:56:772970] [info] [    7] [main():137] Load modules configuration file: /.xpcf/SolARService_MapUpdate_modules.xml
[2021-09-22 07:24:56:789855] [info] [    7] [main():146] Load properties configuration file: /.xpcf/SolARService_MapUpdate_properties.xml
[2021-09-22 07:25:00:716371] [info] [    7] [main():161] LOG LEVEL: INFO
[2021-09-22 07:25:00:716410] [info] [    7] [main():163] GRPC SERVER ADDRESS: 0.0.0.0:8080
[2021-09-22 07:25:00:716429] [info] [    7] [main():165] GRPC SERVER CREDENTIALS: 0
[2021-09-22 07:25:00:716432] [info] [    7] [main():171] GRPC MAX RECEIVED MESSAGE SIZE: 18446744073709551615
[2021-09-22 07:25:00:716435] [info] [    7] [main():178] GRPC MAX SENT MESSAGE SIZE: 18446744073709551615
[2021-09-22 07:25:00:716438] [info] [    7] [main():182] XPCF gRPC server listens on: 0.0.0.0:8080
```

If you want to test this service locally with one of our test applications (presented later in this document), you can use the **Docker bridge IP address** (and the port define in the launch script) given by this command ("Gateway" value): 

```cmd
docker network inspect bridge
[
    {
        "Name": "bridge",
        "Id": "7ec85993b0ab533552db22a0a61794f0051df5782703838948cdac82d4069674",
        "Created": "2021-09-22T06:51:14.2139067Z",
        "Scope": "local",
        "Driver": "bridge",
        "EnableIPv6": false,
        "IPAM": {
            "Driver": "default",
            "Options": null,
            "Config": [
                {
                    "Subnet": "172.17.0.0/16",
                    "Gateway": "172.17.0.1"
                }
            ]
        },
...
```


### kubectl commands

To deploy or re-deploy the map udpate service in your infrastructure, when your manifest file is correctly filled in, you can use the **kubectl** command-line tool to:

* define your **Kubernetes context** in a **.conf** file (cluster name, namespace, server address and port, user, certificates, etc.) like this:

```yaml
    apiVersion: v1
    clusters:
    - cluster:
        certificate-authority-data: ...
        server: https://...
    name: kubernetes
    contexts:
    - context:
        cluster: kubernetes
        namespace: artwin
        user: kubernetes-admin
    name: cluster-mapudpate
    current-context: cluster-mapudpate
    kind: Config
    preferences: {}
    users:
    - name: kubernetes-admin
    user:
        client-certificate-data: ...
        client-key-data: ...
```

and set it as default configuration:
```command
export KUBECONFIG=path/to/your_configuration.conf
```

- **set the context to use**:
```command
kubectl config use-context [context name]
```

you can also specify the default namespace:
```command
kubectl config set-context --current --namespace=[namespace]
```

- **deploy your service**:
```command
kubectl apply -f [path/to/your_manifest.yaml]
```

- **check your deployments**:
```command
kubectl get deployments
NAME               READY   UP-TO-DATE   AVAILABLE   AGE
mapupdate-service   1/1     1            1           21d
```

- **check your services**:
```command
kubectl get services
NAME                      TYPE       CLUSTER-IP       EXTERNAL-IP   PORT(S)          AGE
mapupdate-service          NodePort   10.107.117.85    <none>        80:31887/TCP     30d
```

- **check your pods**:
```command
kubectl get pods
NAME                                READY   STATUS    RESTARTS   AGE
mapupdate-service-84bc74d954-6vc7v   1/1     Running   1          7d15h
```

- **visualize the logs of a pod**:
```command
kubectl logs -f [pod name]
```

- **restart a pod**: to do that, you can for example change an environment variable of the pod, to force it to restart
```command
kubectl set env deployment [deployment name] SOLAR_LOG_LEVEL=INFO
```

information: You can find more kubectl commands on this web site: https://kubernetes.io/docs/reference/kubectl/cheatsheet/

## Map Update Service test samples

This sample application is used to test the processing of local maps carried out remotely by the Map Update Service. It uses a predefined local map dataset, included in this sample package.

The algorithm in this example is quite simple:

1. It requests the Map Update service for the first time to get its current global map, and displays it in a dedicated window if available (in the form of a point cloud). To do that, the application uses this method from the pipeline interface:

```cpp
    /// @brief Request to the map update pipeline to get the global map
    /// @param[out] map: the output global map
    /// @return FrameworkReturnCode::_SUCCESS if the global map is available, else FrameworkReturnCode::_ERROR_
    FrameworkReturnCode getMapRequest(SRef<SolAR::datastructure::Map> & map) const override;
```

2. Then, it reads the local map dataset stored in the _./data/maps_ folder, and sends it for processing to the Map Update Service using this method from its interface: 

```cpp
    /// @brief Request to the map update pipeline to update the global map from a local map
    /// @param[in] map: the input local map to process
    /// @return FrameworkReturnCode::_SUCCESS if the data are ready to be processed, else FrameworkReturnCode::_ERROR_
    FrameworkReturnCode mapUpdateRequest(const SRef<datastructure::Map> map) override;
```

3. And to finish, it requests again the Map Update service to get its new global map, and displays it in a dedicated window if available (in the form of a point cloud).

Traces are displayed at runtime to follow the progress of the application. 

#### Install data files

The map update client needs specific data to run (Hololens camera calibration, bag of words vocanulary and maps already built with the mapping service). 

**To install this dataset from a remote repository, just use the _install_data.sh_ script included in the sample package.**

This script file will download all the data and store it in a new **data** folder containing: 

* the Hololens camera calibration file: _./data/calibrations/hololens_calibration.json_
* maps already built: _./data/maps/map_A_ and _./data/maps/map_B_
* the bag of words vocabulary: _./data/fbow_voc/akaze.fbow_

#### Launch the test application


First, you have to install the test application image on your Docker engine: 

```command
docker pull solar-docker-local.artifact.b-com.com/tests/0.10.0/map-update-client:latest
```

Then, get the script file used to launch the Docker container of the test application from one of these URL: 

*Linux*: https://github.com/SolarFramework/Sample-MapUpdate/blob/develop/SolARService_MapUpdate/tests/SolARServiceTest_MapUpdate/docker/launch.sh

*Linux VM*: https://github.com/SolarFramework/Sample-MapUpdate/blob/develop/SolARService_MapUpdate/tests/SolARServiceTest_MapUpdate/docker/launch_vm.sh

*Windows*: https://github.com/SolarFramework/Sample-MapUpdate/blob/develop/SolARService_MapUpdate/tests/SolARServiceTest_MapUpdate/docker/launch.bat

You can now launch the Docker container using the script file, giving **your Map Update Service URL** and, <u>if you do not use a Linux Virtual Machine</u>, **your computer local IP address (to export graphical display)** as parameter: 

```command
launch.sh [map_update_service_url] [host_IP_address]
launch_vm.sh [map_update_service_url]
launch.bat [map_update_service_url] [host_IP_address]
```

For example: 
```command
launch.bat 172.17.0.1:50053 192.168.56.1
```

CAUTION: On Windows host, do not forget to start the X Server manager before running the test application

Then, you can verify that the application is running correctly by looking at its traces, with this Docker command:

```command
docker logs [-f] solarservicemapupdateclient
```

And you will see something like this: 

```command
[2021-07-28 16:19:29:451534] [info] [12204] [main():87] Get component manager instance
[2021-07-28 16:19:29:451821] [info] [12204] [main():91] Load Client Remote Map Update Service configuration file: SolARServiceTest_MapUpdate_conf.xml
[2021-07-28 16:19:29:452551] [info] [12204] [main():99] Resolve IMapUpdatePipeline interface
[IMapUpdatePipeline_grpcProxy::onConfigured()]Check if remote map update service URL is defined in XPCF_GRPC_MAP_UPDATE_URL
[2021-07-28 16:19:29:482257] [info] [12204] [main():102] Resolve other components
[2021-07-28 16:19:29:506398] [info] [12204] [main():106] Client components loaded
[2021-07-28 16:19:29:506478] [info] [12204] [main():110] Initialize map update service
[2021-07-28 16:19:30:481731] [info] [12204] [main():118] Set camera parameters
[2021-07-28 16:19:30:482769] [info] [12204] [main():125] Start map update service
[2021-07-28 16:19:30:596887] [info] [12204] [main():136] Try to get initial global map from Map Update remote service
[2021-07-28 16:19:37:196074] [info] [12204] [main():151]
==> Display initial global map
```

You will also see the global map given by the Map Update Service in a dedicated window. 

```command
[2021-07-28 16:32:11:961857] [info] [12264] [main():163] Load local map
[2021-07-28 16:32:11:961959] [info] [12264] [loadFromFile():289] Loading the map from file...
[2021-07-28 16:32:12:945752] [info] [12264] [loadFromFile():346] Load done!
[2021-07-28 16:32:12:945831] [info] [12264] [main():170] Send map request for local map
[2021-07-28 16:32:12:945871] [info] [12264] [main():174] Nb points: 8338
[2021-07-28 16:32:31:572424] [info] [12264] [main():188]
==> Display new global map (after local map processing): press Ctrl+C to stop test
```

And you will see the new global map given by the Map Update Service in a dedicated window:
