#!/usr/bin/env bash
# Build LLVM 3.8.1 for building f1x

mkdir -p llvm-3.8.1
pushd llvm-3.8.1
if [ ! -e llvm-3.8.1.src.tar.xz ] ; then
	wget http://releases.llvm.org/3.8.1/llvm-3.8.1.src.tar.xz
fi
if [ ! -e cfe-3.8.1.src.tar.xz ] ; then
	wget http://releases.llvm.org/3.8.1/cfe-3.8.1.src.tar.xz
fi
if [ ! -e compiler-rt-3.8.1.src.tar.xz ] ; then
	wget http://releases.llvm.org/3.8.1/compiler-rt-3.8.1.src.tar.xz
fi
tar xf llvm-3.8.1.src.tar.xz
tar xf cfe-3.8.1.src.tar.xz
tar xf compiler-rt-3.8.1.src.tar.xz
mv llvm-3.8.1.src src
mv cfe-3.8.1.src src/tools/clang
mv compiler-rt-3.8.1.src src/projects/compiler-rt
mkdir -p install
LLVM_INSTALL_DIR=$PWD/install
mkdir -p build
pushd build
cmake -G "Unix Makefiles" -DCMAKE_INSTALL_PREFIX=$LLVM_INSTALL_DIR -DCMAKE_BUILD_TYPE=Release -DLLVM_ENABLE_ASSERTIONS=Off ../src
make install
popd
popd
