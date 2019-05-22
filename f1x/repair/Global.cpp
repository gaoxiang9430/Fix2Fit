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

#include "Config.h"
#include "Core.h"
#include "Global.h"


struct Config cfg = {
  /* globalVariables          = */ false,
  /* verbose                  = */ false,
  /* validatePatches          = */ false,
  /* generateAll              = */ false,
  /* searchSpaceFile          = */ "",
  /* statisticsFile           = */ "",
  /* dataDir                  = */ "",
  /* outputPatchMetadata      = */ false,
  /* removeIntermediateData   = */ false,
  /* insertAssignments        = */ true,
  /* addGuards                = */ true,
  /* maxConditionParameter    = */ 2,
  /* maxExpressionParameter   = */ 0,
  /* valueTEQ                 = */ true,
  /* dependencyTEQ            = */ true,
  /* testPrioritization       = */ TestPrioritization::MAX_FAILING,
  /* patchPrioritization      = */ //PatchPrioritization::SEMANTIC_DIFF,
  /* patchPrioritization      = */ PatchPrioritization::SYNTACTIC_DIFF,
  /* filesToLocalize          = */ 10,
  /* useLLVMCov               = */ false,
  /* outputOnePerLocation     = */ false,
  /* outputTop                = */ 0,
  /* validatePatchesByFuzzing = */ true,
  /* binaryPath               = */ "", //TODO: need better design
  /* ninaryName               = */ ""
};
