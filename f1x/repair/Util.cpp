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

#include <iostream>
#include <iomanip>
#include <memory>
#include <cstdlib>
#include <string>
#include <sys/stat.h>
#include <sstream>

#include <boost/filesystem/fstream.hpp>

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>

#include "Util.h"
#include "Config.h"
#include "Typing.h"

namespace fs = boost::filesystem;
namespace json = rapidjson;

using std::string;
using std::vector;
using std::shared_ptr;


std::string visualizePatchID(const PatchID &id) {
  std::stringstream result;
  result << id.base << ":" << id.int2 << ":" << id.bool2 << ":" << id.cond3 << ":" << id.param;
  return result.str();
}

FromDirectory::FromDirectory(const boost::filesystem::path &path):
  original(fs::current_path()) {
  fs::current_path(path);
}
  
FromDirectory::~FromDirectory() {
  fs::current_path(original);    
}

InEnvironment::InEnvironment(const std::map<std::string, std::string> &env) {
  for (auto &entry : env) {
    string orig;
    if (getenv(entry.first.c_str())) {
      original[entry.first] = getenv(entry.first.c_str());
    }
    setenv(entry.first.c_str(), entry.second.c_str(), true);
  }
}

InEnvironment::~InEnvironment() {
  // TODO: possibly can also erase previously undefined
  for (auto &entry : original) {
    setenv(entry.first.c_str(), entry.second.c_str(), true);
  }
}

bool isAbstractNode(NodeKind kind) {
  return kind == NodeKind::PARAMETER ||
         kind == NodeKind::INT2 ||
         kind == NodeKind::BOOL2 ||
         kind == NodeKind::COND3;
}

NodeKind kindByString(const string &kindStr) {
  if (kindStr == "operator") {
    return NodeKind::OPERATOR;
  } else if (kindStr == "variable") {
    return NodeKind::VARIABLE;
  } else if (kindStr == "dereference") {
    return NodeKind::DEREFERENCE;
  } else if (kindStr == "constant") {
    return NodeKind::CONSTANT;
  } else {
    throw parse_error("unsupported kind: " + kindStr);
  }
}

TransformationSchema transformationSchemaByString(const string &str) {
  if (str == "expression") {
    return TransformationSchema::EXPRESSION;
  } else if (str == "if_guard") {
    return TransformationSchema::IF_GUARD;
  } else if (str == "initialization") {
    return TransformationSchema::INITIALIZATION;
  } else {
    throw parse_error("unsupported transformation schema: " + str);
  }
}


Operator binaryOperatorByString(const string &repr, const Type &outputType) {
  if (repr == "==") {
    return Operator::EQ;
  } else if (repr == "!=") {
    return Operator::NEQ;
  } else if (repr == "<") {
    return Operator::LT;
  } else if (repr == "<=") {
    return Operator::LE;
  } else if (repr == ">") {
    return Operator::GT;
  } else if (repr == ">=") {
    return Operator::GE;
  } else if (repr == "||") {
    return Operator::OR;
  } else if (repr == "&&") {
    return Operator::AND;
  } else if (repr == "+") {
    if (outputType == Type::POINTER)
      return Operator::PTR_ADD;
    else 
      return Operator::ADD;
  } else if (repr == "-") {
    if (outputType == Type::POINTER)
      return Operator::PTR_SUB;
    else 
      return Operator::SUB;
  } else if (repr == "*") {
    return Operator::MUL;
  } else if (repr == "/") {
    return Operator::DIV;
  } else if (repr == "%") {
    return Operator::MOD;
  } else if (repr == "&") {
    return Operator::BV_AND;
  } else if (repr == "|") {
    return Operator::BV_OR;
  } else if (repr == "^") {
    return Operator::BV_XOR;
  } else if (repr == "<<") {
    return Operator::BV_SHL;
  } else if (repr == ">>") {
    return Operator::BV_SHR;
  } else {
    throw parse_error("unsupported binary operator: " + repr);
  }
}


Operator unaryOperatorByString(const string &repr) {
  if (repr == "-") {
    return Operator::NEG;
  } else if (repr == "!") {
    return Operator::NOT;
  } else if (repr == "~") {
    return Operator::BV_NOT;
  } else {
    throw parse_error("unsupported unary operator: " + repr);
  }
}


