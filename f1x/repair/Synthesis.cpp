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

#include <algorithm>
#include <sstream>
#include <stack>
#include <string>
#include <sys/types.h>
#include <unistd.h>
#include <unordered_map>

#include "Synthesis.h"
#include "Runtime.h"
#include "Typing.h"
#include "Global.h"

#include <boost/log/trivial.hpp>
#include <boost/filesystem/fstream.hpp>

namespace fs = boost::filesystem;

using std::pair;
using std::make_pair;
using std::vector;
using std::string;
using std::shared_ptr;
using std::unordered_map;
using std::to_string;
using std::stack;


const string ID_TYPE = "unsigned long";
const string PARAMETER_TYPE = ID_TYPE; // because it is passed through ID


const Expression PARAMETER_NODE = Expression{ NodeKind::PARAMETER,
                                              Type::INTEGER,
                                              Operator::NONE,
                                              PARAMETER_TYPE,
                                              "param_value",
                                              {} };

const Expression INT2_NODE = Expression{ NodeKind::INT2,
                                         Type::INTEGER,
                                         Operator::NONE,
                                         EXPLICIT_INT_CAST_TYPE,
                                         "int2_value",
                                         {} };

const Expression BOOL2_NODE = Expression{ NodeKind::BOOL2,
                                          Type::BOOLEAN,
                                          Operator::NONE,
                                          DEFAULT_BOOLEAN_TYPE,
                                          "bool2_value",
                                          {} };

const Expression COND3_NODE = Expression{ NodeKind::COND3,
                                          Type::BOOLEAN,
                                          Operator::NONE,
                                          DEFAULT_BOOLEAN_TYPE,
                                          "cond3_value",
                                          {} };

/*
 This module is split into two namespaces:
 - synthesis for everything specific to expression synthesis 
 - generator for runtime code generation
 */

namespace synthesis {

  // size of simple (atomic) modification
  const unsigned long ATOMIC_EDIT = 1;

  vector<Operator> mutateNumericOperator(const Operator &op) {
    assert(op != Operator::NONE);
    switch (op) {
    case Operator::EQ:
      return { Operator::NEQ, Operator::LT, Operator::LE, Operator::GT, Operator::GE };
    case Operator::NEQ:
      return { Operator::EQ, Operator::LT, Operator::LE, Operator::GT, Operator::GE };
    case Operator::LT:
      return { Operator::EQ, Operator::NEQ, Operator::LE, Operator::GT, Operator::GE };
    case Operator::LE:
      return { Operator::EQ, Operator::NEQ, Operator::LT, Operator::GT, Operator::GE };
    case Operator::GT:
      return { Operator::EQ, Operator::NEQ, Operator::LT, Operator::LE, Operator::GE };
    case Operator::GE:
      return { Operator::EQ, Operator::NEQ, Operator::LT, Operator::LE, Operator::GT };
    case Operator::OR:
      return { Operator::AND };
    case Operator::AND:
      return { Operator::OR };
    case Operator::ADD:
      return { Operator::SUB, Operator::MUL }; // NOTE: no Operator::DIV and Operator::MOD
    case Operator::SUB:
      return { Operator::ADD, Operator::MUL };
    case Operator::MUL:
      return { Operator::ADD, Operator::SUB };
    case Operator::DIV:
      return { Operator::ADD, Operator::SUB, Operator::MUL };
    case Operator::MOD:
      return { Operator::ADD, Operator::SUB, Operator::MUL };
    case Operator::NEG:
      return {};
    case Operator::NOT:
      return {};
    case Operator::BV_AND:
      return { Operator::BV_OR, Operator::BV_XOR };
    case Operator::BV_OR:
      return { Operator::BV_AND, Operator::BV_XOR };
    case Operator::BV_XOR:
      return { Operator::BV_AND, Operator::BV_OR };
    case Operator::BV_SHL:
      return { Operator::BV_SHR };
    case Operator::BV_SHR:
      return { Operator::BV_SHL };
    case Operator::BV_NOT:
      return {};
    case Operator::IMPLICIT_BV_CAST:
    case Operator::IMPLICIT_INT_CAST:
    case Operator::EXPLICIT_BV_CAST:
    case Operator::EXPLICIT_INT_CAST:
    case Operator::EXPLICIT_UNSIGNED_CAST:
    case Operator::EXPLICIT_PTR_CAST:
      return {};
    }
    throw std::invalid_argument("unsupported operator: " + operatorToString(op));
  }

