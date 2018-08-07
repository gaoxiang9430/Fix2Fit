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

FROM f1x-oss-fuzz

MAINTAINER mmoroz@chromium.org
RUN apt-get update && apt-get install -y make autoconf automake libtool build-essential \
    libass-dev libfreetype6-dev libsdl1.2-dev \
    libvdpau-dev libxcb1-dev libxcb-shm0-dev \
    pkg-config texinfo libbz2-dev zlib1g-dev yasm cmake mercurial wget \
    xutils-dev libpciaccess-dev

#RUN git clone https://github.com/FFmpeg/FFmpeg.git ffmpeg


#RUN git clone https://git.ffmpeg.org/ffmpeg.git 
COPY ffmpeg_189_codes ffmpeg


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
#RUN svn co http://svn.xiph.org/trunk/ogg --trust-server-cert
RUN git clone https://github.com/xiph/ogg.git
RUN git clone git://git.xiph.org/opus.git
#RUN git clone git://git.xiph.org/theora.git
RUN git clone https://github.com/Distrotech/libtheora.git
RUN git clone git://git.xiph.org/vorbis.git
RUN git clone git://git.videolan.org/git/x264.git
RUN hg clone https://bitbucket.org/multicoreware/x265

COPY build.sh group_seed_corpus.py $SRC/

COPY ffmpeg_189 /ffmpeg_189
COPY driver/driver_189 /driver_189

#COPY scpr.c $SRC/ffmpeg/libavcodec/
#COPY ffmpeg_1519 /ffmpeg_1519
#COPY driver_1519 /driver_1519

#COPY scpr.c $SRC/ffmpeg/libavcodec/
#COPY ffmpeg_1478 /ffmpeg_1478
#COPY driver_1478 /driver_1478

#COPY htmlsubtitles.c $SRC/ffmpeg/libavcodec/
#COPY ffmpeg_151 /ffmpeg_151
#COPY driver_151 /driver_151

#COPY flacdsp.c $SRC/ffmpeg/libavcodec/
#COPY ffmpeg_673 /ffmpeg_673
#COPY driver_673 /driver_673

#COPY wavpack.c $SRC/ffmpeg/libavcodec/
#COPY ffmpeg_724 /ffmpeg_724
#COPY driver_724 /driver_724

#COPY h264_cabac.c $SRC/ffmpeg/libavcodec/
#COPY ffmpeg_857 /ffmpeg_857
#COPY driver_857 /driver_857

#COPY movtextdec.c $SRC/ffmpeg/libavcodec/
#COPY ffmpeg_733 /ffmpeg_733
#COPY driver_733 /driver_733

#COPY webp.c $SRC/ffmpeg/libavcodec/
#COPY ffmpeg_1346 /ffmpeg_1346
#COPY driver_1346 /driver_1346

#COPY pixlet.c $SRC/ffmpeg/libavcodec/
#COPY ffmpeg_1489 /ffmpeg_1489
#COPY driver_1489 /driver_1489
