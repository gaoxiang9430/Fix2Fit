#/bin/bash
set -x

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
binary_path=`dirname $BINARY`
binary_name=`basename $BINARY`

pushd $SUBJECT_DIR > /dev/null
# original f1x execution to generate target location
rm -rf original.txt
$CONFIG > original.txt 2>&1
echo $BUGGY_FILE
/src/f1x-oss-fuzz/repair/CInterface/profile -f $BUGGY_FILE -t $TESTCASE -T 6600 -d $DRIVER -b $BUILD --output-one-per-loc -a -P $binary_path -N $binary_name -M 16 &>> $SUBJECT_DIR/original.txt

# generate distance file
location=`cat $SUBJECT_DIR/location.txt`
$SCRIPT_DIR/afl-generateDistance.sh $location
export CFLAGS="$CFLAGS -distance=$SUBJECT_DIR/distance/distance.cfg.txt"
export CXXFLAGS="$CXXFLAGS -distance=$SUBJECT_DIR/distance/distance.cfg.txt"

# execute f1x with fuzzing
$CONFIG

cooling_time=120m
export LD_LIBRARY_PATH=$binary_path:$LD_LIBRARY_PATH
export AFL_NO_AFFINITY=" "

mkdir -p $SUBJECT_DIR/patches
f1x_cmd="f1x -f $BUGGY_FILE -t $TESTCASE -T 15000 -d $DRIVER -b $BUILD -a -P $binary_path -N $binary_name -M 16 -o $SUBJECT_DIR/patches"
echo $f1x_cmd > $SCRIPT_DIR/f1xcmd.sh
timeout $TIMEOUT /src/aflgo/afl-fuzz -S ef709ce2 -z exp -c $cooling_time -i ./input -o ./out $CRASHMODE -s part -t 1000 -R $SCRIPT_DIR/f1xcmd.sh ${BINARY}_profile @@

popd > /dev/null