  vector<Operator> mutatePointerOperator(const Operator &op) {
    assert(op != Operator::NONE);
    switch (op) {
    case Operator::EQ:
      return { Operator::NEQ };
    case Operator::NEQ:
      return { Operator::EQ };
    case Operator::PTR_ADD:
      return { Operator::PTR_SUB };
    case Operator::PTR_SUB:
      return { Operator::PTR_ADD };
    }
    return {};
  }

  unsigned long operatorWeight(const Operator &op) {
    assert(op != Operator::NONE);
    switch (op) {
    case Operator::IMPLICIT_BV_CAST:
    case Operator::IMPLICIT_INT_CAST:
    case Operator::EXPLICIT_BV_CAST:
    case Operator::EXPLICIT_INT_CAST:
    case Operator::EXPLICIT_UNSIGNED_CAST:
    case Operator::EXPLICIT_PTR_CAST:
      return 0;
    default:
      return 1;
    } 
  }

  unsigned long expressionDepth(const Expression &expression) {
    if (expression.kind == NodeKind::VARIABLE ||
        expression.kind == NodeKind::CONSTANT ||
        expression.kind == NodeKind::PARAMETER ||
        expression.kind == NodeKind::DEREFERENCE) {
      return 1;
    } else if (expression.kind == NodeKind::BOOL2 ||
               expression.kind == NodeKind::INT2 ||
               expression.kind == NodeKind::COND3) {
      return 2; //NODE: COND3 can be either 2 or 3, but this is unimportant
    } else if (expression.kind == NodeKind::OPERATOR) {
      unsigned long max = 0;
      for (auto &arg : expression.args) {
        unsigned long depth = expressionDepth(arg);
        if (max < depth)
          max = depth;
      }
      return max + operatorWeight(expression.op);
    }
    throw std::invalid_argument("unsupported node kind");
  }

  // simplifiable are those that have arguments of the same type as the result
  bool isSimplifiable(const Expression &expr) {
    //TODE: not and bv_not are not here, because they represent a separate modification kind
    switch (expr.op) {
    case Operator::OR:
    case Operator::AND:
    case Operator::ADD:
    case Operator::SUB:
    case Operator::MUL:
    case Operator::DIV:
    case Operator::MOD:
    case Operator::NEG:
    case Operator::BV_AND:
    case Operator::BV_OR:
    case Operator::BV_XOR:
    case Operator::BV_SHL:
    case Operator::BV_SHR:
      return true;
    default:
      return false;
    }
  }

  Expression substituteArg(const Expression &expr, const Expression &subs) {
    return Expression{expr.kind, expr.type, expr.op, expr.rawType, expr.repr, {subs}};
  }


  Expression substituteLeftArg(const Expression &expr, const Expression &subs) {
    return Expression{expr.kind, expr.type, expr.op, expr.rawType, expr.repr, {subs, expr.args[1]}};
  }


  Expression substituteRightArg(const Expression &expr, const Expression &subs) {
    return Expression{expr.kind, expr.type, expr.op, expr.rawType, expr.repr, {expr.args[0], subs}};
  }

  vector<Expression> bool2Expressions(const vector<Expression> &components) {
    vector<Expression> result;
    for (auto &left : components) {
      switch (left.type) {
      case Type::POINTER:
        result.push_back(makeNULLCheck(left));
        result.push_back(makeNonNULLCheck(left));
        for (auto &right : components) {
          if (right.type == Type::POINTER && left.rawType == right.rawType) {
            result.push_back(applyBoolOperator(Operator::EQ, left, right));
            result.push_back(applyBoolOperator(Operator::NEQ, left, right));
          }
        }
        break;
      case Type::INTEGER:
        result.push_back(applyBoolOperator(Operator::EQ, left, PARAMETER_NODE));
        result.push_back(applyBoolOperator(Operator::NEQ, left, PARAMETER_NODE));
        result.push_back(applyBoolOperator(Operator::LT, left, PARAMETER_NODE));
        result.push_back(applyBoolOperator(Operator::LE, left, PARAMETER_NODE));
        result.push_back(applyBoolOperator(Operator::GT, left, PARAMETER_NODE));
        result.push_back(applyBoolOperator(Operator::GE, left, PARAMETER_NODE));
        for (auto &right : components) {
          if (right.type == Type::INTEGER) {
            result.push_back(applyBoolOperator(Operator::EQ, left, right));
            result.push_back(applyBoolOperator(Operator::NEQ, left, right));
            result.push_back(applyBoolOperator(Operator::LT, left, right));
            result.push_back(applyBoolOperator(Operator::LE, left, right));
            result.push_back(applyBoolOperator(Operator::GT, left, right));
            result.push_back(applyBoolOperator(Operator::GE, left, right));
          }
        }
        break;
      default:
        throw std::invalid_argument("unsupported component type");
      }
    }
    return result;
  }

