# set(OpenCV_DIR "~/opencv-build/")

cmake_minimum_required(VERSION 3.1)
project(penguin-stats-recognize)

# Use C++ 17
set(CMAKE_CXX_STANDARD 17)

# Set Release as default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif(NOT CMAKE_BUILD_TYPE)

# Does not work
# find_package(OpenCV REQUIRED PATHS "${OPENCV_DIR}/build_wasm" NO_DEFAULT_PATH)

# Not working, because the libraries we need are static libraries. See below for more informations.
# find_package(OpenCV REQUIRED)

# Needed for opencv2/opencv.hpp

# Why using OpenCV_INCLUDE_DIRS
# include_directories(${OpenCV_INCLUDE_DIRS})
include_directories("/usr/local/include/opencv4/") 

# Needed by opencv.hpp for opencv2/opencv_modules.hpp
# include_directories("${OPENCV_DIR}/build_wasm")
include_directories("/root/opencv-js/build_wasm") 

# Needed by opencv_modules.hpp for every module

# Needed by json.hpp
include_directories(./)

# Recognize executable
add_executable(recognize ./recognize_v2_wasm_debug.cpp)

# Link to opencv.js precompiled libraries
# file(GLOB opencv_libs "${OPENCV_DIR}/build_wasm/lib/*.a")
# target_link_libraries(recognize ${opencv_libs})
# Don't use $OpenCV_LIBS. They're probably dynamic libraries.
# Use the following instead. They're static libraries, built by OpenCV sources, with `opencv/platforms/js/build_js.py build_js`.
file(GLOB OpenCV_JS_LIBS "/root/opencv-js/build_wasm/lib/*.a")
target_link_libraries(recognize ${OpenCV_JS_LIBS})

file(GLOB OpenCV_JS_3RD_LIBS "/root/opencv-js/build_wasm/3rdparty/lib/*.a")
target_link_libraries(recognize ${OpenCV_JS_3RD_LIBS})

# There is an issue regarding the order in which libraries
# are passed to the compiler - pass libopencv_core.a last
# https://answers.opencv.org/question/186124/undefined-reference-to-cvsoftdoubleoperator/
file(GLOB OpenCV_JS_LIB_CORE "/root/opencv-js/build_wasm/lib/libopencv_core.a")
target_link_libraries(recognize ${OpenCV_JS_LIB_CORE})

# Specify linker arguments
 set_target_properties(recognize PROPERTIES LINK_FLAGS "-s ALLOW_MEMORY_GROWTH=1 -s EXPORTED_FUNCTIONS=['_preload_json','_preload_templ','_recognize'] -s EXPORTED_RUNTIME_METHODS=['cwrap'] -g4 --source-map-base http://172.24.252.10:5500/build/")
# set_target_properties(recognize PROPERTIES LINK_FLAGS "-s LLD_REPORT_UNDEFINED --no-entry")