string operatorToString(const Operator &op) {
  switch (op) {
  case Operator::EQ:
    return "==";
  case Operator::NEQ:
    return "!=";
  case Operator::LT:
    return "<";
  case Operator::LE:
    return "<=";
  case Operator::GT:
    return ">";
  case Operator::GE:
    return ">=";
  case Operator::OR:
    return "||";
  case Operator::AND:
    return "&&";
  case Operator::ADD:
    return "+";
  case Operator::SUB:
    return "-";
  case Operator::MUL:
    return "*";
  case Operator::DIV:
    return "/";
  case Operator::MOD:
    return "%";
  case Operator::NEG:
    return "-";
  case Operator::NOT:
    return "!";
  case Operator::BV_AND:
    return "&";
  case Operator::BV_OR:
    return "|";
  case Operator::BV_XOR:
    return "^";
  case Operator::BV_SHL:
    return "<<";
  case Operator::BV_SHR:
    return ">>";
  case Operator::BV_NOT:
    return "~";
  case Operator::PTR_ADD:
    return "+";
  case Operator::PTR_SUB:
    return "-";
  case Operator::IMPLICIT_BV_CAST:
    return "";
  case Operator::IMPLICIT_INT_CAST:
    return "";
  case Operator::EXPLICIT_BV_CAST:
    return "(" + EXPLICIT_BV_CAST_TYPE + ")";
  case Operator::EXPLICIT_INT_CAST:
    return "(" + EXPLICIT_INT_CAST_TYPE + ")";
  case Operator::EXPLICIT_PTR_CAST:
    return "(" + EXPLICIT_PTR_CAST_TYPE + "*)";
  case Operator::EXPLICIT_UNSIGNED_CAST:
    return "(" + EXPLICIT_UNSIGNED_CAST_TYPE + ")";
  }
  throw std::invalid_argument("unsupported operator");
}


std::string expressionToString(const Expression &expression) {
  if (expression.args.size() == 0) {
    return expression.repr;
  } else if (expression.args.size() == 1) {
    return expression.repr + " " + expressionToString(expression.args[0]);
  } if (expression.args.size() == 2) {
    return "(" + expressionToString(expression.args[0]) + " " +
           expression.repr + " " +
           expressionToString(expression.args[1]) + ")";
  }
  throw std::invalid_argument("unsupported expression");
}

Expression makeIntegerConst(int n) {
  return Expression{ NodeKind::CONSTANT,
                     Type::INTEGER,
                     Operator::NONE,
                     "int",
                     std::to_string(n),
                     {} };
}

Expression wrapWithImplicitIntCast(const Expression &expression) {
  return Expression{ NodeKind::OPERATOR,
                     Type::INTEGER,
                     Operator::IMPLICIT_INT_CAST,
                     expression.rawType,
                     operatorToString(Operator::IMPLICIT_INT_CAST),
                     {expression} };
}

Expression wrapWithImplicitBVCast(const Expression &expression) {
  return Expression{ NodeKind::OPERATOR,
                     Type::BITVECTOR,
                     Operator::IMPLICIT_BV_CAST,
                     expression.rawType,
                     operatorToString(Operator::IMPLICIT_BV_CAST),
                     {expression} };
}

Expression wrapWithExplicitIntCast(const Expression &expression) {
  return Expression{ NodeKind::OPERATOR,
                     Type::INTEGER,
                     Operator::EXPLICIT_INT_CAST,
                     EXPLICIT_INT_CAST_TYPE,
                     operatorToString(Operator::EXPLICIT_INT_CAST),
                     {expression} };
}

Expression wrapWithExplicitBVCast(const Expression &expression) {
  return Expression{ NodeKind::OPERATOR,
                     Type::BITVECTOR,
                     Operator::EXPLICIT_BV_CAST,
                     EXPLICIT_BV_CAST_TYPE,
                     operatorToString(Operator::EXPLICIT_BV_CAST),
                     {expression} };
}

Expression wrapWithExplicitPtrCast(const Expression &expression) {
  return Expression{ NodeKind::OPERATOR,
                     Type::POINTER,
                     Operator::EXPLICIT_PTR_CAST,
                     EXPLICIT_PTR_CAST_TYPE,
                     operatorToString(Operator::EXPLICIT_PTR_CAST),
                     {expression} };
}

