rm $LIB_FUZZING_ENGINE
echo "Compiling aflgo to $LIB_FUZZING_ENGINE ..."
mkdir -p $WORK/afl
cd $WORK/afl #> /dev/null
#$CC $CFLAGS -c $SRC/aflgo/llvm_mode/afl-llvm-rt.o.c
$CXX $CXXFLAGS -std=c++11 -O2 -c $SRC/libfuzzer/afl/afl_driver.cpp -I$SRC/libfuzzer
ar r $LIB_FUZZING_ENGINE $WORK/afl/*.o
cd -
rm -rf $WORK/afl

make -j$(nproc) -s #&> compile.log

./test/fuzzers/build_google_oss_fuzzers.sh
./test/fuzzers/build_seed_corpus.sh
