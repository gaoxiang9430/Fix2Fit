#!/bin/bash

# Disable UBSan vptr since several targets built with -fno-rtti.
export CC=/usr/local/bin/clang
export CXX=/usr/local/bin/clang++
export CFLAGS="$CFLAGS -fno-sanitize=vptr"
export CXXFLAGS="$CXXFLAGS -fno-sanitize=vptr"

# Build dependencies.
export FFMPEG_DEPS_PATH=$SRC/ffmpeg_deps
mkdir -p $FFMPEG_DEPS_PATH

# Build latest nasm without memory instrumentation.
cd $SRC
tar xzf nasm-*
cd nasm-*
CFLAGS="" CXXFLAGS="" ./configure --prefix="$FFMPEG_DEPS_PATH"
make clean
make -j$(nproc)
make install

export PATH="$FFMPEG_DEPS_PATH/bin:$PATH"
export LD_LIBRARY_PATH="$FFMPEG_DEPS_PATH/lib"

cd $SRC
bzip2 -f -d alsa-lib-*
tar xf alsa-lib-*
cd alsa-lib-*
./configure --prefix="$FFMPEG_DEPS_PATH" --enable-static --disable-shared
make clean
make -j$(nproc) all
make install

cd $SRC/drm
# Requires xutils-dev libpciaccess-dev
./autogen.sh
./configure --prefix="$FFMPEG_DEPS_PATH" --enable-static
make clean
make -j$(nproc)
make install

cd $SRC/fdk-aac
autoreconf -fiv
./configure --prefix="$FFMPEG_DEPS_PATH" --disable-shared
make clean
make -j$(nproc) all
make install

cd $SRC
tar xzf lame.tar.gz
cd lame-*
./configure --prefix="$FFMPEG_DEPS_PATH" --enable-static
make clean
make -j$(nproc)
make install

cd $SRC/libXext
./autogen.sh
./configure --prefix="$FFMPEG_DEPS_PATH" --enable-static
make clean
make -j$(nproc)
make install

cd $SRC/libXfixes
./autogen.sh
./configure --prefix="$FFMPEG_DEPS_PATH" --enable-static
make clean
make -j$(nproc)
make install

cd $SRC/libva
./autogen.sh
./configure --prefix="$FFMPEG_DEPS_PATH" --enable-static --disable-shared
make clean
make -j$(nproc) all
make install

cd $SRC/libvdpau
./autogen.sh
./configure --prefix="$FFMPEG_DEPS_PATH" --enable-static --disable-shared
make clean
make -j$(nproc) all

cd $SRC/libvpx
LDFLAGS="$CXXFLAGS" ./configure --prefix="$FFMPEG_DEPS_PATH" \
    --disable-examples --disable-unit-tests
make clean
make -j$(nproc) all
make install

cd $SRC/ogg
./autogen.sh
./configure --prefix="$FFMPEG_DEPS_PATH" --enable-static
make clean
make -j$(nproc)
make install

cd $SRC/opus
./autogen.sh
./configure --prefix="$FFMPEG_DEPS_PATH" --enable-static
make clean
make -j$(nproc) all
make install

cd $SRC/libtheora
# theora requires ogg, need to pass its location to the "configure" script.
CFLAGS="$CFLAGS -fPIC" LDFLAGS="-L$FFMPEG_DEPS_PATH/lib/" \
    CPPFLAGS="$CXXFLAGS -I$FFMPEG_DEPS_PATH/include/" \
    LD_LIBRARY_PATH="$FFMPEG_DEPS_PATH/lib/" \
    ./autogen.sh --prefix="$FFMPEG_DEPS_PATH" --enable-static --disable-examples
#make clean
make -j$(nproc)
make install

cd $SRC/vorbis
./autogen.sh
./configure --prefix="$FFMPEG_DEPS_PATH" --enable-static
make clean
make -j$(nproc)
make install

cd $SRC/x264
LDFLAGS="$CXXFLAGS" ./configure --prefix="$FFMPEG_DEPS_PATH" \
    --enable-static
make clean
make -j$(nproc)

cd $SRC/x265/build/linux
cmake -G "Unix Makefiles" \
    -DCMAKE_C_COMPILER=$CC -DCMAKE_CXX_COMPILER=$CXX \
    -DCMAKE_C_FLAGS="$CFLAGS" -DCMAKE_CXX_FLAGS="$CXXFLAGS" \
    -DCMAKE_INSTALL_PREFIX="$FFMPEG_DEPS_PATH" -DENABLE_SHARED:bool=off \
    ../../source
make clean
make -j$(nproc) x265-static
make install

# Remove shared libraries to avoid accidental linking against them.
rm $FFMPEG_DEPS_PATH/lib/*.so
rm $FFMPEG_DEPS_PATH/lib/*.so.*

