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

#include "Prioritization.h"


double syntacticDiff(const Patch &el) {
  double result = (double) el.meta.distance;
  const double GOOD = 0.2;
  const double OK = 0.1;
  switch (el.app->schema) {
  case TransformationSchema::EXPRESSION:
    switch (el.meta.rule) {
    case SynthesisRule::OPERATOR:
      result -= GOOD;
      break;
    case SynthesisRule::SWAPING:
      result -= GOOD;
      break;
    case SynthesisRule::SIMPLIFICATION:
      result -= GOOD;
      break;
    case SynthesisRule::GENERALIZATION:
      result -= GOOD;
      break;
    case SynthesisRule::SUBSTITUTION:
      result -= GOOD;
      break;
    case SynthesisRule::LOOSENING:
      result -= OK;
      break;
    case SynthesisRule::TIGHTENING:
      result -= OK;
      break;
    default:
      break; // everything else is bad
    }
    break;
  case TransformationSchema::IF_GUARD:
    result -= GOOD;
    break;
  default:
    break;
  }
  return result;
}
