FROM ghcr.io/penguin-statistics/recognizer-builder:v3

WORKDIR /build/recognizer

COPY . .

RUN cd build && emcmake cmake .. && \
    emmake make
