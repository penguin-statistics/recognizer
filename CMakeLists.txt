cmake_minimum_required(VERSION 3.1)
project(penguin-recognizer)

# Use C++ 17
set(CMAKE_CXX_STANDARD 17)

# Set Release as default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif(NOT CMAKE_BUILD_TYPE)

# change the path to opencv if necessary
if(NOT OPENCV_DIR)
    set(OPENCV_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../opencv")
endif(NOT OPENCV_DIR)

# Does not work
# find_package(OpenCV REQUIRED PATHS "${OPENCV_DIR}/build_wasm" NO_DEFAULT_PATH)

# Needed for opencv2/opencv.hpp
include_directories("${OPENCV_DIR}/sources/include")

# Needed by opencv.hpp for opencv2/opencv_modules.hpp
include_directories("${OPENCV_DIR}/build_wasm")

# Needed by opencv_modules.hpp for every module
file(GLOB opencv_include_modules "${OPENCV_DIR}/sources/modules/*/include")
include_directories(${opencv_include_modules})

# Needed by sources
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src")

# Needed by 3rdparty
file(GLOB 3rdparty_include_modules "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/*/include")
include_directories(${3rdparty_include_modules})

# Recognize executable
add_executable(penguin-recognizer "${CMAKE_CURRENT_SOURCE_DIR}/src/recognizer_wasm.cpp")

# Link to opencv.js precompiled libraries
file(GLOB opencv_libs "${OPENCV_DIR}/build_wasm/lib/*.a")
target_link_libraries(penguin-recognizer ${opencv_libs})

file(GLOB opencv_3rdlibs "${OPENCV_DIR}/build_wasm/3rdparty/lib/*.a")
target_link_libraries(penguin-recognizer ${opencv_3rdlibs})

# Specify linker arguments
set_target_properties(penguin-recognizer PROPERTIES LINK_FLAGS "-O3 -s ALLOW_MEMORY_GROWTH=1 -s EXPORTED_FUNCTIONS=['_get_info','_load_server','_load_json','_load_templ','_recognize'] -s EXPORTED_RUNTIME_METHODS=['cwrap']")