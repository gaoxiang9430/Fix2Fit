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

#include <unordered_set>
#include <string>
#include <sstream>

#include <boost/filesystem.hpp>

#include "Config.h"
#include "Util.h"


const std::string RUNTIME_SOURCE_FILE_NAME = "rt.cpp";
const std::string RUNTIME_HEADER_FILE_NAME = "rt.h";

const unsigned long MAX_PARTITION_SIZE = 1000000;
const std::string PARTITION_FILE_NAME = "/f1x_partition";
const PatchID INPUT_TERMINATOR = PatchID{0, 0, 0, 0, 0};
const PatchID OUTPUT_TERMINATOR = PatchID{0, 0, 0, 0, 1};


class Runtime {
 public:
  Runtime();
  void setPartition(std::unordered_set<PatchID> ids);
  void setOutputResult(int);
  int getOutputResult();
  std::unordered_set<PatchID> getPartition();
  boost::filesystem::path getSource();
  boost::filesystem::path getHeader();
  bool compile();

 private:
  PatchID *partition;
};
