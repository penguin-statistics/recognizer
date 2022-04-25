FROM ghcr.io/penguin-statistics/recognizer-builder:v3

WORKDIR /build

COPY . .

RUN mkdir build && cd build && \
    emcmake cmake .. && \
    emmake make
