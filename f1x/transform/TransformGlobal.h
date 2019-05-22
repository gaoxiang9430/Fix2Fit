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


struct Config {
  unsigned fileId;
  unsigned fromLine;
  unsigned toLine;
  std::string profileFile;
  std::string outputFile;
  unsigned beginLine;
  unsigned beginColumn;
  unsigned endLine;
  unsigned endColumn;
  std::string patch;
  unsigned long baseAppId;
  bool useGlobalVariables;
  bool addGuards;
  bool inplaceModification;
};


extern struct Config cfg;
