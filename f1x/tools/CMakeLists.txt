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

## compile F1X to a shared library
add_library (F1X SHARED
  F1X.cpp
  F1X.h
)
set_target_properties(F1X PROPERTIES PUBLIC_HEADER "F1X.h")
INSTALL(TARGETS F1X
        DESTINATION lib
        PUBLIC_HEADER DESTINATION include
)

## f1x target:

add_executable(f1x F1X.cpp)

target_link_libraries(f1x
  f1xRepair
  ${Boost_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
  rt)


## f1x-bench target:

add_executable(f1x-bench F1XBench.cpp)

target_link_libraries(f1x-bench
  f1xRepair # this is not really needed
  ${Boost_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
  rt)


## f1x-transform target:

# Find the libraries that correspond to the LLVM components
# that we wish to use
llvm_map_components_to_libnames(llvm_libs option support)

add_executable(f1x-transform F1XTransform.cpp)

target_link_libraries(f1x-transform
  f1xTransform
  ${llvm_libs}
  clangTooling
  )

set_target_properties(f1x-transform PROPERTIES COMPILE_FLAGS "-fno-rtti -fno-exceptions" ) # this is to be compatible with llvm libraries

## f1x-cc and f1x-cxx targets
configure_file(f1x-cc f1x-cc COPYONLY)
configure_file(f1x-cxx f1x-cxx COPYONLY)
configure_file(f1x-llvm-cov f1x-llvm-cov COPYONLY)
