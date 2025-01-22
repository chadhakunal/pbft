# First Stage: Build
FROM debian:buster-slim AS client_build

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

RUN git clone --recurse-submodules -b v1.56.0 https://github.com/grpc/grpc
RUN cd grpc \
    && mkdir -p cmake/build \
    && cd cmake/build \
    && cmake ../.. \
    && make -j$(nproc) \
    && make install
RUN rm -rf grpc

RUN apt-get install -y libboost-all-dev sudo
RUN useradd -ms /bin/bash app && \
    usermod -aG sudo app
RUN echo "app:password" | chpasswd

# Install MCL
# RUN apt-get install -y libgmp-dev
# RUN git clone https://github.com/herumi/mcl
# RUN cd mcl \
#     && cmake -DCMAKE_CXX_COMPILER=clang++ . \
#     && make -j$(nproc) \
#     && make install
# RUN rm -rf mcl

# RUN rm -rf /var/lib/apt/lists/* /grpc

USER app

WORKDIR /home/app

RUN mkdir /home/app/protos
RUN mkdir /home/app/client

COPY client /home/app/client/
COPY protos/linear-pbft.proto /home/app/protos/linear-pbft.proto

RUN mkdir /home/app/client/build

RUN cd /home/app/client/build && cmake .. && make -j4

# Second stage: Runtime stage
FROM debian:buster-slim

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Etc/UTC

RUN apt-get update && \
    apt-get install -y libboost-all-dev libgmp-dev sudo 
    # && \
    # rm -rf /var/lib/apt/lists/*

RUN useradd -ms /bin/bash app
RUN echo "app ALL=(ALL) NOPASSWD: ALL" > /etc/sudoers.d/app
USER app

COPY --from=client_build /home/app/client/build /home/app/build
# COPY --from=client_build /usr/local/lib/libmcl.so.1 /usr/local/lib/libmcl.so.1

# ENV LD_LIBRARY_PATH=/usr/local/lib

WORKDIR /home/app/build
