#!/bin/bash

usage="Usage: source afl-generateDistance.sh PATCH_LOCATION"

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

INSTALL_CFLAGS=$CFLAGS
INSTALL_CXXFLAGS=$CXXFLAGS

#echo "src/PJ_stere.c:56" > $TMP_DIR/BBtargets.txt

# Generate distance
$AFLGO/scripts/genDistance.sh $OUT $TMP_DIR standard_fuzzer

cp $TMP_DIR/distance.cfg.txt $OUT
rm -rf $TMP_DIR

export CFLAGS="$CFLAGS -distance=$OUT/distance.cfg.txt"
export CXXFLAGS="$CXXFLAGS -distance=$OUT/distance.cfg.txt"
