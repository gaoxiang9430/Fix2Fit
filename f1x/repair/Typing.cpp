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

#include "Typing.h"
#include <string>


Type operatorOutputType(const Operator &op) {
  switch (op) {
  case Operator::EQ:
    return Type::BOOLEAN;
  case Operator::NEQ:
    return Type::BOOLEAN;
  case Operator::LT:
    return Type::BOOLEAN;
  case Operator::LE:
    return Type::BOOLEAN;
  case Operator::GT:
    return Type::BOOLEAN;
  case Operator::GE:
    return Type::BOOLEAN;
  case Operator::OR:
    return Type::BOOLEAN;
  case Operator::AND:
    return Type::BOOLEAN;
  case Operator::ADD:
    return Type::INTEGER;
  case Operator::SUB:
    return Type::INTEGER;
  case Operator::MUL:
    return Type::INTEGER;
  case Operator::DIV:
    return Type::INTEGER;
  case Operator::MOD:
    return Type::INTEGER;
  case Operator::NEG:
    return Type::INTEGER;
  case Operator::NOT:
    return Type::BOOLEAN;
  case Operator::BV_AND:
    return Type::BITVECTOR;
  case Operator::BV_OR:
    return Type::BITVECTOR;
  case Operator::BV_XOR:
    return Type::BITVECTOR;
  case Operator::BV_SHL:
    return Type::BITVECTOR;
  case Operator::BV_SHR:
    return Type::BITVECTOR;
  case Operator::BV_NOT:
    return Type::BITVECTOR;
  case Operator::PTR_ADD:
    return Type::POINTER;
  case Operator::PTR_SUB:
    return Type::POINTER;
  case Operator::IMPLICIT_BV_CAST:
    return Type::INTEGER;
  case Operator::IMPLICIT_INT_CAST:
    return Type::BITVECTOR;
  case Operator::EXPLICIT_BV_CAST:
    return Type::BITVECTOR;
  case Operator::EXPLICIT_INT_CAST:
    return Type::INTEGER;
  case Operator::EXPLICIT_UNSIGNED_CAST:
    return Type::INTEGER;
  case Operator::EXPLICIT_PTR_CAST:
    return Type::POINTER;
  }
  throw std::invalid_argument("unsupported operator: " + std::to_string((unsigned long)op));
}

//NOTE: the first and the second are the same except for pointer arithmetic
Type operatorFirstArgType(const Operator &op) {
  switch (op) {
  case Operator::EQ:
    return Type::ANY;
  case Operator::NEQ:
    return Type::ANY;
  case Operator::LT:
    return Type::INTEGER;
  case Operator::LE:
    return Type::INTEGER;
  case Operator::GT:
    return Type::INTEGER;
  case Operator::GE:
    return Type::INTEGER;
  case Operator::OR:
    return Type::BOOLEAN;
  case Operator::AND:
    return Type::BOOLEAN;
  case Operator::ADD:
    return Type::INTEGER;
  case Operator::SUB:
    return Type::INTEGER;
  case Operator::MUL:
    return Type::INTEGER;
  case Operator::DIV:
    return Type::INTEGER;
  case Operator::MOD:
    return Type::INTEGER;
  case Operator::NEG:
    return Type::INTEGER;
  case Operator::NOT:
    return Type::BOOLEAN;
  case Operator::BV_AND:
    return Type::BITVECTOR;
  case Operator::BV_OR:
    return Type::BITVECTOR;
  case Operator::BV_XOR:
    return Type::BITVECTOR;
  case Operator::BV_SHL:
    return Type::BITVECTOR;
  case Operator::BV_SHR:
    return Type::BITVECTOR;
  case Operator::BV_NOT:
    return Type::BITVECTOR;
  case Operator::PTR_ADD:
    return Type::POINTER;
  case Operator::PTR_SUB:
    return Type::POINTER;
  case Operator::IMPLICIT_BV_CAST:
  case Operator::IMPLICIT_INT_CAST:
  case Operator::EXPLICIT_BV_CAST:
  case Operator::EXPLICIT_INT_CAST:
  case Operator::EXPLICIT_UNSIGNED_CAST:
  case Operator::EXPLICIT_PTR_CAST:
    return Type::ANY;
  }
  throw std::invalid_argument("unsupported operator: " + std::to_string((unsigned long)op));
}

