FROM emscripten/emsdk:1.40.1

WORKDIR /build

# install python and build tools
RUN apt update && apt install -y python python-dev

RUN git clone --depth=1 -b 4.5.5 https://github.com/opencv/opencv.git opencv/sources

# Modify opencv/sources/platforms/js/build_js.py, where in def get_cmake_cmd(self):
# Replace the value before '->' to the value after '->'
# -DWITH_JPEG=OFF               -> -DWITH_JPEG=ON
# -DWITH_PNG=OFF                -> -DWITH_PNG=ON
# -DBUILD_opencv_imgcodecs=OFF  -> -DBUILD_opencv_imgcodecs=ON
# -DWITH_QUIRC=ON               -> -DWITH_QUIRC=OFF
# -DBUILD_ZLIB=ON               -> -DBUILD_ZLIB=OFF
# -DBUILD_opencv_calib3d=ON     -> -DBUILD_opencv_calib3d=OFF
# -DBUILD_opencv_dnn=ON         -> -DBUILD_opencv_dnn=OFF
# -DBUILD_opencv_features2d=ON  -> -DBUILD_opencv_features2d=OFF
# -DBUILD_opencv_flann=ON       -> -DBUILD_opencv_flann=OFF
# -DBUILD_opencv_photo=ON       -> -DBUILD_opencv_photo=OFF
# -DBUILD_EXAMPLES=ON           -> -DBUILD_EXAMPLES=OFF
# -DBUILD_TESTS=ON              -> -DBUILD_TESTS=OFF
# -DBUILD_PERF_TESTS=ON         -> -DBUILD_PERF_TESTS=OFF

RUN cd opencv/sources/platforms/js/ && \
    sed -i 's/-DWITH_JPEG=OFF/-DWITH_JPEG=ON/g' build_js.py && \
    sed -i 's/-DWITH_PNG=OFF/-DWITH_PNG=ON/g' build_js.py && \
    sed -i 's/-DBUILD_opencv_imgcodecs=OFF/-DBUILD_opencv_imgcodecs=ON/g' build_js.py && \
    sed -i 's/-DWITH_QUIRC=ON/-DWITH_QUIRC=OFF/g' build_js.py && \
    sed -i 's/-DBUILD_ZLIB=ON/-DBUILD_ZLIB=OFF/g' build_js.py && \
    sed -i 's/-DBUILD_opencv_calib3d=ON/-DBUILD_opencv_calib3d=OFF/g' build_js.py && \
    sed -i 's/-DBUILD_opencv_dnn=ON/-DBUILD_opencv_dnn=OFF/g' build_js.py && \
    sed -i 's/-DBUILD_opencv_features2d=ON/-DBUILD_opencv_features2d=OFF/g' build_js.py && \
    sed -i 's/-DBUILD_opencv_flann=ON/-DBUILD_opencv_flann=OFF/g' build_js.py && \
    sed -i 's/-DBUILD_opencv_photo=ON/-DBUILD_opencv_photo=OFF/g' build_js.py && \
    sed -i 's/-DBUILD_EXAMPLES=ON/-DBUILD_EXAMPLES=OFF/g' build_js.py && \
    sed -i 's/-DBUILD_TESTS=ON/-DBUILD_TESTS=OFF/g' build_js.py && \
    sed -i 's/-DBUILD_PERF_TESTS=ON/-DBUILD_PERF_TESTS=OFF/g' build_js.py && \
    cd /build/opencv && \
    emcmake python ./sources/platforms/js/build_js.py build_wasm --build_wasm && \
    cd build_wasm && \
    emmake make -j$(nproc)

# Now, check the build result. Those files should exist:
# /build/opencv/build_wasm/lib/libopencv_imgcodecs.a
# /build/opencv/build_wasm/3rdparty/lib/liblibjpeg-turbo.a
# /build/opencv/build_wasm/3rdparty/lib/liblibpng.a
# if not, exit the build with error code 1
RUN [ -f /build/opencv/build_wasm/lib/libopencv_imgcodecs.a ] && \
    [ -f /build/opencv/build_wasm/3rdparty/lib/liblibjpeg-turbo.a ] && \
    [ -f /build/opencv/build_wasm/3rdparty/lib/liblibpng.a ] || exit 1