  unsigned long substitutionDistance(const Expression &from, const Expression &to) {
    return expressionDepth(from) + expressionDepth(to) - 1;
  }

  vector<pair<Expression, PatchMetadata>> baseSubstitutions(const Expression &expr,
                                                           const vector<Expression> &components) {
    vector<pair<Expression, PatchMetadata>> result;
    if (expr.type == Type::BOOLEAN) {
      //TODO: distance computation
      auto meta = PatchMetadata{SynthesisRule::SUBSTITUTION, substitutionDistance(expr, BOOL2_NODE)};
      result.push_back(make_pair(BOOL2_NODE, meta));
    }
    if (expr.args.size() == 0) {
      if (expr.kind == NodeKind::CONSTANT) {
        if (expr.type == Type::INTEGER) {
          for (auto &c : components) {
            if (c.type == Type::INTEGER) {
              auto meta = PatchMetadata{SynthesisRule::GENERALIZATION, ATOMIC_EDIT};
              result.push_back(make_pair(c, meta));
            }
          }
          auto meta = PatchMetadata{SynthesisRule::SUBSTITUTION, ATOMIC_EDIT};
          result.push_back(make_pair(PARAMETER_NODE, meta));
        }
      } else if (expr.kind == NodeKind::VARIABLE ||
                 expr.kind == NodeKind::DEREFERENCE) {
        if (expr.type == Type::INTEGER) {
          for (auto &c : components) {
            if (c.type == Type::INTEGER) {
              auto meta = PatchMetadata{SynthesisRule::SUBSTITUTION, ATOMIC_EDIT};
              result.push_back(make_pair(c, meta));
            }
          }
          auto meta = PatchMetadata{SynthesisRule::CONCRETIZATION, ATOMIC_EDIT};
          result.push_back(make_pair(PARAMETER_NODE, meta));
        }
        if (expr.type == Type::POINTER) {
          for (auto &c : components) {
            if (c.type == Type::POINTER && expr.rawType == c.rawType) {
              auto meta = PatchMetadata{SynthesisRule::SUBSTITUTION, ATOMIC_EDIT};
              result.push_back(make_pair(c, meta));
            }
          }
          auto meta = PatchMetadata{SynthesisRule::CONCRETIZATION, ATOMIC_EDIT};
          result.push_back(make_pair(NULL_NODE, meta));
        }
      }
    } else {
      vector<Operator> oms;
      if (expr.args[0].type == Type::POINTER) {
        oms = mutatePointerOperator(expr.op);
      } else {
        oms = mutateNumericOperator(expr.op);
      }
      for (auto &m : oms) {
        Expression e = expr;
        e.op = m;
        e.repr = operatorToString(m);
        auto meta = PatchMetadata{SynthesisRule::OPERATOR, ATOMIC_EDIT};
        result.push_back(make_pair(std::move(e), meta));
      }

      if (expr.args.size() == 1) {
        vector<pair<Expression, PatchMetadata>> argMods = baseSubstitutions(expr.args[0], components);
        for (auto &m : argMods) {
          result.push_back(make_pair(substituteArg(expr, m.first), m.second));
        }
        if (isSimplifiable(expr)) {
          Expression argCopy = expr.args[0];
          //TODO: distance computation
          auto meta = PatchMetadata{SynthesisRule::SIMPLIFICATION, ATOMIC_EDIT};
          result.push_back(make_pair(argCopy, meta));
        }
      } else if (expr.args.size() == 2) {
        vector<pair<Expression, PatchMetadata>> leftMods;
        if (expr.op != Operator::PTR_ADD && expr.op != Operator::PTR_SUB) {
          leftMods = baseSubstitutions(expr.args[0], components);
        }
        for (auto &m : leftMods) {
          result.push_back(make_pair(substituteLeftArg(expr, m.first), m.second));
        }
        vector<pair<Expression, PatchMetadata>> rightMods = baseSubstitutions(expr.args[1], components);
        for (auto &m : rightMods) {
          result.push_back(make_pair(substituteRightArg(expr, m.first), m.second));
        }
        if (expr.args[0].type == Type::INTEGER) {
          auto meta = PatchMetadata{SynthesisRule::UNSIGNED_CAST, ATOMIC_EDIT};
          result.push_back(make_pair(substituteLeftArg(expr, wrapWithExplicitUnsignedCast(expr.args[0])), meta));
        }
        if (expr.args[1].type == Type::INTEGER) {
          auto meta = PatchMetadata{SynthesisRule::UNSIGNED_CAST, ATOMIC_EDIT};
          result.push_back(make_pair(substituteRightArg(expr, wrapWithExplicitUnsignedCast(expr.args[1])), meta));
        }
        if (isSimplifiable(expr)) {
          Expression leftCopy = expr.args[0];
          Expression rightCopy = expr.args[1];
          //TODO: distance computation
          auto leftMeta = PatchMetadata{SynthesisRule::SIMPLIFICATION, expressionDepth(rightCopy)};
          auto rightMeta = PatchMetadata{SynthesisRule::SIMPLIFICATION, expressionDepth(leftCopy)};
          result.push_back(make_pair(leftCopy, leftMeta));
          result.push_back(make_pair(rightCopy, rightMeta));
        }
      }
    }
    return result;
  }

