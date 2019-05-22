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

#include "Util.h"
#include "Config.h"

/*
  The aims of type inference are
  1. Eliminate nonsensical expressions from search space
  2. Guarantee that meta program compiles

  C expressions are lifted to a type system with the follwing types:
  1. Boolean
  2. Integer
  3. Bitvector
  4. Pointer

  Type constraints come from:
  1. Context (condition/unknown)
  2. Components (integer/pointer)
  3. Operators (input/output types)
  4. Operator overloading (arguments should be of the same type)
 */

Type operatorOutputType(const Operator &op);

Expression correctTypes(const Expression &expression, const Type &context);
