# penguin-stats-recognize

# Build Guide

Follow the following instructions to build `penguin-stats-recognize` .

## Pre-requirements

* OS: Unix-like (I use WSL 2 - Ubuntu 20.04. Windows hasn't been tested.)
* C/C++ Build Toolchains installed.

Clone this repository and be ready to build!

```sh
git clone https://github.com/penguin-statistics/penguin-stats-recognize.git
cd penguin-stats-recognize
```

## Build OpenCV to Javascript + WebAssembly

> I am not sure whether this step is needed, because `penguin-stats-recognize` seems only need the static libraries of OpenCV instead of `opencv.js` and `opencv.wasm` . However, these steps also produce the static libraries.

> Reference: https://docs.opencv.org/4.x/d4/da1/tutorial_js_setup.html

Follow the [instructions](https://emscripten.org/docs/getting_started/downloads.html) to install Emscripten SDK. 

Note that only Emscripten version 2.0.10 is compatible with OpenCV 4.5.4. Other versions may not work. Therefore: 

```sh
# replace:
# ./emsdk install latest
# ./emsdk activate latest

# with:
./emsdk install 2.0.10
./emsdk activate 2.0.10
```

Clone OpenCV 4.5.4 (other versions may work. idk).

```sh
git clone --depth 1 --branch 4.5.4 https://github.com/opencv/opencv.git
```

Before building OpenCV, some switches need to be turned on to build `opencv2/imgcodecs` . Open `opencv/platforms/js/build_js.py` with a editor, and turn `-DWITH_JPEG` , `-DWITH_PNG` and `-DBUILD_opencv_imgcodecs` to `ON` .

```py
# opencv/platforms/js/build_js.py

# Turn these 3 lines to ON.
# "-DWITH_JPEG=OFF",
# "-DWITH_PNG=OFF",
# "-DBUILD_opencv_imgcodecs=OFF",
...
"-DWITH_JPEG=ON",
"-DWITH_PNG=ON",
"-DBUILD_opencv_imgcodecs=ON",
...
```

Then build OpenCV.js: 

```sh
emcmake python opencv/platforms/js/build_js.py --build_wasm opencv_build_wasm
```

After that, if there're multiple OpenCV static libraries under `opencv_build_wasm/lib/` , that means everything goes right. However, there's no `opencv_build_wasm/lib/libopencv_imgcodecs.lib` , which means `opencv2/imgcodecs` hasn't been built. Well, although the switches in `opencv/platforms/js/build_js.py` are turned on, the script doesn't build `opencv2/imgcodecs` by its own. To build it, run CMake in build mode manually: 

```sh
cmake --build opencv_build_wasm
```

Now `opencv_build_wasm/lib/libopencv_imgcodecs.lib` should exist. The last thing to do is install what have been built in previous steps: 

```sh
cmake --install opencv_build_wasm --prefix opencv_install_wasm
```

If everything goes right, the built OpenCV should be located at `opencv_install_wasm` . The directories `opencv` and `opencv_build_wasm` can be removed now.

## Build `penguin-stats-recognize`

With CMake and Emscripten, `penguin-stats-recognize` can easily be built. In order to build it, run Emscripten CMake in `build` directory: 

```sh
emcmake cmake -S . -B build

# Should print the following lines if Emscripten CMake runs successfully:
# -- Configuring done
# -- Generating done
# -- Build files have been written to: .../penguin-stats-recognize/build
```

If the OpenCV built and insatlled in previous steps isn't located in `penguin-stats-recognize/opencv_install_wasm` , the location of the installed OpenCV should be given by adding arguments: 

```sh
emcmake cmake -S . -B build -D OpenCV_DIR=path/to/opencv
```

After a sucessfully run of Emscripten CMake, run CMake build: 

```sh
cmake --build build

# Should print the following lines if CMake build runs successfully:
# [ 50%] Building CXX object CMakeFiles/recognize.dir/recognize_v2_wasm_debug.cpp.o
# [100%] Linking CXX executable recognize.js
# [100%] Built target recognize
```

That's it! Take a look at `build` directory, there should be `recognize.js` and `recognize.wasm` .

## Test `recognize.js` and `recognize.wasm`

I've written a tiny test to test if `recognize.js` and `recognize.wasm` is whether successfully built. To run it, start a HTTP server in the project root (I use Live Server plugin of VS Code).

Open the DevTools of the browser and navigate to `/recognize_test.html` . You should see 3 buttons.

* Preload JSON
* Preload Templates
* Recognize

Click them in top-down order, then you should see the following in the console: 

```
[recognize.wasm]: preload_json() successful
[recognize.wasm]: preload_templ() successful. img.empty()=0
[recognize.wasm]: preload_templ() successful. img.empty()=0
...
Decode cost: 20.8
Image resolution: 480 x 800
Analysing...
Getting baseline_v...	[1 x 145 from (276, 318)]
Shrink coefficient: 1
Is Result...		YES
	Hamming distance: 3
	Hash value: 02003e0fbe3f823003c07fc7ffb1833187c70ff70fb20f160b550bf921b92010
Getting stage...	6-11
Is 3stars...		YES
Getting baseline_h...	[72 x 1 from (281, 449)]
Getting droptypes...
	LMB     HSV [51, 1.00, 0.93]
	NOR     HSV [ 0, 0.00, 0.69]
Getting drops...
	["NORMAL_DROP"]
	30073	扭转醇		Similarity: 97.1166%	Second Similarity: 74.5293%
		[1]		Hamming distance: 13	Second char and distance: [2] 26
		Hash value: 0fc00fc00fc003c003c003c003c003c003c003c003c003c003c01ff81ff81ff8

Time cost: 26
[recognize.wasm]: recognize() successful

{"drops":[{"confidence":"0.971166","dropType":"NORMAL_DROP","itemId":"30073","quantity":"1"}],"errors":[],"fingerprint":"aad5b67c52322f6cf40cfb6054ca484a4ea142a0a781363e9726370d2733233c7059467f091e40400a1b2e6f76081f11040712acaf9f190e0f0d0e62685f0c0e","md5":"4f7219cd8a213935e06dcb401519350c","stageId":"main_06-10","warnings":[]}
```