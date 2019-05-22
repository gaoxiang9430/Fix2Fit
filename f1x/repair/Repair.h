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

#include <boost/filesystem.hpp>

#include "Config.h"
#include "Core.h"
#include "Project.h"
#include "SearchEngine.h"

/*
  Initializes repair process, performs search, saves patch(es) if found.
 */
RepairStatus repair(Project &project,
                    TestingFramework &tester,
                    const std::vector<std::string> &tests,
                    const boost::filesystem::path &patchOutput,
                    SearchEngine* &engine);