  vector<pair<Expression, PatchMetadata>> baseModifications(const TransformationSchema &schema,
                                                            const Expression &expr,
                                                            const vector<Expression> &components) {
    vector<pair<Expression, PatchMetadata>> baseModifications;
    auto bool2Meta = PatchMetadata{SynthesisRule::SUBSTITUTION, expressionDepth(BOOL2_NODE)};
    switch (schema) {
    case TransformationSchema::EXPRESSION:
      baseModifications = baseSubstitutions(expr, components);
      if (expr.type == Type::BOOLEAN) {
        auto looseningMeta = PatchMetadata{SynthesisRule::LOOSENING, expressionDepth(BOOL2_NODE)};
        baseModifications.push_back(make_pair(applyBoolOperator(Operator::OR, expr, BOOL2_NODE), looseningMeta));
        auto tighteningMeta = PatchMetadata{SynthesisRule::TIGHTENING, expressionDepth(BOOL2_NODE)};
        baseModifications.push_back(make_pair(applyBoolOperator(Operator::AND, expr, BOOL2_NODE), tighteningMeta));
      }
      break;
    case TransformationSchema::IF_GUARD:
      if (expr.repr == TRUE_NODE.repr) {
        auto meta = PatchMetadata{SynthesisRule::SUBSTITUTION, ATOMIC_EDIT};
        baseModifications.push_back(make_pair(FALSE_NODE, meta));
      }
      if (expr.repr == FALSE_NODE.repr) {
        auto meta = PatchMetadata{SynthesisRule::SUBSTITUTION, ATOMIC_EDIT};
        baseModifications.push_back(make_pair(TRUE_NODE, meta));
      }
      //TODO: compute distance
      baseModifications.push_back(make_pair(BOOL2_NODE, bool2Meta));
      break;
    case TransformationSchema::LOOSENING:
    case TransformationSchema::TIGHTENING:
      //TODO: compute distance
      
      baseModifications.push_back(make_pair(BOOL2_NODE, bool2Meta)); //TODO: should be COND3
      break;
    }
    return baseModifications;
  }

}

namespace generator {

  const string POINTER_ARG_NAME = "__ptr_vals";
  const string SIZES_ARG_NAME = "__ptr_sizes";
  const string NULLDEREF_ARG_NAME = "__nullderef";

  void substituteWithRuntimeRepr(Expression &expression,
                                 unordered_map<string, string> &runtimeReprBySource) {
    if (expression.kind == NodeKind::VARIABLE ||
        expression.kind == NodeKind::DEREFERENCE) {
      expression.repr = runtimeReprBySource[expression.repr];
    } else {
      for (auto &arg : expression.args) {
        substituteWithRuntimeRepr(arg, runtimeReprBySource);
      }
    }
  }

  string locationNameSuffix(Location loc) {
    std::ostringstream result;
    result << loc.fileId << "_"
           << loc.beginLine << "_"
           << loc.beginColumn << "_"
           << loc.endLine << "_"
           << loc.endColumn;
    return result.str();
  }

  string argNameByNonPtrType(const string &typeName) {
    string result = typeName;
    std::replace(result.begin(), result.end(), ' ', '_');
    return "__" + result + "_vals";
  }

