<img src="https://penguin.upyun.galvincdn.com/logos/penguin_stats_logo.png"
     alt="Penguin Statistics - Logo"
     width="96px" />

# Penguin Statistics - Recognizer

[![GitHub release (latest SemVer)](https://img.shields.io/github/v/release/penguin-statistics/recognizer)](https://github.com/penguin-statistics/recognizer/releases)
[![Last Commit](https://img.shields.io/github/last-commit/penguin-statistics/recognizer)](https://github.com/penguin-statistics/recognizer/commits/v4)
[![GitHub Actions Status](https://github.com/penguin-statistics/recognizer/actions/workflows/build-release.yml/badge.svg)](https://github.com/penguin-statistics/recognizer/actions/workflows/build-release.yml)
![GitHub](https://img.shields.io/github/license/penguin-statistics/recognizer)

This is the core of recognition component for [Penguin Statistics](https://penguin-stats.io/?utm_source=github), including:

+ screenshot recognition
+ depot recognition (in development)

This recognizer is developed by C++ and providing front-end recognition service by [WebAssembly](https://webassembly.org/) technology. It can be also used in other scenarios to get the statistics of Arknights.

# Acquire Pre-built WASM

Head to [releases](https://github.com/penguin-statistics/recognizer/releases) to get the latest release of the WASM.

The WASMs here in releases are built automatically by GitHub Actions via Docker. Inspect them as you like at [Dockerfile](https://github.com/penguin-statistics/recognizer/blob/v4/Dockerfile), [GitHub Actions](https://github.com/penguin-statistics/recognizer/blob/v4/.github/workflows/build-release.yml) and [GitHub Actions History](https://github.com/penguin-statistics/recognizer/actions/workflows/build-release.yml).

# Build WASM via Docker

To simplify the process of building WASM for the recognizer, we provide a builder Docker image for the one to build the WASM.

The Docker image is stored at [GitHub Package recognizer-builder](https://github.com/penguin-statistics/recognizer/pkgs/container/recognizer-builder). To build your own WASM, you'd need to use a `Dockerfile` to use the pre-built builder image as the base image, and build your WASM upon that. You can find the Dockerfile under this repository. After changing anything you'd want, you can build your own WASM by running the following command:

```bash
$ docker build .
```

This will build the WASM using the `Dockerfile` (instead of using `Dockerfile.builder` which is just used to build the builder). To extract the WASM and its corresponding JavaScript loader, you can run the following command:

```bash
$ docker cp <container-id>:/build/recognizer/build/dist/penguin-recognizer* ./build/penguin-recognizer*
```

# Build WASM Manually

Follow the following steps to build `penguin-recognizer` manually, in which requires you to install and configure all build dependencies yourself.

## Pre-requisites

+ OS: Unix-like (Tested: **`Ubuntu 20.04`**)
+ Python, Cmake

Clone this repository and be ready to build!

```bash
$ git clone https://github.com/penguin-statistics/recognizer.git penguin-recognizer
$ cd penguin-recognizer
```

## Install Emscripten

> ⚠ **Note on `emsdk` version**
>
> Currently, `penguin-recognizer` is built by **`Emscripten 1.39.0`**, which is verified for supporting some old version Chromium browsers.
> According to the limitation of `OpenCV`, the latest supported version is **`Emscripten 2.0.10`**, but it might not support some old version browsers.

```bash
# Get the emsdk repo
$ git clone https://github.com/emscripten-core/emsdk.git

# Enter that directory
$ cd emsdk

# Download and install the SDK tools.
$ ./emsdk install 1.39.0

# Make the "latest" SDK "active" for the current user. (writes .emscripten file)
$ ./emsdk activate 1.39.0

# Activate PATH and other environment variables in the current terminal
$ source ./emsdk_env.sh

# Check installation
$ emcc -v
```

## Before Building Opencv

> ℹ️ **Note on `OpenCV` version**
>
> Currently, `penguin-recognizer` is built by **`OpenCV 4.5.1`**.  
> Other verified versions are:  
> **`OpenCV 4.5.4`** Tested by [FlandiaYingman](https://github.com/FlandiaYingman)  
> **`OpenCV 4.5.5`** (latest) Tested by [KumoSiunaus](https://github.com/KumoSiunaus)

```bash
$ git clone --depth=1 -b 4.5.5 https://github.com/opencv/opencv.git opencv/sources
```

Before building OpenCV, some build settings have to be customized.  
Open `opencv/sources/platforms/js/build_js.py` by an editor, and change the following settings.

```plain
# In def get_cmake_cmd(self)

# Turn these settings to ON, which are necessary for building
-DWITH_JPEG=OFF               -> ON
-DWITH_PNG=OFF                -> ON
-DBUILD_opencv_imgcodecs=OFF  -> ON

# (Optional) Turn these settings to OFF to accelerate building
-DWITH_QUIRC=ON               -> OFF
-DBUILD_ZLIB=ON               -> OFF
-DBUILD_opencv_calib3d=ON     -> OFF
-DBUILD_opencv_dnn=ON         -> OFF
-DBUILD_opencv_features2d=ON  -> OFF
-DBUILD_opencv_flann=ON       -> OFF
-DBUILD_opencv_photo=ON       -> OFF
-DBUILD_EXAMPLES=ON           -> OFF
-DBUILD_TESTS=ON              -> OFF
-DBUILD_PERF_TESTS=ON         -> OFF
```

## Build OpenCV

```bash
# change directory to opencv
$ cd .../opencv

$ emcmake python ./sources/platforms/js/build_js.py build_wasm --build_wasm
```

### Finalize Dependency Builds

```bash
$ cd build_wasm
$ emmake make
```

### Verify Dependency Artifacts

Now, the following files should exist  

`opencv/build_wasm/lib/libopencv_imgcodecs.a`  
`opencv/build_wasm/3rdparty/lib/liblibjpeg-turbo.a`  
`opencv/build_wasm/3rdparty/lib/liblibpng.a`  

## Build penguin-recognizer

### Modify `CMakeLists.txt` if necessary

> If your `opencv` directory is at the same level as the `penguin-recognizer` directory, like so:
>
> ```
> .
> ├── opencv
> └── penguin-recognizer
> ```
>
> then you can skip this step as it is unnecessary.

Open `CMakeLists.txt`, modify the value of `OPENCV_DIR` to the path to your `opencv` directory.

```cmake
set(OPENCV_DIR "/path/to/opencv")
```

### Make build files

```bash
# change directory to penguin-recognizer
$ cd .../penguin-recognizer

$ mkdir build && cd build
$ emcmake cmake ..

# Should print the following output if success:
# -- Configuring done
# -- Generating done
# -- Build files have been written to: .../penguin-recognizer/build
```

### Finalize

```bash
$ emmake make

# Should print the following output if success:
# Scanning dependencies of target penguin-recognizer
# [ 50%] Building CXX object CMakeFiles/penguin-recognizer.dir/src/recognizer_wasm.cpp.o
# [100%] Linking CXX executable penguin-recognizer.js
# [100%] Built target penguin-recognizer
```

### Verify result

Now, those files should exist:

```
penguin-recognizer/build/penguin-recognizer.js  
penguin-recognizer/build/penguin-recognizer.wasm
```

# Known issues & Todo

## Known issues

1. ~~In some situations, get wrong result of stage recognition. (e.g. WR-10 -> WR-1O)~~ (Resolved in v4)
2. ~~In some situations, get wrong result of drop type recognition. (e.g. LMB`yellow` -> EXTRA_DROP`green`)~~ (Resolved in v4)
3. In some situations, get wrong result of drop number recognition.

## Todo

+ [x] add automatic fallback to fix `Known issues 1.`
+ [x] add droptype order check to fix `Known issues 2.`
+ [ ] update drop number recognition strategy to fix `Known issues 3.`
+ [ ] depot recognition (in development)
+ [x] new wasm interface using emscripten::bind
+ [ ] adapt to new interface (for now, CN server)
