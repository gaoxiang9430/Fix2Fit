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

cmake_minimum_required(VERSION 3.4.3)
project(f1x)

set(LLVM_DIR "${F1X_LLVM}/share/llvm/cmake/")
set(Clang_DIR "${F1X_LLVM}/share/clang/cmake/")
set(F1X_CLANG_INCLUDE "${F1X_LLVM}/lib/clang/3.8.1/include/")

# LLVM
find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# Set your project compile flags.
# E.g. if using the C++ header files
# you will need to enable C++11 support
# for your compiler.

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# Threads
FIND_PACKAGE (Threads)

# Clang
find_package(Clang REQUIRED CONFIG)

# Boost
set(Boost_USE_MULTITHREADED OFF)
FIND_PACKAGE( Boost 1.40 COMPONENTS program_options log REQUIRED )
INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIR} )
ADD_DEFINITIONS(-DBOOST_LOG_DYN_LINK)

# Compiler
set(CMAKE_CXX_FLAGS "-lboost_program_options") #added on 29 Sep 2017
set(CMAKE_CXX_STANDARD 11) # C++11...
set(CMAKE_CXX_STANDARD_REQUIRED ON) #...is required...
set(CMAKE_CXX_EXTENSIONS OFF) #...without compiler extensions like gnu++11

# The version number.
set (F1X_VERSION_MAJOR 0)
set (F1X_VERSION_MINOR 1)
set (F1X_VERSION_PATCH 0)

# configure a header file to pass some of the CMake settings
# to the source code
configure_file (
  "${PROJECT_SOURCE_DIR}/Config.h.in"
  "${PROJECT_BINARY_DIR}/Config.h"
  )

# add the binary tree to the search path for include files
# so that we will find Config.h
include_directories("${PROJECT_BINARY_DIR}")

# JSON library
include_directories(thirdparty)

include_directories(repair)
include_directories(transform)

add_subdirectory(repair)
add_subdirectory(transform)
add_subdirectory(tools)

set (F1X_TOOLS_DIR "${PROJECT_BINARY_DIR}/tools/")
set (F1X_LIBEAR_DIR "${PROJECT_BINARY_DIR}/thirdparty/bear/libear/")

add_subdirectory(thirdparty/bear)
