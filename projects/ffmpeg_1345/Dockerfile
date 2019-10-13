# Copyright 2016 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
################################################################################

#FROM gcr.io/oss-fuzz-base/base-builder
FROM gaoxiang9430/fix2fit
MAINTAINER even.rouault@spatialys.com
RUN apt-get update && apt-get install -y make autoconf automake libtool build-essential \
    libass-dev libfreetype6-dev libsdl1.2-dev \
    libvdpau-dev libxcb1-dev libxcb-shm0-dev \
    pkg-config texinfo libbz2-dev zlib1g-dev yasm cmake mercurial wget \
    xutils-dev libpciaccess-dev vim

#WORKDIR ffmpeg
COPY build.sh $SRC/
COPY group_seed_corpus.py $SRC/
#COPY ffmpeg_deps $SRC/ffmpeg_deps

RUN wget ftp://ftp.alsa-project.org/pub/lib/alsa-lib-1.1.0.tar.bz2
RUN wget http://www.nasm.us/pub/nasm/releasebuilds/2.13.01/nasm-2.13.01.tar.gz
RUN git clone git://anongit.freedesktop.org/mesa/drm
RUN git clone https://github.com/mstorsjo/fdk-aac.git
ADD https://sourceforge.net/projects/lame/files/latest/download lame.tar.gz
RUN git clone git://anongit.freedesktop.org/xorg/lib/libXext
RUN git clone git://anongit.freedesktop.org/git/xorg/lib/libXfixes
RUN git clone https://github.com/01org/libva
RUN git clone git://people.freedesktop.org/~aplattner/libvdpau
RUN git clone https://chromium.googlesource.com/webm/libvpx
RUN git clone https://github.com/xiph/ogg.git
RUN git clone git://git.xiph.org/opus.git
RUN git clone https://github.com/Distrotech/libtheora.git
RUN git clone git://git.xiph.org/vorbis.git
RUN git clone git://git.videolan.org/git/x264.git
RUN hg clone https://bitbucket.org/multicoreware/x265
ENV FFMPEG_DEPS_PATH=/src/ffmpeg_deps
ENV PATH="$FFMPEG_DEPS_PATH/bin:$PATH"
ENV LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$FFMPEG_DEPS_PATH/lib"

#COPY x264_prev x264

COPY testcase /testcase
COPY ffmpeg $SRC/ffmpeg
COPY build_dependency.sh $SRC/
COPY project_build.sh $SRC/ffmpeg/project_build.sh
COPY project_config.sh $SRC/ffmpeg/project_config.sh
