#!/bin/bash

usage="Usage: afl-generateDistance.sh PATCH_LOCATION"

rm -rf temp
mkdir temp
export TMP_DIR=$PWD/temp

if [ -f $TMP_DIR/BBtargets.txt ]; then
  rm $TMP_DIR/BBtargets.txt
fi

if [[ $# < 1 ]]; then
    echo "$usage"
    exit 1
fi

length=$#
for (( c=1; c<=length; c++ ))
do  
    target="$1"
    echo $BUGGY_FILE:$target >> $TMP_DIR/BBtargets.txt
    shift
done

export INITIAL_CC=$CC
export INITIAL_CXX=$CXX
export INITIAL_CFLAGS=$CFLAGS
export INITIAL_CXXFLAGS=$CXXFLAGS
export INITIAL_PATH=$PATH

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

# Generate distance
$AFLGO/scripts/genDistance.sh $OUT $TMP_DIR $BINARY

cp $TMP_DIR/distance.cfg.txt $OUT
#rm -rf $TMP_DIR