  void runtimeLoader(std::ostream &OUT) {
    OUT << "struct __f1xid_t {" << "\n"
        << ID_TYPE << " base;" << "\n"
        << ID_TYPE << " int2;" << "\n"
        << ID_TYPE << " bool2;" << "\n"
        << ID_TYPE << " cond3;" << "\n"
        << ID_TYPE << " param;" << "\n"
        << "};" << "\n";

    OUT << ID_TYPE << " __f1xapp = strtoul(getenv(\"F1X_APP\"), (char **)NULL, 10);" << "\n"
        << ID_TYPE << " __f1xid_base = strtoul(getenv(\"F1X_ID_BASE\"), (char **)NULL, 10);" << "\n"
        << ID_TYPE << " __f1xid_int2 = strtoul(getenv(\"F1X_ID_INT2\"), (char **)NULL, 10);" << "\n"
        << ID_TYPE << " __f1xid_bool2 = strtoul(getenv(\"F1X_ID_BOOL2\"), (char **)NULL, 10);" << "\n"
        << ID_TYPE << " __f1xid_cond3 = strtoul(getenv(\"F1X_ID_COND3\"), (char **)NULL, 10);" << "\n"
        << ID_TYPE << " __f1xid_param = strtoul(getenv(\"F1X_ID_PARAM\"), (char **)NULL, 10);" << "\n"
        << "__f1xid_t *__f1xids = NULL;\n" 
        << "int * result_map;"
        << "\n";

    OUT << "void __f1x_init_runtime() {" << "\n";
    if (cfg.valueTEQ) {
      OUT << "int fd = shm_open(\"" << PARTITION_FILE_NAME << "_" << geteuid()
          << "\", O_RDWR, 0);"
          << "\n"
          << "struct stat sb;"
          << "\n"
          << "fstat(fd, &sb);"
          << "\n"
          << "__f1xids = (__f1xid_t*) mmap(NULL, sb.st_size, PROT_READ | "
             "PROT_WRITE, MAP_SHARED, fd, 0);"
          << "\n"
          << "close(fd);"
          << "\n\n"
          << "int fd2 = shm_open(\"/f1x_result\", O_CREAT|O_RDWR, (mode_t)0666);\n"
          << "ftruncate(fd2, sizeof(int));\n"
          << "result_map = (int*) mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, fd2, 0);\n"
          << "close(fd2);\n"
          << "\n";
    }
    OUT << "}" << "\n";
  }


  string parameterList(shared_ptr<SchemaApplication> sa) {
    std::ostringstream result;

    vector<string> types;
    bool hasPointers = false;
    bool hasDereferences = false;
    for (auto &c : sa->components) {
      if (c.kind == NodeKind::DEREFERENCE)
        hasDereferences = true;
      if (c.type == Type::INTEGER) {
        if(std::find(types.begin(), types.end(), c.rawType) == types.end()) {
          types.push_back(c.rawType);
        }
      } else {
        hasPointers = true;
      }
    }

    std::stable_sort(types.begin(), types.end());

    bool firstArray = true;
    for (auto &type : types) {
      if (firstArray) {
        firstArray = false;
      } else {
        result << ", ";
      }
      result << type << " " << argNameByNonPtrType(type) << "[]";
    }
    if (hasPointers) {
      if (firstArray) {
        firstArray = false;
      } else {
        result << ", ";
      }
      result << "void *" << POINTER_ARG_NAME << "[], ";
      result << "int " << SIZES_ARG_NAME << "[]";
    }
    if (hasDereferences) {
      if (firstArray) {
        firstArray = false;
      } else {
        result << ", ";
      }
      result << "int " << NULLDEREF_ARG_NAME << "[]";
    }
  
    return result.str();
  }


  unordered_map<string, string> runtimeRenaming(shared_ptr<SchemaApplication> sa) {
    vector<string> nonPtrTypes;
    vector<Expression> pointers;
    for (auto &c : sa->components) {
      switch (c.type) {
      case Type::INTEGER:
        if(std::find(nonPtrTypes.begin(), nonPtrTypes.end(), c.rawType) == nonPtrTypes.end())
          nonPtrTypes.push_back(c.rawType);
        break;
      case Type::POINTER:
        pointers.push_back(c);
        break;
      default:
        throw std::invalid_argument("unsupported component type");
      }
    }

    std::stable_sort(nonPtrTypes.begin(), nonPtrTypes.end());

    unordered_map<string, vector<string>> reprsByType;
    for (auto &type : nonPtrTypes) {
      reprsByType[type] = vector<string>();
    }
    for (auto &c : sa->components) {
      if (c.type == Type::INTEGER) {
        reprsByType[c.rawType].push_back(c.repr);
      }
    }

    unordered_map<string, string> runtimeReprBySource;
    for (auto &type : nonPtrTypes) {
      auto reprs = reprsByType[type];
      for (int index = 0; index < reprs.size(); index++) {
        runtimeReprBySource[reprs[index]] =
          argNameByNonPtrType(type) + "[" + to_string(index) + "]";
      }
    }
    for (int index = 0; index < pointers.size(); index++) {
      runtimeReprBySource[pointers[index].repr] =
        POINTER_ARG_NAME + "[" + to_string(index) + "]";
    }
    
    return runtimeReprBySource;
  }

