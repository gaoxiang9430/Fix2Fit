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

make -j$(nproc) -s &> compile.log

make install

fuzzers=$(find $SRC/yara/tests/oss-fuzz/ -name "*.cc")
for f in $fuzzers; do
  fuzzer_name=$(basename -s ".cc" $f)
  echo "Building $fuzzer_name"
  $CXX $CXXFLAGS -std=c++11 -I. $f -o $OUT/$fuzzer_name \
    ./libyara/.libs/libyara.a \
    $LIB_FUZZING_ENGINE
  if [ -d "$SRC/yara/tests/oss-fuzz/${fuzzer_name}_corpus" ]; then
    zip -j $OUT/${fuzzer_name}_seed_corpus.zip $SRC/yara/tests/oss-fuzz/${fuzzer_name}_corpus/*
  fi
done

find $SRC/yara/tests/oss-fuzz -name \*.dict -exec cp {} $OUT \;
find $SRC/yara/tests/oss-fuzz -name \*.options -exec cp {} $OUT \;