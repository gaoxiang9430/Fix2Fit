#!/bin/bash

export INITIAL_CC=$CC
export INITIAL_CXX=$CXX
export INITIAL_CFLAGS=$CFLAGS
export INITIAL_CXXFLAGS=$CXXFLAGS
export INITIAL_PATH=$PATH

rm -rf temp
mkdir temp
export TMP_DIR=$PWD/temp
echo "src/PJ_stere.c:56" > $TMP_DIR/BBtargets.txt

# Set aflgo-instrumenter
export CC=$AFLGO/afl-clang-fast
export CXX=$AFLGO/afl-clang-fast++
export ADDITIONAL="-targets=$TMP_DIR/BBtargets.txt -outdir=$TMP_DIR -flto -fuse-ld=gold -Wl,-plugin-opt=save-temps"
export CFLAGS="$CFLAGS $ADDITIONAL"
export CXXFLAGS="$CXXFLAGS $ADDITIONAL"

pushd ../$SUBJECT > /dev/null
  ./project_config.sh
  ./project_build.sh
popd > /dev/null

# Clean up
cat $TMP_DIR/BBnames.txt | rev | cut -d: -f2- | rev | sort | uniq > $TMP_DIR/BBnames2.txt && mv $TMP_DIR/BBnames2.txt $TMP_DIR/BBnames.txt
cat $TMP_DIR/BBcalls.txt | sort | uniq > $TMP_DIR/BBcalls2.txt && mv $TMP_DIR/BBcalls2.txt $TMP_DIR/BBcalls.txt

export LDFLAGS=
export CC=$INITIAL_CC
export CXX=$INITIAL_CXX
export CFLAGS=$INITIAL_CFLAGS
export CXXFLAGS=$INITIAL_CXXFLAGS
export PATH=$INITIAL_PATH

