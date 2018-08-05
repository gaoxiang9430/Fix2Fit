#/bin/bash

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
pushd ../$SUBJECT/ > /dev/null
  #make clean
  #make distclean
  ./project_config.sh
popd > /dev/null

#original f1x execution
rm -rf original.txt
pushd ../$SUBJECT/ > /dev/null
  /src/f1x-oss-fuzz/f1x/CInterface/main -f $BUGGY_FILE -t $TESTCASE -T 1000 -d $DRIVER -b ./project_build.sh --output-one-per-loc -a -P /out -N $BINARY &> $SCRIPT_DIR/original.txt
  #generate distance to specific
  make clean
  make distclean
  ./project_config.sh
popd > /dev/null

location=`cat ../$SUBJECT/location.txt`
./afl-generateDistance.sh $location

export CFLAGS="$CFLAGS -distance=$OUT/distance.cfg.txt"
export CXXFLAGS="$CXXFLAGS -distance=$OUT/distance.cfg.txt"

#execute f1x with fuzzing
pushd ../$SUBJECT/ > /dev/null
  make clean
  make distclean
  ./project_config.sh
popd > /dev/null
rm -rf f1x_with_fuzzing.txt

./executeAFLGO_address
