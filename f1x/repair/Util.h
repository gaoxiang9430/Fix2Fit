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

#include <unordered_map>
#include "Config.h"
#include "Core.h"


// this need to be removed when we use a custom timeout function:
const unsigned TIMEOUT_EXIT_CODE = 124;


// http://stackoverflow.com/questions/19195183/how-to-properly-hash-the-custom-struct
template <class T>
inline void hash_combine(std::size_t & s, const T & v) {
  std::hash<T> h;
  s ^= h(v) + 0x9e3779b9 + (s << 6) + (s >> 2);
}

namespace std {
  template<>
    struct hash<PatchID> {
    inline size_t operator()(const PatchID& id) const {
      size_t value = 0;
      hash_combine(value, id.base);
      hash_combine(value, id.int2);
      hash_combine(value, id.bool2);
      hash_combine(value, id.cond3);
      hash_combine(value, id.param);
      return value;
    }
  };
}

namespace std {
  template<>
    struct hash<Partition> {
    inline size_t operator()(const Partition& par) const {
      size_t value = 0;
      hash_combine(value, par.id);
      hash_combine(value, par.patches);
      return value;
    }
  };
}


bool isAbstractNode(NodeKind kind);

Operator binaryOperatorByString(const std::string &repr);

Operator unaryOperatorByString(const std::string &repr);

std::string operatorToString(const Operator &op);

std::string expressionToString(const Expression &expression);

Expression makeIntegerConst(int n);

Expression wrapWithImplicitIntCast(const Expression &expression);

Expression wrapWithImplicitBVCast(const Expression &expression);

Expression wrapWithExplicitIntCast(const Expression &expression);

Expression wrapWithExplicitBVCast(const Expression &expression);

Expression wrapWithExplicitPtrCast(const Expression &expression);

Expression wrapWithExplicitUnsignedCast(const Expression &expression);

Expression applyBoolOperator(const Operator &op, 
                             const Expression &left,
                             const Expression &right);

Expression makeNonNULLCheck(const Expression &pointer);

Expression makeNULLCheck(const Expression &pointer);

Expression makeNonZeroCheck(const Expression &expression);

namespace std {
  template<>
    struct hash<Location> {
    inline size_t operator()(const Location& id) const {
      size_t value = 0;
      hash_combine(value, id.fileId);
      hash_combine(value, id.beginLine);
      hash_combine(value, id.beginColumn);
      hash_combine(value, id.endLine);
      hash_combine(value, id.endColumn);
      return value;
    }
  };
}

std::string visualizePatchID(const PatchID &id);

std::string visualizeChange(const Patch &el);


std::string visualizeElement(const Patch &el, const boost::filesystem::path &file);


class FromDirectory {
 public:
  FromDirectory(const boost::filesystem::path &path);
  ~FromDirectory();

 private:
  boost::filesystem::path original;
};


class InEnvironment {
 public:
  InEnvironment(const std::map<std::string, std::string> &env);
  ~InEnvironment();

 private:
  std::map<std::string, std::string> original;
};


class parse_error : public std::logic_error {
 public:
  using std::logic_error::logic_error;
};


std::vector<std::shared_ptr<SchemaApplication>> loadSchemaApplications(const std::vector<boost::filesystem::path> &paths);


bool isExecutable(const char *file);


boost::filesystem::path relativeTo(boost::filesystem::path from, boost::filesystem::path to);


const unsigned long MAX_PRINT_TESTS = 5;

std::string prettyPrintTests(const std::vector<std::string> &tests);


void dumpSearchSpace(std::vector<Patch> &searchSpace,
                     const boost::filesystem::path &file,
                     const std::vector<boost::filesystem::path> &files,
                     std::unordered_map<PatchID, double> &cost);
