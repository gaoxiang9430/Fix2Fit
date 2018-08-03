#!/bin/bash -eu
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

# build the project
#./build/autogen.sh
#./configure
make -j$(nproc) all

# build fuzzer(s)
$CXX $CXXFLAGS -Ilibarchive \
    $SRC/libarchive_fuzzer.cc -o $OUT/libarchive_fuzzer \
    -lFuzzingEngine .libs/libarchive.a \
    -Wl,-Bstatic -lbz2 -llzo2  -lxml2 -llzma -lz -lcrypto -llz4 -licuuc \
    -licudata -Wl,-Bdynamic
