/*
  This file is part of f1x.
  Copyright (C) 2016  Sergey Mechtaev, Gao Xiang, Shin Hwei Tan, Abhik Roychoudhury

  f1x is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <vector>
#include <string>


/* This is a library for executing a given command in a child process.
   The command is executed in a new process group and the group is killed on timeout. */

unsigned run_executable(std::string file, std::vector<std::string> args, unsigned timeout=0, bool mute_stdout=true, bool mute_stderr=true, bool always_kill=false);

unsigned run_shell(std::string cmd, unsigned timeout=0, bool mute_stdout=true, bool mute_stderr=true, bool always_kill=false);
