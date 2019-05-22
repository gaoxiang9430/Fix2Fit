#!/usr/bin/env bash

#  This file is part of f1x.
#  Copyright (C) 2016  Sergey Mechtaev, Gao Xiang, Shin Hwei Tan, Abhik Roychoudhury
#
#  f1x is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.

set -euo pipefail

download () {
    wget --retry-connrefused --waitretry=1 --read-timeout=20 --timeout=15 --tries=5 --continue "$1"
}

require () {
    hash "$1" 2>/dev/null || { echo >&2 "I require $1 but it's not installed. Aborting."; exit 1; }
}

require wget
require cmake
require ninja

LLVM_URL="http://llvm.org/releases/3.8.1/llvm-3.8.1.src.tar.xz"
LLVM_ARCHIVE="llvm-3.8.1.src.tar.xz"
CLANG_URL="http://llvm.org/releases/3.8.1/cfe-3.8.1.src.tar.xz"
CLANG_ARCHIVE="cfe-3.8.1.src.tar.xz"
COMPILER_RT_URL="http://llvm.org/releases/3.8.1/compiler-rt-3.8.1.src.tar.xz"
COMPILER_RT_ARCHIVE="compiler-rt-3.8.1.src.tar.xz"
CLANG_TOOLS_EXTRA_URL="http://llvm.org/releases/3.8.1/clang-tools-extra-3.8.1.src.tar.xz"
CLANG_TOOLS_EXTRA_ARCHIVE="clang-tools-extra-3.8.1.src.tar.xz"

INSTALL_DIR="$PWD"

download "$LLVM_URL"
download "$CLANG_URL"
download "$COMPILER_RT_URL"
download "$CLANG_TOOLS_EXTRA_URL"

mkdir -p "src"
tar xf "$LLVM_ARCHIVE" --directory "src" --strip-components=1

mkdir -p "src/tools/clang"
tar xf "$CLANG_ARCHIVE" --directory "src/tools/clang" --strip-components=1

mkdir -p "src/projects/compiler-rt"
tar xf "$COMPILER_RT_ARCHIVE" --directory "src/projects/compiler-rt" --strip-components=1

mkdir -p "src/tools/clang/tools/extra"
tar xf "$CLANG_TOOLS_EXTRA_ARCHIVE" --directory "src/tools/clang/tools/extra" --strip-components=1

mkdir -p "build"

(
    cd "build"
    cmake "../src" -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR" -G "Ninja"
    ninja
    ninja install
)

rm -rf build
rm -f "$LLVM_ARCHIVE"
rm -f "$CLANG_ARCHIVE"
rm -f "$COMPILER_RT_ARCHIVE"
rm -f "$CLANG_TOOLS_EXTRA_ARCHIVE"
