#!/bin/bash -eu
/*
  This file is part of Fix2Fit.
  Copyright (C) 2016  Xiang Gao, Sergey Mechtaev, Shin Hwei Tan, Abhik Roychoudhury

  f1x is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#########################################################################
# For building the target subject
#########################################################################

# Redefinition to emphasize that we crash the sanitizer upon catching bug
if [ x$SANITIZER = xundefined ] ; then
    export CFLAGS=${CFLAGS/\,vptr/}
    export CXXFLAGS=${CXXFLAGS/\,vptr/}
fi

export CFLAGS="$CFLAGS  -fsanitize-undefined-trap-on-error"
export CXXFLAGS="$CXXFLAGS  -fsanitize-undefined-trap-on-error"

export CFLAGS="$CFLAGS -lrt"
export CXXFLAGS="$CXXFLAGS -lrt -stdlib=libstdc++"

export IS_DOCKER_SINGLE_CORE_MODE=
#set some environmnent variables for aflgo
#export AFL_I_DONT_CARE_ABOUT_MISSING_CRASHES=core_pattern
#export AFL_SKIP_CPUFREQ=
#export AFL_NO_AFFINITY=

export SUBJECT=
export BUGGY_FILE=
export DRIVER=/driver
export BINARY=
export TESTCASE="${SUBJECT}_testcase"

export F1X_PROJECT_CC=/src/aflgo/afl-clang-fast
export F1X_PROJECT_CXX=/src/aflgo/afl-clang-fast++
export CC=f1x-cc
export CXX=f1x-cxx
export LDFLAGS=-lpthread
export LD_LIBRARY_PATH=/usr/local/lib
export PATH=$PATH:/src/scripts

pushd /src/f1x-oss-fuzz/f1x/CInterface/ > /dev/null
  make
#  make f1x-aflgo
popd > /dev/null

mkdir /in
cp /proj4_testcase /in/

cd /src/scripts
if [ x$SANITIZER = xundefined ] ; then
    echo "./executeAFLGO" >> run.sh
elif [ x$SANITIZER = xaddress ] ; then
    echo "./executeAFLGO_address" >> run.sh
fi
#exec "/bin/bash"
/bin/bash run.sh
