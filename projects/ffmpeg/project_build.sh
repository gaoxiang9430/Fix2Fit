#!/bin/bash -eux
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

rm -f $LIB_FUZZING_ENGINE
echo "Compiling aflgo to $LIB_FUZZING_ENGINE ..." 
mkdir -p $WORK/afl
pushd $WORK/afl > /dev/null
#$CC $CFLAGS -c $SRC/aflgo/llvm_mode/afl-llvm-rt.o.c 
$CXX $CXXFLAGS -std=c++11 -O2 -c $SRC/libfuzzer/afl/afl_driver.cpp -I$SRC/libfuzzer
ar r $LIB_FUZZING_ENGINE $WORK/afl/*.o
popd > /dev/null
rm -rf $WORK/afl

# Build ffmpeg.
cd $SRC/ffmpeg
#PKG_CONFIG_PATH="$FFMPEG_DEPS_PATH/lib/pkgconfig" ./configure \
#    --cc=$CC --cxx=$CXX --ld="$CXX $CXXFLAGS -std=c++11" \
#    --extra-cflags="-I$FFMPEG_DEPS_PATH/include" \
#    --extra-ldflags="-L$FFMPEG_DEPS_PATH/lib" \
#    --prefix="$FFMPEG_DEPS_PATH" \
#    --pkg-config-flags="--static" \
#    --libfuzzer=-lFuzzingEngine \
#    #--optflags=-O1 \
#    #--enable-gpl \
#    --enable-libass \
#    --enable-libfdk-aac \
#    --enable-libfreetype \
#    --enable-libmp3lame \
#    --enable-libopus \
#    --enable-libtheora \
#    --enable-libvorbis \
#    --enable-libvpx \
#    --enable-libx264 \
#    --enable-libx265 \
#    --enable-nonfree \
#    --disable-shared
#make clean
make -j$(nproc) install

# Download test sampes, will be used as seed corpus.
# DISABLED.
# TODO: implement a better way to maintain a minimized seed corpora
# for all targets. As of 2017-05-04 now the combined size of corpora
# is too big for ClusterFuzz (over 10Gb compressed data).
# export TEST_SAMPLES_PATH=$SRC/ffmpeg/fate-suite/
# make fate-rsync SAMPLES=$TEST_SAMPLES_PATH

# Build the fuzzers.
cd $SRC/ffmpeg

FUZZ_TARGET_SOURCE=$SRC/ffmpeg/tools/target_dec_fuzzer.c

export TEMP_VAR_CODEC="AV_CODEC_ID_H264"
export TEMP_VAR_CODEC_TYPE="VIDEO"

# Build fuzzers for decoders.
CONDITIONALS=`grep 'DECODER 1$' config.h | sed 's/#define CONFIG_\(.*\)_DECODER 1/\1/'`
for c in $CONDITIONALS ; do
  fuzzer_name=ffmpeg_AV_CODEC_ID_${c}_fuzzer
  symbol=`git grep 'REGISTER_[A-Z]*DEC[A-Z ]*('"$c"' *,' libavcodec/allcodecs.c | sed 's/.*, *\([^) ]*\)).*/\1/'`
  echo -en "[libfuzzer]\nmax_len = 1000000\n" > $OUT/${fuzzer_name}.options
  make tools/target_dec_${symbol}_fuzzer
  mv tools/target_dec_${symbol}_fuzzer $OUT/${fuzzer_name}
done
