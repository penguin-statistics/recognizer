FROM ghcr.io/penguin-statistics/recognizer-builder:v3

WORKDIR /build

COPY . .

RUN cd build && emcmake cmake .. && \
    emmake make