Expression wrapWithExplicitUnsignedCast(const Expression &expression) {
  return Expression{ NodeKind::OPERATOR,
                     Type::INTEGER,
                     Operator::EXPLICIT_UNSIGNED_CAST,
                     EXPLICIT_UNSIGNED_CAST_TYPE,
                     operatorToString(Operator::EXPLICIT_UNSIGNED_CAST),
                     {expression} };
}

Expression applyBoolOperator(const Operator &op, 
                             const Expression &left,
                             const Expression &right) {
  return Expression{ NodeKind::OPERATOR,
                     Type::BOOLEAN,
                     op,
                     DEFAULT_BOOLEAN_TYPE,
                     operatorToString(op),
                     {left, right} };
}


Expression makeNonNULLCheck(const Expression &pointer) {
  return applyBoolOperator(Operator::NEQ, pointer, NULL_NODE);
}

Expression makeNULLCheck(const Expression &pointer) {
  return applyBoolOperator(Operator::EQ, pointer, NULL_NODE);
}

Expression makeNonZeroCheck(const Expression &expression) {
  return applyBoolOperator(Operator::EQ, expression, makeIntegerConst(0));
}

std::string visualizeTransformationSchema(const TransformationSchema &schema) {
  switch (schema) {
  case TransformationSchema::EXPRESSION:
    return "modify expression";
  case TransformationSchema::IF_GUARD:
    return "add if guard";
  case TransformationSchema::INITIALIZATION:
    return "add array initialization";
  default:
    throw std::invalid_argument("unsupported transformation schema");
  }
}


std::string visualizeSynthesisRule(const SynthesisRule &rule) {
  switch (rule) {
  case SynthesisRule::OPERATOR:
    return "replace operator";
  case SynthesisRule::SWAPING:
    return "swap arguments";
  case SynthesisRule::SIMPLIFICATION:
    return "simplify";
  case SynthesisRule::GENERALIZATION:
    return "generalize";
  case SynthesisRule::CONCRETIZATION:
    return "concretize";
  case SynthesisRule::LOOSENING:
    return "loosen";
  case SynthesisRule::TIGHTENING:
    return "tighten";
  case SynthesisRule::NEGATION:
    return "negate";
  case SynthesisRule::NULL_CHECK:
    return "null check";
  case SynthesisRule::SUBSTITUTION:
    return "substitute";
  default:
    throw std::invalid_argument("unsupported synthesis rule");
  }
}


std::string visualizeChange(const Patch &el) {
  std::stringstream result;
  result << "\"" << expressionToString(el.app->original) << "\""
         << " ---> "
         << "\"" << expressionToString(el.modified) << "\"";
  return result.str();
}                                                          


std::string visualizeElement(const Patch &el,
                             const boost::filesystem::path &file) {
  std::stringstream result;
  result << visualizePatchID(el.id) << " "
         << visualizeTransformationSchema(el.app->schema) 
         << " [" << visualizeSynthesisRule(el.meta.rule) << "] "
         << visualizeChange(el)
         << " in " << file.string() << ":" << el.app->location.beginLine;
  return result.str();
}                                                          


Expression convertExpression(const json::Value &json) {
  string kindStr = json["kind"].GetString();
  NodeKind kind = kindByString(kindStr);
  string typeStr = json["type"].GetString();
  Type approxType;
  if (typeStr == "pointer") {
    approxType = Type::POINTER;
  } else {
    approxType = Type::INTEGER;
  }
  string rawType = json["rawType"].GetString();
  Type type;
  Operator op;
  string repr = json["repr"].GetString();
  vector<Expression> args;
  if (json.HasMember("args")) {
    const auto &arguments = json["args"].GetArray();
    assert(kind == NodeKind::OPERATOR);
    if (arguments.Size() == 1) {
      op = unaryOperatorByString(repr);
      type = operatorOutputType(op);
    } else if (arguments.Size() == 2) {
      op = binaryOperatorByString(repr, approxType);
      type = operatorOutputType(op);      
    } else {
      throw parse_error("unsupported arguments size: " + arguments.Size());
    }
    for (auto &arg : arguments) {
      args.push_back(convertExpression(arg));
    }
  } else {
    op = Operator::NONE;
    type = approxType;
  }
  
  return Expression{kind, type, op, rawType, repr, args};
}

