#!/bin/bash -eu
# Copyright 2017 Google Inc.
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

pushd $SRC > /dev/null

export INITIAL_CC=$CC
export INITIAL_CXX=$CXX
export INITIAL_CFLAGS=$CFLAGS
export INITIAL_CXXFLAGS=$CXXFLAGS

# Build LLVM 3.8.1 for building f1x
CC=gcc
CXX=g++
CFLAGS=
CXXFLAGS=

if [ -x "llvm-3.8.1/install/bin/clang" ] ; then
    echo LLVM 3.8.1 built
    exit 1
fi

mkdir -p llvm-3.8.1
pushd llvm-3.8.1 > /dev/null
if [ ! -e llvm-3.8.1.src.tar.xz ] ; then
	wget http://releases.llvm.org/3.8.1/llvm-3.8.1.src.tar.xz
	tar xf llvm-3.8.1.src.tar.xz
	mv llvm-3.8.1.src src
	rm -f llvm-3.8.1.src.tar.xz
fi
if [ ! -e cfe-3.8.1.src.tar.xz ] ; then
	wget http://releases.llvm.org/3.8.1/cfe-3.8.1.src.tar.xz
	tar xf cfe-3.8.1.src.tar.xz
	mv cfe-3.8.1.src src/tools/clang
	rm -f cfe-3.8.1.src.tar.xz
fi
if [ ! -e compiler-rt-3.8.1.src.tar.xz ] ; then
	wget http://releases.llvm.org/3.8.1/compiler-rt-3.8.1.src.tar.xz
	tar xf compiler-rt-3.8.1.src.tar.xz
	mv compiler-rt-3.8.1.src src/projects/compiler-rt
	rm -f compierl-rt-3.8.1.src.tar.xz
fi
mkdir -p install
LLVM_INSTALL_DIR=$PWD/install
mkdir -p build
pushd build > /dev/null
cmake -G "Unix Makefiles" -DCMAKE_INSTALL_PREFIX=$LLVM_INSTALL_DIR -DCMAKE_BUILD_TYPE=Release -DLLVM_ENABLE_ASSERTIONS=Off ../src
make -j$(nproc) install
popd > /dev/null
popd > /dev/null

# Build f1x
unzip f1x.zip
pushd f1x > /dev/null
mkdir -p build
pushd build > /dev/null
cmake -DF1X_LLVM=/src/llvm-3.8.1/install ..
make
popd > /dev/null
popd > /dev/null

export CC=$INITIAL_CC
export CXX=$INITIAL_CXX
export CFLAGS=$INITIAL_CFLAGS
export CXXFLAGS=$INITIAL_CXXFLAGS

export PATH=/src/f1x/build/tools:$PATH

# Build AFL
# This is where we want to replace OSS-Fuzz afl with our specialized afl engine
# This code is based on infra/base-images/base-builder/compile_afl
echo -n "Compiling afl to $LIB_FUZZING_ENGINE ..."

# First, we remove existing fuzzing engine and afl
rm -f $LIB_FUZZING_ENGINE
rm -rf afl

# This should download our new afl engine
git clone https://github.com/mirrorer/afl.git

# afl needs its special coverage flags
export COVERAGE_FLAGS="-fsanitize-coverage=trace-pc-guard"

