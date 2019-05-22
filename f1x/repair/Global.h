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


#include "Config.h"
#include "Core.h"


struct Config {
  bool globalVariables;
  bool verbose;
  bool validatePatches;
  bool generateAll;
  std::string searchSpaceFile;
  std::string statisticsFile;
  std::string dataDir;
  bool outputPatchMetadata;
  bool removeIntermediateData;
  bool insertAssignments;
  bool addGuards;
  unsigned maxConditionParameter;
  unsigned maxExpressionParameter;
  bool valueTEQ;
  bool dependencyTEQ;
  TestPrioritization testPrioritization;
  PatchPrioritization patchPrioritization;
  unsigned filesToLocalize;
  bool useLLVMCov;
  bool outputOnePerLocation;
  signed outputTop;
  bool validatePatchesByFuzzing;
  std::string binaryPath;
  std::string binaryName;
};


extern struct Config cfg;
