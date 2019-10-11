#!/bin/bash -eu

# Redefinition to emphasize that we crash the sanitizer upon catching bug
if [ x$SANITIZER = xundefined ] ; then
    export CFLAGS=${CFLAGS/\,vptr/}
    export CXXFLAGS=${CXXFLAGS/\,vptr/}
fi

export CFLAGS="$CFLAGS  -fsanitize-undefined-trap-on-error -fno-sanitize=vptr -lrt"
export CXXFLAGS="$CXXFLAGS  -fsanitize-undefined-trap-on-error -fno-sanitize=vptr -lrt -stdlib=libstdc++"

# set envionment variable for AFLGo
export IS_DOCKER_SINGLE_CORE_MODE=

# configuration for F1X
export F1X_PROJECT_CC=/src/aflgo/afl-clang-fast
export F1X_PROJECT_CXX=/src/aflgo/afl-clang-fast++
export CC=f1x-cc
export CXX=f1x-cxx
export LDFLAGS=-lpthread
export LD_LIBRARY_PATH=/usr/local/lib
export PATH=$PATH:/src/scripts
pushd /src/f1x-oss-fuzz/repair/CInterface/ > /dev/null
  make
popd > /dev/null

# set exploit as seed for fuzzing
mkdir /in
cp /testcase /in/

# project specific configuration
export SUBJECT=proj4
export BUGGY_FILE=src/pj_init.c:368-394
export BINARY=standard_fuzzer

export DRIVER=/driver
export TESTCASE="testcase"

cd /src/scripts
if [ x$SANITIZER = xaddress ] ; then
    MEMMODE="-m none"
fi

exec /bin/bash

