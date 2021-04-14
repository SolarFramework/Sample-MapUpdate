# Sample-MapUpdate
[![License](https://img.shields.io/github/license/SolARFramework/SolARModuleTools?style=flat-square&label=License)](https://www.apache.org/licenses/LICENSE-2.0)

The samples demonstrate the map update task that updates a global map based on local maps.

The samples for map update are open-source, designed by [b<>com](https://b-com.com/en), under [Apache v2 licence](https://www.apache.org/licenses/LICENSE-2.0).

## How to run

### Install required data

Before running the samples, you need to download data such as maps and the vocabulary of the bag of word used for image retrieval.
To install the required data, just launch the following script:

> #### Windows
>
	installData.bat

> #### Linux
>
	./installData.sh

This script will install the following data into the `./data` folder:
- The bag of words downloaded from our [GitHub releases](https://github.com/SolarFramework/binaries/releases/download/fbow%2F0.0.1%2Fwin/fbow_voc.zip) and unzipped in the `./data` folder.
- Maps produced from the previous captures and downloaded from our Artifactory ([mapA](https://artifact.b-com.com/solar-generic-local/maps/hololens/bcomLab/loopDesktopA.zip) and [mapB](https://artifact.b-com.com/solar-generic-local/maps/hololens/bcomLab/loopDesktopB.zip)) and copied into the `./data/map_hololens` folder.

### Install required modules

Some samples require several SolAR modules such as OpenGL, OpenCV, FBOW and G20. If they are not yet installed on your machine, please run the following command from the test folder:

<pre><code>remaken install packagedependencies.txt</code></pre>

and for debug mode:

<pre><code>remaken install packagedependencies.txt -c debug</code></pre>

For more information about how to install remaken on your machine, visit the [install page](https://solarframework.github.io/install/) on the SolAR website.

## Run the samples

### Map Fusion samples

The two samples allow to fuse two maps previoulsy built:
* <strong>Local Map Fusion</strong>: This fusion requires to specify the 3D transform between the two maps in the `TransformLocalToGlobal.txt` file. This 3D transform can be estimated with the OpenCV module test called `SolARTest_ModuleOpenCV_DeviceDualMarkerCalibration` which will estimate the transform between two markers, here the `FiducialMarkerA` reference of the `mapA`, and `FiducialMarkerB` reference of the `mapB` and also visible in the `loop_desktop_A` AR device capture.

> #### Windows
>
	SolARSample_MapUpdate_LocalMapFusion.exe

> #### Linux
>
	./run.sh ./SolARSample_MapUpdate_LocalMapFusion

* <strong>Floating Map Fusion</strong>: This fusion will automatically detect overlaps between two maps based on a keyframe retrieval approach. The overlaps detection will estimate the 3D transform between the two maps, and the the sample will merge them.

> #### Windows
>
	SolARSample_MapUpdate_FloatingMapFusion.exe

> #### Linux
>
	./run.sh ./SolARSample_MapUpdate_FloatingMapFusion