vector<shared_ptr<SchemaApplication>> loadSAFile(const fs::path &path) {
  vector<shared_ptr<SchemaApplication>> result;
  json::Document d;
  {
    fs::ifstream ifs(path);
    json::IStreamWrapper isw(ifs);
    d.ParseStream(isw);
  }

  for (auto &app : d.GetArray()) {
    unsigned long appId = app["appId"].GetUint();

    TransformationSchema schema = transformationSchemaByString(app["schema"].GetString());

    Location location {
      app["location"]["fileId"].GetUint(),
      app["location"]["beginLine"].GetUint(),
      app["location"]["beginColumn"].GetUint(),
      app["location"]["endLine"].GetUint(),
      app["location"]["endColumn"].GetUint()
    };

    LocationContext context;
    string contextStr = app["context"].GetString();
    if (contextStr == "condition") {
      context = LocationContext::CONDITION;
    } else {
      context = LocationContext::UNKNOWN;
    }

    Expression expression = convertExpression(app["expression"]);

    vector<Expression> components;
    vector<string> completePointeeTypes;
    for (auto &c : app["components"].GetArray()) {
      Expression comp = convertExpression(c);
      components.push_back(comp);
      string typeStr = c["type"].GetString();
      bool completePointeeType = false;
      if (typeStr == "pointer") {
        completePointeeType = ! ((bool) c["incomplete"].GetBool());
      }
      if (completePointeeType && std::find(completePointeeTypes.begin(), completePointeeTypes.end(), comp.rawType) == completePointeeTypes.end()) {
        completePointeeTypes.push_back(comp.rawType);
      }
    }

    std::stable_sort(completePointeeTypes.begin(), completePointeeTypes.end());

    shared_ptr<SchemaApplication> sa(new SchemaApplication{ appId,
                                                            schema,
                                                            location,
                                                            context,
                                                            expression,
                                                            components,
                                                            completePointeeTypes });
    result.push_back(sa);
  }

  return result;
}


vector<shared_ptr<SchemaApplication>> loadSchemaApplications(const vector<fs::path> &paths) {
  vector<shared_ptr<SchemaApplication>> result;
  for (auto &path : paths) {
    vector<shared_ptr<SchemaApplication>> singleFile = loadSAFile(path);
    result.insert(result.end(), singleFile.begin(), singleFile.end());
  }
  return result;
}


bool isExecutable(const char *file)
{
  struct stat  st;

  if (stat(file, &st) < 0)
    return false;
  if ((st.st_mode & S_IEXEC) != 0)
    return true;
  return false;
}

boost::filesystem::path relativeTo(boost::filesystem::path from, boost::filesystem::path to) {
  // Start at the root path and while they are the same then do nothing then when they first
  // diverge take the remainder of the two path and replace the entire from path with ".."
  // segments.
  boost::filesystem::path::const_iterator fromIter = from.begin();
  boost::filesystem::path::const_iterator toIter = to.begin();

  // Loop through both
  while (fromIter != from.end() && toIter != to.end() && (*toIter) == (*fromIter)) {
    ++toIter;
    ++fromIter;
  }

  boost::filesystem::path finalPath;
  while (fromIter != from.end()) {
    finalPath /= "..";
    ++fromIter;
  }

  while (toIter != to.end()) {
    finalPath /= *toIter;
    ++toIter;
  }

  return finalPath;
}


std::string prettyPrintTests(const std::vector<std::string> &tests) {
  std::stringstream printTests;
  printTests << "[";
  bool firstTest = true;
  for (int i=0; i<std::min(tests.size(), MAX_PRINT_TESTS); i++) {
    if (!firstTest)
      printTests << ", ";
    firstTest = false;
    printTests << tests[i];
  }
  if (MAX_PRINT_TESTS < tests.size())
    printTests << ", ...";
  printTests << "]";
  return printTests.str();
}


void dumpSearchSpace(vector<Patch> &searchSpace,
                     const fs::path &file,
                     const vector<fs::path> &files,
                     std::unordered_map<PatchID, double> &cost) {
  fs::ofstream os(file);
  for (auto &el : searchSpace) {
    os << std::setprecision(3) << cost[el.id] << " " 
       << visualizeElement(el, files[el.app->location.fileId]) << "\n";
  }
}
