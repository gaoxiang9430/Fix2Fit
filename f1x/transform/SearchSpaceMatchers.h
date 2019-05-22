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

#include "clang/ASTMatchers/ASTMatchers.h"

#define BOUND "repairable"


/*
  Matcher for EXPRESSION transformation schema
 */
extern clang::ast_matchers::StatementMatcher ExpressionSchemaMatcher;

/*
  Matcher for IF_GUARD transformation schema
  NOTE: need to manually check if is it top level statement
 */
extern clang::ast_matchers::StatementMatcher IfGuardSchemaMatcher;


/*
  Matcher for REFINEMENT transformation schema. Matches conditions not matched by ExpressionSchemaMatcher
 */
extern clang::ast_matchers::StatementMatcher RefinementSchemaMatcher;
