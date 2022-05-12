FROM ghcr.io/penguin-statistics/recognizer-builder:v4

ARG VERSION

ENV VERSION=$VERSION

WORKDIR /build/recognizer

COPY . .

RUN cd build && \
    emcmake cmake .. && \
    emmake make && \
    mkdir dist && \
    mv penguin-recognizer* dist/