# First Stage: Build
FROM debian:buster-slim AS server_build

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Etc/UTC

RUN apt-get update && apt-get install -y \
    build-essential \
    autoconf \
    libtool \
    pkg-config \
    git \
    cmake \
    wget \
    unzip \
    libssl-dev \
    clang

# Install gRPC
RUN git clone --recurse-submodules -b v1.56.0 https://github.com/grpc/grpc
RUN cd grpc \
    && mkdir -p cmake/build \
    && cd cmake/build \
    && cmake ../.. \
    && make -j$(nproc) \
    && make install
RUN rm -rf grpc

# Install MCL
# RUN apt-get install -y libgmp-dev
# RUN git clone https://github.com/herumi/mcl
# RUN cd mcl \
#     && cmake -DCMAKE_CXX_COMPILER=clang++ -DMCL_USE_XBYAK=ON . \
#     && make -j$(nproc) \
#     && make install
# RUN rm -rf mcl

RUN apt-get install -y libcrypto++-dev

# Clean up package list to reduce image size
RUN rm -rf /var/lib/apt/lists/*

# Create user and set working directory
RUN useradd -ms /bin/bash app
USER app

WORKDIR /home/app

# Create necessary directories
RUN mkdir /home/app/protos
RUN mkdir /home/app/server

# Copy source files
COPY server /home/app/server/
COPY protos/linear-pbft.proto /home/app/protos/linear-pbft.proto

RUN mkdir /home/app/server/build

# Build project
RUN cd /home/app/server/build && cmake .. && make -j4

# Second stage: Runtime stage
FROM debian:buster-slim

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Etc/UTC

RUN apt-get update && apt-get install -y \
    libcrypto++-dev \
    libssl-dev && \
    rm -rf /var/lib/apt/lists/*

ENV LD_LIBRARY_PATH=/usr/local/lib:/usr/lib/x86_64-linux-gnu:$LD_LIBRARY_PATH

RUN useradd -ms /bin/bash app
USER app

COPY --from=server_build /home/app/server/build /home/app/build
WORKDIR /home/app/build
