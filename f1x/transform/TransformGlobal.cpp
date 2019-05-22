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
#include "TransformGlobal.h"


struct Config cfg = {
  /* fileId              = */ 0,
  /* fromLine            = */ 0,
  /* toLine              = */ 0,
  /* profileFile         = */ "",
  /* outputFile          = */ "",
  /* beginLine           = */ 0,
  /* beginColumn         = */ 0,
  /* endLine             = */ 0,
  /* endColumn           = */ 0,
  /* patch               = */ "",
  /* baseAppId           = */ 0,
  /* useGlobalVariables  = */ false,
  /* addGuards           = */ true,
  /* inplaceModification = */ true
};