  unordered_map<string, string> typeSizes(shared_ptr<SchemaApplication> sa) {
    unordered_map<string, string> sizeByType;
    for (int index = 0; index < sa->completePointeeTypes.size(); index++) {
      sizeByType[sa->completePointeeTypes[index]] =
        SIZES_ARG_NAME + "[" + to_string(index) + "]";
    }

    return sizeByType;
  }

  unordered_map<string, string> nullDerefCondition(shared_ptr<SchemaApplication> sa,
                                                   unordered_map<string, string> &runtimeReprBySource) {
    vector<string> dereferences;
    for (auto &c : sa->components) {
      if (c.kind == NodeKind::DEREFERENCE) {
        if(std::find(dereferences.begin(), dereferences.end(), c.repr) == dereferences.end())
          dereferences.push_back(c.repr);
      }
    }

    std::stable_sort(dereferences.begin(), dereferences.end());

    unordered_map<string, string> nullDerefByName;
    for (int index = 0; index < dereferences.size(); index++) {
      nullDerefByName[runtimeReprBySource[dereferences[index]]] =
        NULLDEREF_ARG_NAME + "[" + to_string(index) + "]";
    }

    return nullDerefByName;
  }

  bool isAbstractExpression(const Expression &expression) {
    if (isAbstractNode(expression.kind)) {
      return true;
    } else {
      for (auto &arg : expression.args) {
        if(isAbstractExpression(arg))
          return true;
      }
    }
    return false;
  }


  bool hasNodeOfKind(const Expression &expression, const NodeKind &kind) {
    if (expression.kind == kind) {
      return true;
    } else {
      for (auto &arg : expression.args) {
        if(hasNodeOfKind(arg, kind))
          return true;
      }
    }
    return false;
  }

  bool substituteNodeOfKind(Expression &expression,
                            NodeKind kind, 
                            const Expression &substitution) {
    if (expression.kind == kind) {
      expression = substitution;
      return true;
    } else {
      for (auto &arg : expression.args) {
        if(substituteNodeOfKind(arg, kind, substitution))
          return true;
      }
    }
    return false;
  }

  string runtimeSemantics(const Expression &expression,
                          unordered_map<string, string> &sizeByType,
                          unordered_map<string, string> &nullDerefByName) {
    if (expression.op == Operator::PTR_ADD ||
        expression.op == Operator::PTR_SUB) {
      string sizeExpr = sizeByType[expression.args[0].rawType];
      std::ostringstream result;
      result << "(void*) ("
             << "(std::size_t) " << runtimeSemantics(expression.args[0], sizeByType, nullDerefByName)
             << " " << operatorToString(expression.op) << " "
             << sizeExpr << " * " << runtimeSemantics(expression.args[1], sizeByType, nullDerefByName)
             << ")";
      return result.str();
    } else if (expression.op == Operator::DIV ||
               expression.op == Operator::MOD) {
      string denominator = runtimeSemantics(expression.args[1], sizeByType, nullDerefByName);
      std::ostringstream result;
      result << "(" << denominator << " != 0 ? "
             << runtimeSemantics(expression.args[0], sizeByType, nullDerefByName)
             << " " << operatorToString(expression.op) << " "
             << denominator
             << " : ({ current_panic = true; 0; })"
             << ")";
      return result.str();
    } else {
      if (expression.args.size() == 0) {
        if (expression.kind == NodeKind::DEREFERENCE) {
          
          std::ostringstream result;
          result << "(" << nullDerefByName[expression.repr]
                 << " ? ({ current_panic = true; " << (expression.type == Type::POINTER ? "(void*) 0" : "0") << "; })"
                 << " : " << expression.repr
                 << ")";
          return result.str();
        } else {
          return expression.repr;
        }
      } else if (expression.args.size() == 1) {
        return expression.repr + " " + runtimeSemantics(expression.args[0], sizeByType, nullDerefByName);
      } if (expression.args.size() == 2) {
        return "(" + runtimeSemantics(expression.args[0], sizeByType, nullDerefByName) + " " +
          expression.repr + " " +
          runtimeSemantics(expression.args[1], sizeByType, nullDerefByName) + ")";
      }
      throw std::invalid_argument("unsupported expression");
    }
  }

