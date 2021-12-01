set(OpenCV_DIR "./opencv")

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
find_package(OpenCV REQUIRED)

# Needed for opencv2/opencv.hpp
include_directories(${OpenCV_INCLUDE_DIRS})

# Needed by opencv.hpp for opencv2/opencv_modules.hpp
# include_directories("${OPENCV_DIR}/build_wasm")

# Needed by opencv_modules.hpp for every module
include_directories("${OPENCV_DIR}/build/include/")

# Needed by json.hpp
include_directories(./)

# Recognize executable
add_executable(recognize ./recognize_v2_debug.cpp)
add_executable(hello_cpp ./hello_cpp.cpp)
add_executable(hello_opencv ./hello_opencv.cpp)

# Link to opencv.js precompiled libraries
# file(GLOB opencv_libs "${OPENCV_DIR}/build_wasm/lib/*.a")
# target_link_libraries(recognize ${opencv_libs})
target_link_libraries(recognize ${OpenCV_LIBS})
target_link_libraries(hello_opencv ${OpenCV_LIBS})

# file(GLOB opencv_3rdlibs "${OPENCV_DIR}/build_wasm/3rdparty/lib/*.a")
# target_link_libraries(recognize ${opencv_3rdlibs})

# There is an issue regarding the order in which libraries
# are passed to the compiler - pass libopencv_core.a last
# https://answers.opencv.org/question/186124/undefined-reference-to-cvsoftdoubleoperator/
# file(GLOB opencv_lib_core "${OPENCV_DIR}/build_wasm/lib/libopencv_core.a")
# target_link_libraries(recognize ${opencv_lib_core})

# Specify linker arguments
# set_target_properties(recognize PROPERTIES LINK_FLAGS "-O3 -s ALLOW_MEMORY_GROWTH=1 -s EXPORTED_FUNCTIONS=['_main','_preload_json','_preload_templ','_recognize','_free_buffer'] -s EXPORTED_RUNTIME_METHODS=['cwrap']")
