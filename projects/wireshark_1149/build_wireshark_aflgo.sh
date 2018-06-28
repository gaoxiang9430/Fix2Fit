#This is program is used to build wireshark under aflgo
#Author: gaoxiang Email: gaoxiang@comp.nus.edu.sg
#Time: ..2018
#!/bin/bash

export INITIAL_CC=$CC
export INITIAL_CXX=$CXX
export INITIAL_CFLAGS=$CFLAGS
export INITIAL_CXXFLAGS=$CXXFLAGS
export INITIAL_PATH=$PATH

mkdir temp
export TMP_DIR=$PWD/temp

#$TMP_DIR/BBtargets.txt  #TODO:set target here
echo "epan/dissectors/packet-ositp.c:539" > $TMP_DIR/BBtargets.txt

# Print extracted targets. 
echo "Targets:"
cat $TMP_DIR/BBtargets.txt

export CC=$AFLGO/afl-clang-fast
export CXX=$AFLGO/afl-clang-fast++

if [ x$SANITIZER = xundefined ] ; then
    export CFLAGS=${CFLAGS/\,vptr/}
    export CXXFLAGS=${CXXFLAGS/\,vptr/}
    export CFLAGS="$CFLAGS -fsanitize-undefined-trap-on-error"
    export CXXFLAGS="$CXXFLAGS -stdlib=libstdc++ -fsanitize-undefined-trap-on-error"
fi

export ADDITIONAL="-targets=$TMP_DIR/BBtargets.txt -outdir=$TMP_DIR -flto -fuse-ld=gold -Wl,-plugin-opt=save-temps"
export CFLAGS="$CFLAGS $ADDITIONAL"
export CXXFLAGS="$CXXFLAGS $ADDITIONAL"

FUZZ_DISSECTORS="ip \
  udp"

export WIRESHARK_INSTALL_PATH="$WORK/install"
mkdir -p "$WIRESHARK_INSTALL_PATH"

# compile static version of libs
# XXX, with static wireshark linking each fuzzer binary is ~240 MB (just libwireshark.a is 423 MBs).
# XXX, wireshark is not ready for including static plugins into binaries.
CONFOPTS="--disable-shared --enable-static --without-plugins"
# disable optional dependencies
CONFOPTS="$CONFOPTS --without-pcap --without-ssl --without-gnutls"

# need only libs, disable programs
CONFOPTS="$CONFOPTS --disable-wireshark --disable-tshark --disable-sharkd \
             --disable-dumpcap --disable-capinfos --disable-captype --disable-randpkt --disable-dftest \
             --disable-editcap --disable-mergecap --disable-reordercap --disable-text2pcap \
             --without-extcap \
         "

pushd wireshark >/dev/null
./autogen.sh
./configure --prefix="$WIRESHARK_INSTALL_PATH" $CONFOPTS --disable-warnings-as-errors

make clean -s
make "-j$(nproc)"
make install

WIRESHARK_FUZZERS_COMMON_FLAGS="-lFuzzingEngine \
    -L"$WIRESHARK_INSTALL_PATH/lib" -lwireshark -lwiretap -lwsutil \
    -Wl,-Bstatic `pkg-config --libs glib-2.0` -pthread -lpcre -lgcrypt -lgpg-error -lz -Wl,-Bdynamic"

for dissector in $FUZZ_DISSECTORS; do
  fuzzer_name=fuzzshark_dissector_${dissector}

  # -I$SRC/wireshark is correct, wireshark don't install header files.
  $CC $CFLAGS -I $SRC/wireshark/ `pkg-config --cflags glib-2.0` \
      $SRC/wireshark/tools/oss-fuzzshark.c \
      -c -o $WORK/${fuzzer_name}.o \
      -DFUZZ_DISSECTOR_TARGET=\"$dissector\"

  $CXX $CXXFLAGS $WORK/${fuzzer_name}.o \
      -o $OUT/${fuzzer_name} \
      ${WIRESHARK_FUZZERS_COMMON_FLAGS}

  echo -en "[libfuzzer]\nmax_len = 1024\n" > $OUT/${fuzzer_name}.options
done

popd > /dev/null

export LDFLAGS=
export CC=$INITIAL_CC
export CXX=$INITIAL_CXX
export CFLAGS=$INITIAL_CFLAGS
export CXXFLAGS=$INITIAL_CXXFLAGS
export PATH=$INITIAL_PATH
export ADDITIONAL=