  vector<pair<PatchID, Expression>> generateParameterInstances(const PatchID &partialId,
                                                               const Expression &expression,
                                                               const unsigned long &paramBound) {
    vector<pair<PatchID, Expression>> result;
    return result;
  }

  void candidateDispatch(shared_ptr<SchemaApplication> sa,
                                unsigned long &baseId,
                                std::ostream &OS,
                                vector<Patch> &ss,
                                unordered_map<string, string> &runtimeReprBySource) {
    runtimeReprBySource = runtimeRenaming(sa);
    unordered_map<string, string> sizeByType = typeSizes(sa);
    unordered_map<string, string> nullDerefByName = nullDerefCondition(sa, runtimeReprBySource);

    unsigned long paramBound;
    if (sa->context == LocationContext::CONDITION) {
      paramBound = cfg.maxConditionParameter;
    } else {
      paramBound = cfg.maxExpressionParameter;
    }

    OS << "param_value = id.param;" << "\n";

    OS << "switch (id.bool2) {" << "\n"
       << "case 0:" << "\n"
       << "break;" << "\n";
    vector<Expression> bool2Expressions =
      synthesis::bool2Expressions(sa->components);
    for (int i = 0; i < bool2Expressions.size(); i++) {
      Expression runtimeExpr = bool2Expressions[i];
      substituteWithRuntimeRepr(runtimeExpr, runtimeReprBySource);
      OS << "case " << (i + 1) << ":" << "\n" // 0 means disabled
         << "bool2_value = " << runtimeSemantics(runtimeExpr, sizeByType, nullDerefByName) << ";" << "\n"
         << "break;" << "\n";
    }
    OS << "}" << "\n";

    vector<pair<Expression, PatchMetadata>> baseModifications =
      synthesis::baseModifications(sa->schema, sa->original, sa->components);

    OS << "switch (id.base) {" << "\n";

    for (auto &candidate : baseModifications) {
      Expression runtimeExpr = candidate.first;
      PatchMetadata metadata = candidate.second;
      substituteWithRuntimeRepr(runtimeExpr, runtimeReprBySource);

      PatchID partialId{0};
      partialId.base = baseId;

      OS << "case " << partialId.base << ":" << "\n"
         << "base_value = " << runtimeSemantics(runtimeExpr, sizeByType, nullDerefByName) << ";" << "\n"
         << "break;" << "\n";
      
      stack<pair<PatchID, Expression>> parametrizedCandidates;
      parametrizedCandidates.push(std::make_pair(partialId, candidate.first));
      
      while (!parametrizedCandidates.empty()) {
        pair<PatchID, Expression> current = parametrizedCandidates.top();
        parametrizedCandidates.pop();
        if (hasNodeOfKind(current.second, NodeKind::BOOL2)) {
          for (int i = 0; i < bool2Expressions.size(); i++) {
            PatchID instanceId = current.first;
            instanceId.bool2 = i + 1; // 0 means disabled
            Expression instance = current.second;
            substituteNodeOfKind(instance, NodeKind::BOOL2, bool2Expressions[i]);
            parametrizedCandidates.push(std::make_pair(instanceId, instance));
          }
        } else if (hasNodeOfKind(current.second, NodeKind::PARAMETER)) {
          for (int i = 0; i <= paramBound; i++) {
            PatchID instanceId = current.first;
            instanceId.param = i;
            Expression instance = current.second;
            substituteNodeOfKind(instance, NodeKind::PARAMETER, makeIntegerConst(i));
            ss.push_back(Patch{instanceId, sa, instance, metadata});
          }
        } else {
          ss.push_back(Patch{current.first, sa, current.second, metadata});
        }
      }
      
      baseId++;
    }

    OS << "}" << "\n";
  }