/*
  POINTER < INTEGER < ANY
  BOOLEAN < INTEGER < ANY
  POINTER < BOOLEAN --- this is just so that runtime compiles
*/
Type commonSubtype(Type a, Type b) {
  if (a == Type::ANY)
    return b;
  if (b == Type::ANY)
    return a;
  if (a == Type::POINTER || b == Type::POINTER)
    return Type::POINTER;
  if (a == Type::BOOLEAN || b == Type::BOOLEAN)
    return Type::BOOLEAN;
  if (a == Type::INTEGER || b == Type::INTEGER)
    return Type::INTEGER;
  throw std::invalid_argument("unsupported type");
}

Expression correctTopNode(const Expression &expression, const Type &context) {
  switch (context) {
  case Type::ANY:
    return expression;
  case Type::INTEGER:
    if (expression.type == Type::INTEGER) {
      return expression;
    } else if (expression.type == Type::POINTER) {
      return wrapWithExplicitIntCast(expression);
    } else {
      return wrapWithImplicitIntCast(expression);
    }
  case Type::BITVECTOR:
    if (expression.type == Type::BITVECTOR) {
      return expression;
    } else if (expression.type == Type::POINTER) {
      return wrapWithExplicitBVCast(expression);
    } else {
      return wrapWithImplicitBVCast(expression);
    }
  case Type::BOOLEAN:
    if (expression.type == Type::BOOLEAN) {
      return expression;
    } else if (expression.type == Type::POINTER) {
      return makeNonNULLCheck(expression);
    } else if (expression.type == Type::INTEGER &&
               expression.kind == NodeKind::CONSTANT) {
      if (expression.repr == "0")
        return FALSE_NODE;
      else if (expression.repr == "1")
        return TRUE_NODE;
      else
        return makeNonZeroCheck(expression);
    } else {
      return makeNonZeroCheck(expression);
    }
  case Type::POINTER:
    if (expression.type == Type::POINTER) {
      return expression;
    } else {
      return wrapWithExplicitPtrCast(expression);
    }
  }
}

Expression correctTypes(const Expression &expression, const Type &context) {
  if (expression.kind == NodeKind::VARIABLE ||
      expression.kind == NodeKind::CONSTANT ||
      expression.kind == NodeKind::PARAMETER ||
      expression.kind == NodeKind::DEREFERENCE) {
    return correctTopNode(expression, context);
  } else if (expression.kind == NodeKind::OPERATOR) {
    if (expression.args.size() == 1) {
      Type argContext = operatorFirstArgType(expression.op);
      Expression correctedArg = correctTypes(expression.args[0], argContext);
      Expression copy = expression;
      copy.args[0] = correctedArg;
      return correctTopNode(copy, context);
    } else if (expression.args.size() == 2) {
      if (expression.op == Operator::PTR_ADD ||
          expression.op == Operator::PTR_SUB) {
        assert(expression.args[0].type == Type::POINTER);
        Expression correctedArg1 = correctTypes(expression.args[1], Type::INTEGER);
        Expression copy = expression;
        copy.args[1] = correctedArg1;
        return correctTopNode(copy, context);
      } else {
        Type argContext = operatorFirstArgType(expression.op);
        if (argContext == Type::ANY) {
          argContext = commonSubtype(expression.args[0].type, expression.args[1].type);
        }
        Expression correctedArg0 = correctTypes(expression.args[0], argContext);
        Expression correctedArg1 = correctTypes(expression.args[1], argContext);
        Expression copy = expression;
        copy.args[0] = correctedArg0;
        copy.args[1] = correctedArg1;
        return correctTopNode(copy, context);
      }
    } else {
      throw std::invalid_argument("unsupported operator");
    }
  }
  throw std::invalid_argument("unsupported node kind");
}
