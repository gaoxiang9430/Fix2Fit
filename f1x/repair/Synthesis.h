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

#include <memory>

#include <boost/filesystem.hpp>

#include "Util.h"

/*
  Expression synthesizer:

  BOOL2 is "x > y", "p != NULL" (there cannot be BOOL1 in our type system)
  INT2 is "x + y"
  COND3 is either BOOL2 or "x > INT2"

  The synthsizer currently has two limitations because of the runtime design:
  1. We cast all INT2 to EXPLICIT_INT_CAST_TYPE, because they are representated using a single variable
  2. We don't substitute left argument of PTR_ADD/PTR_SUB because of scaling

  narrowing/tightening is done with COND3

  Note that the synthesizer accepts and produces only well-typed expressions

  Distance computation:
  replace operator = 1
  simplify (remove subtree A) = depth(A)
  substitute A -> B = depth(A) + depth(B) - 1
  append || A (&& A) = depth(A) 
 */

std::vector<Patch>
generateSearchSpace(const std::vector<std::shared_ptr<SchemaApplication>> &schemaApplications,
                    std::ostream &OS,
                    std::ostream &OH);