INITIAL_CC=$CC
INITIAL_CXX=$CXX
CC=clang
CXX=clang++
mkdir -p $WORK/afl
pushd $WORK/afl > /dev/null
# Add -Wno-pointer-sign to silence warning (AFL is compiled this way).
$CC $CFLAGS -Wno-pointer-sign -c $SRC/afl/llvm_mode/afl-llvm-rt.o.c
$CXX $CXXFLAGS -std=c++11 -O2 -c $SRC/libfuzzer/afl/*.cpp -I$SRC/libfuzzer
ar r $LIB_FUZZING_ENGINE $WORK/afl/*.o
popd > /dev/null
rm -rf $WORK/afl

# Build and copy afl tools necessary for fuzzing.
pushd $SRC/afl > /dev/null

# Unset CFLAGS and CXXFLAGS while building AFL since we don't want to slow it
# down with sanitizers.
INITIAL_CXXFLAGS=$CXXFLAGS
INITIAL_CFLAGS=$CFLAGS
unset CXXFLAGS
unset CFLAGS
make clean && make
CFLAGS=$INITIAL_CFLAGS
CXXFLAGS=$INITIAL_CXXFLAGS
CC=$INITIAL_CC
CXX=$INITIAL_CXX

find . -name 'afl-*' -executable -type f | xargs cp -t $OUT
popd > /dev/null

echo " done compiling afl."

popd > /dev/null

#########################################################################
# For building the target subject
#########################################################################

# Redefinition to emphasize that we crash the sanitizer upon catching bug
export CFLAGS="$CFLAGS  -fsanitize-undefined-trap-on-error"
export CXXFLAGS="$CXXFLAGS  -fsanitize-undefined-trap-on-error"

# Note that we need to use libstdc++ for afl
export CXXFLAGS=${CXXFLAGS/libc++/libstdc++}

# Reset these two
export F1X_PROJECT_CFLAGS=
export F1X_PROJECT_CXXFLAGS=

# Set the compilers
export F1X_PROJECT_CC=$SRC/afl/afl-clang
export F1X_PROJECT_CXX=$SRC/afl/afl-clang++
export CC=f1x-cc
export CXX=f1x-cxx

# Wireshark build.sh script inspired from projects/ffmpeg/build.sh

FUZZ_DISSECTORS="ip \
  udp"

export WIRESHARK_INSTALL_PATH="$WORK/install"
mkdir -p "$WIRESHARK_INSTALL_PATH"

# compile static version of libs
# XXX, with static wireshark linking each fuzzer binary is ~240 MB (just libwireshark.a is 423 MBs).
# XXX, wireshark is not ready for including static plugins into binaries.
CONFOPTS="--disable-shared --enable-static --without-plugins"

# disable optional dependencies
CONFOPTS="$CONFOPTS --without-pcap --without-ssl --without-gnutls"

# need only libs, disable programs
CONFOPTS="$CONFOPTS --disable-wireshark --disable-tshark --disable-sharkd \
             --disable-dumpcap --disable-capinfos --disable-captype --disable-randpkt --disable-dftest \
             --disable-editcap --disable-mergecap --disable-reordercap --disable-text2pcap \
             --without-extcap \
         "

./autogen.sh
./configure --prefix="$WIRESHARK_INSTALL_PATH" $CONFOPTS --disable-warnings-as-errors

make "-j$(nproc)"
make install

WIRESHARK_FUZZERS_COMMON_FLAGS="-lFuzzingEngine \
    -L"$WIRESHARK_INSTALL_PATH/lib" -lwireshark -lwiretap -lwsutil \
    -Wl,-Bstatic `pkg-config --libs glib-2.0` -pthread -lpcre -lgcrypt -lgpg-error -lz -Wl,-Bdynamic"

for dissector in $FUZZ_DISSECTORS; do
  fuzzer_name=fuzzshark_dissector_${dissector}

  # -I$SRC/wireshark is correct, wireshark don't install header files.
  $CC $CFLAGS -I $SRC/wireshark/ `pkg-config --cflags glib-2.0` \
      $SRC/wireshark/tools/oss-fuzzshark.c \
      -c -o $WORK/${fuzzer_name}.o \
      -DFUZZ_DISSECTOR_TARGET=\"$dissector\"

  $CXX $CXXFLAGS $WORK/${fuzzer_name}.o \
      -o $OUT/${fuzzer_name} \
      ${WIRESHARK_FUZZERS_COMMON_FLAGS}

  echo -en "[libfuzzer]\nmax_len = 1024\n" > $OUT/${fuzzer_name}.options
done

#exec "/bin/bash"
