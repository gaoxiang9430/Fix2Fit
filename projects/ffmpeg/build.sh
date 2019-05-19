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

#########################################################################
# For building the target subject
#########################################################################

# Redefinition to emphasize that we crash the sanitizer upon catching bug
if [ x$SANITIZER = xundefined ] ; then
    export CFLAGS=${CFLAGS/\,vptr/}
    export CXXFLAGS=${CXXFLAGS/\,vptr/}
fi

# Disable UBSan vptr since several targets built with -fno-rtti.
export CFLAGS="$CFLAGS  -fsanitize-undefined-trap-on-error -fno-sanitize=vptr"
export CXXFLAGS="$CXXFLAGS  -fsanitize-undefined-trap-on-error -fno-sanitize=vptr"

export FFMPEG_DEPS_PATH=/src/ffmpeg_deps
export PATH="$FFMPEG_DEPS_PATH/bin:$PATH"

#export CFLAGS="$CFLAGS -lrt"
#export CXXFLAGS="$CXXFLAGS -lrt -stdlib=libstdc++"

export IS_DOCKER_SINGLE_CORE_MODE=
#set some environmnent variables for aflgo
#export AFL_I_DONT_CARE_ABOUT_MISSING_CRASHES=core_pattern
#export AFL_SKIP_CPUFREQ=
#export AFL_NO_AFFINITY=

export SUBJECT=ffmpeg
export BUGGY_FILE
export DRIVER=/driver
export BINARY=
export TESTCASE="ffmpeg_testcase"

export F1X_PROJECT_CC=/src/aflgo/afl-clang-fast
export F1X_PROJECT_CXX=/src/aflgo/afl-clang-fast++
export CC=f1x-cc
export CXX=f1x-cxx
export LDFLAGS=-lpthread
export LD_LIBRARY_PATH="/usr/local/lib:$FFMPEG_DEPS_PATH/lib"
export PATH=$PATH:/src/scripts

export PS1='${debian_chroot:+($debian_chroot)}SUBJECT_TAG~\h:\w\$ '

pushd /src/f1x-oss-fuzz/f1x/CInterface/ > /dev/null
  make
#  make f1x-aflgo
popd > /dev/null

mkdir /in
cp /ffmpeg_testcase /in/
touch /out/distance.cfg.txt

cd /src/scripts
if [ x$SANITIZER = xundefined ] ; then
    echo "./executeAFLGO" >> run.sh
elif [ x$SANITIZER = xaddress ] ; then
    echo "./executeAFLGO_address" >> run.sh
fi
/bin/bash run.sh
#exec "/bin/bash"
