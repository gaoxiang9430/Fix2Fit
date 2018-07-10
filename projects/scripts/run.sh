#/bin/bash

./afl-preprocessing.sh

pushd ../$SUBJECT/ > /dev/null
  #make clean
  #make distclean
  ./project_config.sh
popd > /dev/null

#original f1x execution
rm -rf original.txt
./f1xcmd.sh /src/f1x-oss-fuzz/f1x/CInterface/main | tee original.txt

#generate distance to specific
location=`cat ../$SUBJECT/location.txt`
source ./afl-generateDistance.sh $location
echo CFLAGS="$CFLAGS"
echo CXXFLAGS="$CXXFLAGS"

#execute f1x with fuzzing
pushd ../$SUBJECT/ > /dev/null
  make clean
  make distclean
  ./project_config.sh
popd > /dev/null
rm -rf f1x_with_fuzzing.txt
./f1xcmd.sh /src/f1x-oss-fuzz/f1x/CInterface/main_with_fuzz | tee f1x_with_fuzzing.txt
