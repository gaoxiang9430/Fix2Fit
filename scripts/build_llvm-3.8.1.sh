#!/usr/bin/env bash
# Build LLVM 3.8.1 for building f1x

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

if [ -x "$SCRIPT_DIR/../llvm-3.8.1/install/bin/clang" ] ; then
    echo LLVM 3.8.1 built
    exit 1
fi

pushd $SCRIPT_DIR/..

mkdir -p llvm-3.8.1
pushd llvm-3.8.1
if [ ! -e llvm-3.8.1.src.tar.xz ] ; then
	wget http://releases.llvm.org/3.8.1/llvm-3.8.1.src.tar.xz
	tar xf llvm-3.8.1.src.tar.xz
	mv llvm-3.8.1.src src
fi
if [ ! -e cfe-3.8.1.src.tar.xz ] ; then
	wget http://releases.llvm.org/3.8.1/cfe-3.8.1.src.tar.xz
	tar xf cfe-3.8.1.src.tar.xz
	mv cfe-3.8.1.src src/tools/clang
fi
if [ ! -e compiler-rt-3.8.1.src.tar.xz ] ; then
	wget http://releases.llvm.org/3.8.1/compiler-rt-3.8.1.src.tar.xz
	tar xf compiler-rt-3.8.1.src.tar.xz
	mv compiler-rt-3.8.1.src src/projects/compiler-rt
fi
mkdir -p install
LLVM_INSTALL_DIR=$PWD/install
mkdir -p build
pushd build
cmake -G "Unix Makefiles" -DCMAKE_INSTALL_PREFIX=$LLVM_INSTALL_DIR -DCMAKE_BUILD_TYPE=Release -DLLVM_ENABLE_ASSERTIONS=Off ../src
make -j$(nproc) install
popd
popd
popd
