cmake_minimum_required(VERSION 3.1)
project(penguin-stats-recognize)

# Use C++ 17
set(CMAKE_CXX_STANDARD 17)

# Set Release as default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif(NOT CMAKE_BUILD_TYPE)

if(NOT OpenCV_DIR)
    set(OpenCV_DIR "opencv_install_wasm")
endif(NOT OpenCV_DIR)

# Does not work
# find_package(OpenCV REQUIRED PATHS "${OPENCV_DIR}/build_wasm" NO_DEFAULT_PATH)

# OpenCV Headers
include_directories(${OpenCV_DIR}/include/opencv4)

# json.hpp & md5.hpp
include_directories(include)

# Recognize executable
add_executable(recognize ./recognize_v2_wasm_debug.cpp)

# Link to opencv.js precompiled libraries
file(GLOB OpenCV_LIBS ${OpenCV_DIR}/lib/*.a)
target_link_libraries(recognize ${OpenCV_LIBS})

file(GLOB OpenCV_3RD_LIBS ${OpenCV_DIR}/lib/opencv4/3rdparty/*.a)
target_link_libraries(recognize ${OpenCV_3RD_LIBS})

# Specify linker arguments
set_target_properties(recognize PROPERTIES LINK_FLAGS "-s ALLOW_MEMORY_GROWTH=1 -s EXPORTED_FUNCTIONS=['_preload_json','_preload_templ','_recognize'] -s EXPORTED_RUNTIME_METHODS=['cwrap'] ")
