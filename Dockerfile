FROM ghcr.io/penguin-statistics/recognizer-builder:v3

WORKDIR /build

COPY . .

RUN emcmake cmake .. && \
    emmake make
