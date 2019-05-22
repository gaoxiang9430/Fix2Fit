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

#include <memory>
#include <stdexcept>
#include <map>
#include <boost/filesystem.hpp>
#include <string>

enum class RepairStatus {
  SUCCESS,
  FAILURE,
  ERROR,
  NO_NEGATIVE_TESTS
};


const unsigned SUCCESS_EXIT_CODE = 0;
const unsigned FAILURE_EXIT_CODE = 122;
const unsigned ERROR_EXIT_CODE = 1;
const unsigned NO_NEGATIVE_TESTS_EXIT_CODE = 123;


enum class TestPrioritization {
  FIXED_ORDER, // first run failing, then passing tests
  MAX_FAILING  // dynamically prioritize tests based on previous failures
};


enum class PatchPrioritization {
  SYNTACTIC_DIFF, // patches that are syntactically closer to the original program are assigned lower cost
  SEMANTIC_DIFF   // patches that produce execution traces closer to the execution traces of the original program are assigned lower cost
};


struct PatchID {
  unsigned long base;  // 0 is reserved for special purpose
  unsigned long int2;  // 0 means disabled
  unsigned long bool2; // 0 means disabled
  unsigned long cond3; // 0 means disabled
  unsigned long param; // this is expression parameter

  bool operator==(const PatchID &other) const {
    return (base == other.base
         && int2 == other.int2
         && bool2 == other.bool2
         && cond3 == other.cond3
         && param == other.param);
  }
};


typedef unsigned long AppID;


enum class TestStatus {
  PASS, FAIL, TIMEOUT
};


enum class NodeKind {
  OPERATOR, VARIABLE, CONSTANT,
  DEREFERENCE, // marks "a->b" in order to add NULL checks
  PARAMETER, INT2, BOOL2, COND3 // abstract node kinds
};


// BOOLEAN, BITVECTOR and INTEGER are all integer types
// ANY is for type inference
enum class Type {
  ANY, BOOLEAN, INTEGER, POINTER, BITVECTOR
};


enum class Operator {
  NONE, // this is when node is not an operator
  EQ, NEQ, LT, LE, GT, GE, OR, AND, ADD, SUB, MUL, DIV, MOD, NEG, NOT,
  BV_AND, BV_XOR, BV_OR, BV_SHL, BV_SHR, BV_NOT,
  PTR_ADD, PTR_SUB, // pointer arithmetic
  IMPLICIT_BV_CAST, IMPLICIT_INT_CAST, // auxiliary operators to satisfy our type system
    EXPLICIT_BV_CAST, EXPLICIT_INT_CAST, EXPLICIT_PTR_CAST, EXPLICIT_UNSIGNED_CAST // auxiliary operators for (1) INT2 substitutions, (2) pointer arithmetics
};


const std::string DEFAULT_BOOLEAN_TYPE = "int"; // any type is OK
const std::string EXPLICIT_INT_CAST_TYPE = "long";
const std::string EXPLICIT_UNSIGNED_CAST_TYPE = "unsigned";
const std::string EXPLICIT_BV_CAST_TYPE = "unsigned long";
const std::string EXPLICIT_PTR_CAST_TYPE = "void";


//NOTE: instead of designing hierarchy, we put everything into a single node (because of a whim)
struct Expression {
  NodeKind kind;
  Type type; /* should not be ANY */
  Operator op; /* should be NONE if not of the kind OPERATOR */
  std::string rawType; /* either integer type (char, unsinged char, unsigned short, ...) or pointer base type */
  std::string repr; /* 1, 2,... for constants; "x", "y",... for variables; ">=",... for ops */
  std::vector<Expression> args;
};


const Expression TRUE_NODE = Expression{ NodeKind::CONSTANT,
                                         Type::BOOLEAN,
                                         Operator::NONE,
                                         "int",
                                         "1",
                                         {} };

const Expression FALSE_NODE = Expression{ NodeKind::CONSTANT,
                                          Type::BOOLEAN,
                                          Operator::NONE,
                                          "int",
                                          "0",
                                          {} };

const Expression NULL_NODE = Expression{ NodeKind::CONSTANT,
                                         Type::POINTER,
                                         Operator::NONE,
                                         "void",
                                         "(void*)0",
                                         {} };


struct Location {
  unsigned long fileId;
  unsigned long beginLine;
  unsigned long beginColumn;
  unsigned long endLine;
  unsigned long endColumn;

  bool operator==(const Location &other) const { 
    return (fileId == other.fileId
            && beginLine == other.beginLine
            && beginColumn == other.beginColumn
            && endLine == other.endLine
            && endColumn == other.endColumn);
  }
};


enum class TransformationSchema {
  EXPRESSION,      // modifying side-effect free expressions (conditions, RHS of assignments, return arguments)
  IF_GUARD,        // inserting if-guards for break, continue, function calls
  LOOSENING,       // appending `|| expr` to conditions with side effects
  TIGHTENING,      // appending `&& expr` to conditions with side effects
  INITIALIZATION   // inserting memory initialization
};


enum class SynthesisRule {
  OPERATOR,       // operator replacement e.g. > --> >=
  SWAPING,        // swaping arguments
  SIMPLIFICATION, // simplifying expression
  GENERALIZATION, // e.g. 1 --> x
  CONCRETIZATION, // e.g. x --> 1
  LOOSENING,      // adding "|| something"
  TIGHTENING,     // adding "&& something"
  NEGATION,       // (logically) negate or remove negation
  NULL_CHECK,     // adding null check
  SUBSTITUTION,   // (generic) substution of subnode
  UNSIGNED_CAST   // adding unsigned cast
};


enum struct LocationContext {
  CONDITION, //NOTE: this is for if and loop conditions, etc.
  UNKNOWN
};


struct SchemaApplication {
  AppID id;
  TransformationSchema schema;
  Location location;
  LocationContext context;
  Expression original;
  std::vector<Expression> components;
  std::vector<std::string> completePointeeTypes; // for pointer arithmetic
};


struct PatchMetadata {
  SynthesisRule rule;
  unsigned long distance;
};


struct Patch {
  PatchID id;
  std::shared_ptr<SchemaApplication> app;
  Expression modified;
  PatchMetadata meta;
};

struct Partition {
  std::shared_ptr<std::vector<PatchID>> patches;
  AppID id;

  bool operator==(const Partition &other) const {
    return (id == other.id
         && patches == other.patches);
  }

};

typedef std::__cxx11::basic_string<char> __string;
