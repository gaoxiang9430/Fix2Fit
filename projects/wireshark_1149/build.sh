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

#########################################################################
# For building the target subject
#########################################################################

# Redefinition to emphasize that we crash the sanitizer upon catching bug
# Redefinition to emphasize that we crash the sanitizer upon catching bug
if [ x$SANITIZER = xundefined ] ; then
    export CFLAGS=${CFLAGS/\,vptr/}
    export CXXFLAGS=${CXXFLAGS/\,vptr/}
    export CFLAGS="$CFLAGS  -fsanitize-undefined-trap-on-error"
    export CXXFLAGS="$CXXFLAGS  -fsanitize-undefined-trap-on-error"
fi
export CFLAGS
export CXXFLAGS

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
pushd wireshark > /dev/null

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

popd > /dev/null

exec "/bin/bash"
