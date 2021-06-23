# FROM FROM ubuntu:20.04
FROM gcr.io/oss-fuzz-base/base-builder
MAINTAINER Gao Xiang <gaoxiang@comp.nus.edu.sg>
ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get upgrade -y && apt-get autoremove -y

# install experiment dependencies
RUN apt-get install -y  \
    autopoint \
    automake \
    bison \
    flex \
    gettext \
    gperf \
    libass-dev \
    libfreetype6 \
    libfreetype6-dev \
    libjpeg-dev \
    libtool \
    libxml2-dev \
    liblzma-dev \
    nasm \
    pkg-config \
    texinfo \
    yasm \
    xutils-dev \
    libpciaccess-dev \
    libpython-dev \
    libpython3-dev \
    libx11-dev \
    libxcb-xfixes0-dev \
    libxcb1-dev \
    libxcb-shm0-dev \
    libsdl1.2-dev  \
    libvdpau-dev \
    libnuma-dev \
    python3-pip \
    vim

RUN apt-get install -y build-essential cmake zlib1g-dev libtinfo-dev python unzip
RUN apt-get install -y libboost-filesystem-dev libboost-program-options-dev libboost-log-dev wget
RUN pip3 install networkx==2.4


WORKDIR $SRC

RUN mkdir f1x-oss-fuzz
RUN mkdir f1x-oss-fuzz/repair/

#ADD docs       f1x-oss-fuzz/docs
ADD infra/repair.zip       f1x-oss-fuzz/
ADD aflgo                  $SRC/aflgo
ADD scripts/build_aflgo.sh /src/build_aflgo.sh
ADD scripts/driver         /driver
ADD projects/scripts       $SRC/scripts
ADD scripts/build.sh       $SRC/build.sh

RUN unzip f1x-oss-fuzz/repair.zip -d f1x-oss-fuzz/repair/
RUN rm -rf f1x-oss-fuzz/repair.zip
RUN cp f1x-oss-fuzz/repair/lib/* /usr/lib/

ENV C_INCLUDE_PATH="$C_INCLUDE_PATH:/src/f1x-oss-fuzz/repair/include"
ENV CPLUS_INCLUDE_PATH="$CPLUS_INCLUDE_PATH:/src/f1x-oss-fuzz/repair/include"
ENV PATH="/src/f1x-oss-fuzz/repair/bin/:${PATH}"

RUN chmod u+x $SRC/build_aflgo.sh
RUN $SRC/build_aflgo.sh
ENV PATH="$SRC/llvm-4.0.0/install/bin/:${PATH}"
ENV AFLGO="$SRC/aflgo"

RUN git config --global user.email "gaoxiang9430@gmail.com"
RUN git config --global user.name "Gao Xiang"

ENV IS_DOCKER_SINGLE_CORE_MODE=

# configuration for F1X
ENV F1X_PROJECT_CC="/src/aflgo/afl-clang-fast"
ENV F1X_PROJECT_CXX="/src/aflgo/afl-clang-fast++"
ENV CC="f1x-cc"
ENV CXX="f1x-cxx"
ENV LDFLAGS="-lpthread"
ENV PATH="$PATH:/src/scripts"

WORKDIR /src/f1x-oss-fuzz/repair/CInterface
RUN make

RUN mkdir /benchmark
WORKDIR /benchmark
RUN git clone https://github.com/OSGeo/proj.4 proj4
WORKDIR /benchmark/proj4/
RUN git checkout fe843d11b4b5f58e09e896848ce4354170bf14bc
ADD demo/proj4/config.sh /benchmark/proj4/config.sh
ADD demo/proj4/build.sh /benchmark/proj4/build.sh
ADD demo/proj4/input /benchmark/proj4/input
