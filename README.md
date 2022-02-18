# penguin-recognizer

This is the core of recognition component for [Penguin Statistics](https://penguin-stats.io/?utm_source=github), including:

+ screenshot recognition
+ depot recognition (in development)

This recognizer is developed by C++ and providing front-end recognition service by [WebAssembly](https://webassembly.org/) technology. It can be also used in other scenarios to get the statistics of Arknights.

## WASM Build Guide

Follow the following steps to build `penguin-recognizer`.

### Pre-requirements

+ OS: Unix-like (Tested: **`Ubuntu 20.04`**)
+ Python, Cmake

Clone this repository and be ready to build!
```
git clone https://github.com/penguin-statistics/recognizer.git penguin-recognizer
cd penguin-recognizer
```

### Install Emscripten

> #### ⚠**Version Warning**
> Currently, `penguin-recognizer` is built by **`Emscripten 1.39.0`**, which is verified for supporting some old version chromium core browsers.  
> According to the limitation of `OpenCV`, the latest supported version is **`Emscripten 2.0.10`**, but it might not support some old version browsers.

```
# Get the emsdk repo
git clone https://github.com/emscripten-core/emsdk.git

# Enter that directory
cd emsdk

# Download and install the SDK tools.
./emsdk install 1.39.0

# Make the "latest" SDK "active" for the current user. (writes .emscripten file)
./emsdk activate 1.39.0

# Activate PATH and other environment variables in the current terminal
source ./emsdk_env.sh

# Check installation
emcc -v
```

### Build Opencv

> #### ℹ️**Version Info**
> Currently, `penguin-recognizer` is built by **`OpenCV 4.5.1`**.  
> Other verified version:  
> **`OpenCV 4.5.4`** Tested by [FlandiaYingman](https://github.com/FlandiaYingman)  
> **`OpenCV 4.5.5-dev`** (latest) Tested by [KumoSiunaus](https://github.com/KumoSiunaus)

```
git clone --depth=1 -b 4.5.5 https://github.com/opencv/opencv.git opencv/sources
```

Before building OpenCV, some build settings have to be customized.  
Open `opencv/sources/platforms/js/build_js.py` by an editor, and change the following settings.

```
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

#### Start to build OpenCV
```
# change directory to opencv
cd .../opencv

emcmake python ./sources/platforms/js/build_js.py build_wasm --build_wasm
```

#### To finish
```
cd build_wasm
emmake make
```

#### Verify result
Now, the following files should exist  

`opencv/build_wasm/lib/libopencv_imgcodecs.a`  
`opencv/build_wasm/3rdparty/lib/liblibjpeg-turbo.a`  
`opencv/build_wasm/3rdparty/lib/liblibpng.a`  

### Build penguin-recognizer

#### Modify `CMakeLists.txt` if necessary
> If your `opencv` directory is at the same level as the `penguin-recognizer` directory, skip this step.

Open `CMakeLists.txt`, modify the value of `OPENCV_DIR` to the path to your `opencv` directory.
```
set(OPENCV_DIR "/path/to/opencv")
```


#### Make build files
```
# change directory to penguin-recognizer
cd .../penguin-recognizer

mkdir build && cd build
emcmake cmake ..

# Should print the following output if success:
# -- Configuring done
# -- Generating done
# -- Build files have been written to: .../penguin-recognizer/build
```

#### To finish
```
emmake make

# Should print the following output if success:
# Scanning dependencies of target penguin-recognizer
# [ 50%] Building CXX object CMakeFiles/penguin-recognizer.dir/src/recognizer_wasm.cpp.o
# [100%] Linking CXX executable penguin-recognizer.js
# [100%] Built target penguin-recognizer
```

#### Verify result
Now, the following files should exist  

`penguin-recognizer/build/penguin-recognizer.js`  
`penguin-recognizer/build/penguin-recognizer.wasm`   

## Known issues & Todo

### Known issues

~~1. In some situations, get wrong result of stage recognition. (e.g. WR-10 -> WR-1O)~~  
2. In some situations, get wrong result of drop type recognition. (e.g. LMB`yellow` -> EXTRA_DROP`green`)  
3. In some situations, get wrong result of drop number recognition.

### Todo

+ [x] add automatic fallback to fix `Known issues 1.`
+ [ ] add droptype order check to fix `Known issues 2.`
+ [ ] update drop number recognition strategy to fix `Known issues 3.`
+ [ ] depot recognition (in development)
+ [ ] new wasm interface using emscripten::bind