  void partitioningFunctions(const vector<shared_ptr<SchemaApplication>> &schemaApplications,
                             std::ostream &OS,
                             vector<Patch> &searchSpace) {

    OS << "#include \"rt.h\"" << "\n"
       << "#include <stdlib.h>" << "\n"
       << "#include <vector>" << "\n"
       << "#include <cstddef>" << "\n"
       << "#include <unistd.h>" << "\n"
       << "#include <fcntl.h>" << "\n"
       << "#include <sys/stat.h>" << "\n"
       << "#include <sys/mman.h>" << "\n";
//added by gaoxiang, to show the modified location is executed
//    OS << "#include \"SharedMemorySetter.h\"" << "\n";
//end


    generator::runtimeLoader(OS);

    unsigned long baseId = 1; // because 0 is reserved:

    for (auto sa : schemaApplications) {
      string outputType;
      if (sa->original.type == Type::POINTER) {
        outputType= "void*";
      } else {
        outputType = sa->original.rawType;
      }

      OS << outputType << " __f1x_"
         << locationNameSuffix(sa->location)
         << "(" << generator::parameterList(sa) << ")"
         << "{" << "\n";

      OS << "__f1xid_t id;" << "\n"
         << "id.base = __f1xid_base;" << "\n"
         << "id.int2 = __f1xid_int2;" << "\n"
         << "id.bool2 = __f1xid_bool2;" << "\n"
         << "id.cond3 = __f1xid_cond3;" << "\n"
         << "id.param = __f1xid_param;" << "\n";

      OS << outputType << " base_value;" << "\n"
         << EXPLICIT_INT_CAST_TYPE << " int2_value;" << "\n"
         << "bool bool2_value;" << "\n"
         << "bool cond3_value;" << "\n"
         << PARAMETER_TYPE << " param_value;" << "\n";

      OS << outputType << " output_value = 0;" << "\n"
         << "bool output_initialized = false;" << "\n"
         << "unsigned long input_index = 0;" << "\n"
         << "unsigned long output_index = 0;" << "\n"
         << "bool output_panic = false;" << "\n"
         << "bool current_panic = false;" << "\n";

      if (cfg.valueTEQ) {
        OS << "if (__f1xids == NULL) __f1x_init_runtime();" << "\n";
      }

      OS << "label_" << locationNameSuffix(sa->location) << ":" << "\n";

      OS << "current_panic = false;" << "\n";

      unordered_map<string, string> runtimeReprBySource;
      generator::candidateDispatch(sa, baseId, OS, searchSpace, runtimeReprBySource);

      OS << "if (!output_initialized) {" << "\n"
         << "output_panic = current_panic;" << "\n"
         << "output_value = base_value;" << "\n"
         << "output_initialized = true;" << "\n"
         << "} else if ((output_panic && current_panic)"
         << " || (!output_panic && !current_panic && output_value == base_value)) {" << "\n";
      if (cfg.valueTEQ) {
        OS << "__f1xids[output_index] = id;" << "\n"
           << "output_index++;" << "\n";
      }
      OS << "}" << "\n";

      OS << "if (__f1xids && __f1xids[input_index].base != 0) {" << "\n"
         << "id = __f1xids[input_index];" << "\n"
         << "input_index++;" << "\n"
         << "goto " << "label_" << locationNameSuffix(sa->location) << ";" << "\n"
         << "}" << "\n";

      if (cfg.valueTEQ) {
        // output terminator:
        OS << "__f1xids[output_index] = __f1xid_t{0, 0, 0, 0, 1};" << "\n";
      }

      OS << "if (output_panic) {" << "\n"
         << "abort();" << "\n"
         << "}" << "\n";
//added by gaoxiang, to show the modified location is executed
      if(schemaApplications.size() > 0){
        Expression originalExpression = sa->original;
        BOOST_LOG_TRIVIAL(info) << "original expression is : " << expressionToString(originalExpression);
        substituteWithRuntimeRepr(originalExpression, runtimeReprBySource);
        OS << outputType << " origin_value = " << expressionToString(originalExpression) << ";\n"
           << "if(origin_value!=output_value)\n"
           << "result_map[0]=0;\n"
           << "\n";
      }
//end
      OS << "return output_value;" << "\n";

      OS << "}" << "\n";
    }

  }

}


vector<Patch> 
generateSearchSpace(const vector<shared_ptr<SchemaApplication>> &schemaApplications,
                    std::ostream &OS,
                    std::ostream &OH) {
  
  // header

  OH << "#ifdef __cplusplus" << "\n"
     << "extern \"C\" {" << "\n"
     << "#endif" << "\n"
     << "extern " << ID_TYPE << " __f1xapp;" << "\n";

  for (auto sa : schemaApplications) {
    string outputType;
    if (sa->original.type == Type::POINTER) {
      outputType= "void*";
    } else {
      outputType = sa->original.rawType;
    }

    OH << outputType << " __f1x_" 
       << generator::locationNameSuffix(sa->location)
       << "(" << generator::parameterList(sa) << ")"
       << ";" << "\n";
  }

  OH << "#ifdef __cplusplus" << "\n"
     << "}" << "\n"
     << "#endif" << "\n";

  // source

  vector<Patch> searchSpace;
  
  generator::partitioningFunctions(schemaApplications, OS, searchSpace);  

  return searchSpace;
}
