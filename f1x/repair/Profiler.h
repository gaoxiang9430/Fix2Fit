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

#include <unordered_map>
#include <set>

#include <boost/filesystem.hpp>

#include "Util.h"


const std::string TRACE_FILE_NAME          = "trace.txt";
const std::string PROFILE_FILE_NAME        = "profile.txt";
const std::string PROFILE_SOURCE_FILE_NAME = "profile.cpp";
const std::string PROFILE_HEADER_FILE_NAME = "profile.h";


class Profiler {
 public:
  boost::filesystem::path getHeader();
  boost::filesystem::path getSource();
  bool compile();
  std::unordered_map<Location, std::vector<unsigned>> getRelatedTestIndexes();
  boost::filesystem::path getProfile();
  void mergeTrace(unsigned testIndex, bool isPassing);
  void clearTrace();

 private:
  std::unordered_map<Location, std::vector<unsigned>> relatedTestIndexes;
  std::set<std::string> interestingLocations; //NOTE: set of string to make more deterministic
};
