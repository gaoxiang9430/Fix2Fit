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

add_library (f1xRepair SHARED
  Process.cpp
  Global.cpp
  Typing.cpp
  Util.cpp
  Prioritization.cpp  
  Project.cpp
  Profiler.cpp
  Runtime.cpp
  Synthesis.cpp
  SearchEngine.cpp
  Repair.cpp
  FaultLocalization.cpp
  Partitions.cpp
)

target_include_directories (f1xRepair PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(f1xRepair PROPERTIES PUBLIC_HEADER "SearchEngine.h")
INSTALL(TARGETS f1xRepair
        DESTINATION lib
        PUBLIC_HEADER DESTINATION include
)
