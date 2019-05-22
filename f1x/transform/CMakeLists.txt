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

add_library (f1xTransform
  TransformGlobal.cpp
  TransformUtil.cpp
  SearchSpaceMatchers.cpp
  ProfileInstrumentation.cpp
  SchemaApplication.cpp
  PatchApplication.cpp
  )

# Make sure the compiler can find include files for our Hello library
# when other libraries or executables link to Hello
target_include_directories (f1xTransform PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(f1xTransform PROPERTIES COMPILE_FLAGS "-fno-rtti -fno-exceptions" ) # this is to be compatible with llvm libraries