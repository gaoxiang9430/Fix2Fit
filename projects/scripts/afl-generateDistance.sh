#!/bin/bash

usage="Usage: afl-generateDistance.sh PATCH_LOCATION"

export TMP_DIR=$PWD/distance
rm -rf $TMP_DIR
mkdir $TMP_DIR

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

# Set aflgo-instrumenter
export CC=$AFLGO/afl-clang-fast
export CXX=$AFLGO/afl-clang-fast++
export ADDITIONAL="-targets=$TMP_DIR/BBtargets.txt -outdir=$TMP_DIR -flto -fuse-ld=gold -Wl,-plugin-opt=save-temps"
export CFLAGS="$CFLAGS $ADDITIONAL"
export CXXFLAGS="$CXXFLAGS $ADDITIONAL"

$CONFIG > /dev/null 2>&1; $BUILD

# Clean up
cat $TMP_DIR/BBnames.txt | rev | cut -d: -f2- | rev | sort | uniq > $TMP_DIR/BBnames2.txt && mv $TMP_DIR/BBnames2.txt $TMP_DIR/BBnames.txt
cat $TMP_DIR/BBcalls.txt | sort | uniq > $TMP_DIR/BBcalls2.txt && mv $TMP_DIR/BBcalls2.txt $TMP_DIR/BBcalls.txt

# Generate distance
$AFLGO/scripts/genDistance.sh `dirname $BINARY` $TMP_DIR `basename $BINARY`

if ! test -f "$TMP_DIR/distance.cfg.txt"; then
    touch $TMP_DIR/distance.cfg.txt
fi
