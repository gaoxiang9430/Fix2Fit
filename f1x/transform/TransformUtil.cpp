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
#include <map>
#include <stack>
#include <sstream>

#include "clang/Lex/Preprocessor.h"
#include "clang/AST/Type.h"
#include "llvm/Support/raw_ostream.h"

#include "TransformUtil.h"
#include "TransformGlobal.h"
#include "Config.h"


namespace json = rapidjson;

using namespace clang;
using namespace llvm;
using std::string;
using std::pair;
using std::stack;
using std::vector;
using std::map;


const BuiltinType::Kind DEFAULT_NUMERIC_TYPE = BuiltinType::Long;
const string DEFAULT_POINTEE_TYPE = "void";


std::shared_ptr<std::vector<clang::SourceRange>> globalConditionalsPP = std::shared_ptr<std::vector<clang::SourceRange>>(new std::vector<clang::SourceRange>());

const unsigned F1XAPP_WIDTH = 32;
const unsigned F1XAPP_VALUE_BITS = 10;

/*
  __f1xapp is a F1XAPP_WIDTH bit transparent schema application ID. The left F1XAPP_VALUE_BITS bits of this id is the file ID.
  FIXME: does it need to be so complex?
 */

unsigned long f1xapp(unsigned long baseId, unsigned fileId) {
  assert(baseId < (1 << (F1XAPP_WIDTH - F1XAPP_VALUE_BITS)));
  unsigned long result = fileId;
  result <<= (F1XAPP_WIDTH - F1XAPP_VALUE_BITS);
  result += baseId;
  return result;
}


bool inRange(unsigned line) {
  if (cfg.fromLine || cfg.toLine) {
    return cfg.fromLine <= line && line <= cfg.toLine;
  } else {
    return true;
  }
}


unsigned getDeclExpandedLine(const Decl* decl, SourceManager &srcMgr) {
  SourceLocation startLoc = decl->getLocStart();
  if(startLoc.isMacroID()) {
    // Get the start/end expansion locations
    pair<SourceLocation, SourceLocation> expansionRange = srcMgr.getExpansionRange(startLoc);
    // We're just interested in the start location
    startLoc = expansionRange.first;
  }

  return srcMgr.getExpansionLineNumber(startLoc);
}


bool insideMacro(const Stmt* expr, SourceManager &srcMgr, const LangOptions &langOpts) {
  SourceLocation startLoc = expr->getLocStart();
  SourceLocation endLoc = expr->getLocEnd();

  if(startLoc.isMacroID() && !Lexer::isAtStartOfMacroExpansion(startLoc, srcMgr, langOpts))
    return true;
  
  if(endLoc.isMacroID() && !Lexer::isAtEndOfMacroExpansion(endLoc, srcMgr, langOpts))
    return true;

  return false;
}


PPConditionalRecoder::PPConditionalRecoder(std::shared_ptr<std::vector<SourceRange>> conditionals):
  conditionals(conditionals) { }

void PPConditionalRecoder::Endif(SourceLocation Loc, SourceLocation IfLoc) {
  this->conditionals->push_back(SourceRange(IfLoc, Loc));
}


bool intersectConditionalPP(const Stmt* stmt, 
                            SourceManager &srcMgr, 
                            const std::shared_ptr<std::vector<SourceRange>> conditionalsPP) {
  BeforeThanCompare<SourceLocation> isBefore(srcMgr);

  for (auto range : *conditionalsPP) {
    // only in the main file:
    std::pair<FileID, unsigned> decLoc = srcMgr.getDecomposedExpansionLoc(range.getBegin());
    if (srcMgr.getMainFileID() != decLoc.first)
      continue;

    /*
      Matching situation like this (and the opposite):
      EXPR_BEGIN
      #ifdef
      EXPR_END
      #endif
     */
    if ((isBefore(stmt->getLocStart(), range.getBegin()) &&
         isBefore(range.getBegin(), stmt->getLocEnd()) &&
         isBefore(stmt->getLocEnd(), range.getEnd())) || 
        (isBefore(range.getBegin(), stmt->getLocStart()) && 
         isBefore(stmt->getLocStart(), range.getEnd()) && 
         isBefore(range.getEnd(), stmt->getLocEnd()))) {
      return true;
    }
  }

  return false;
}


SourceRange getExpandedLoc(const Stmt* expr, SourceManager &srcMgr) {
  SourceLocation startLoc = expr->getLocStart();
  SourceLocation endLoc = expr->getLocEnd();

  if(startLoc.isMacroID()) {
    // Get the start/end expansion locations
    pair<SourceLocation, SourceLocation> expansionRange = srcMgr.getExpansionRange(startLoc);
    // We're just interested in the start location
    startLoc = expansionRange.first;
  }
  if(endLoc.isMacroID()) {
    // Get the start/end expansion locations
    pair<SourceLocation, SourceLocation> expansionRange = srcMgr.getExpansionRange(endLoc);
    // We're just interested in the start location
    endLoc = expansionRange.second; // TODO: I am not sure about it
  }
      
  SourceRange expandedLoc(startLoc, endLoc);

  return expandedLoc;
}


string toString(const Stmt *stmt) {
  /* Special case for break and continue statement
     Reason: There were semicolon ; and newline found
     after break/continue statement was converted to string
  */
  if (dyn_cast<BreakStmt>(stmt))
    return "break";
  
  if (dyn_cast<ContinueStmt>(stmt))
    return "continue";

  LangOptions LangOpts;
  PrintingPolicy Policy(LangOpts);
  string str;
  raw_string_ostream rso(str);

  stmt->printPretty(rso, nullptr, Policy);

  string stmtStr = rso.str();
  return stmtStr;
}


bool overwriteMainChangedFile(Rewriter &TheRewriter) {
  bool AllWritten = true;
  FileID id = TheRewriter.getSourceMgr().getMainFileID();
  const FileEntry *Entry = TheRewriter.getSourceMgr().getFileEntryForID(id);
  std::error_code err_code;
  auto buffer = TheRewriter.getRewriteBufferFor(id);
  if (buffer) {// if there are modifications
    raw_fd_ostream out(Entry->getName(), err_code, sys::fs::F_None);
    buffer->write(out);
    out.close();
  }
  return !AllWritten;
}

bool isTopLevelStatement(const Stmt *stmt, ASTContext *context) {
  auto it = context->getParents(*stmt).begin();

  if(it == context->getParents(*stmt).end())
    return true;

  const CompoundStmt* cs;
  if ((cs = it->get<CompoundStmt>()) != NULL) {
    return true;
  }

  if (isChildOfNonblock(stmt, context)) {
    return true;
  }
    
  return false;
}

bool isChildOfNonblock(const Stmt *stmt, ASTContext *context) {
  auto it = context->getParents(*stmt).begin();

  const IfStmt* is;
  if ((is = it->get<IfStmt>()) != NULL) {
    return stmt != is->getCond();
  }

  const ForStmt* fs;
  if ((fs = it->get<ForStmt>()) != NULL) {
    return stmt != fs->getCond() && stmt != fs->getInc() && stmt != fs->getInit();
  }

  const WhileStmt* ws;
  if ((ws = it->get<WhileStmt>()) != NULL) {
    return stmt != ws->getCond();
  }

  return false;
}


bool inConditionContext(const Expr *expr, ASTContext *context) {
  auto it = context->getParents(*expr).begin();
  const ImplicitCastExpr* ic;
  while ((ic = it->get<ImplicitCastExpr>()) != NULL) {
    it = context->getParents(*ic).begin();
  }

  const IfStmt* is;
  if ((is = it->get<IfStmt>()) != NULL) {
    return expr == is->getCond();
  }

  const ForStmt* fs;
  if ((fs = it->get<ForStmt>()) != NULL) {
    return expr == fs->getCond();
  }

  const WhileStmt* ws;
  if ((ws = it->get<WhileStmt>()) != NULL) {
    return expr == ws->getCond();
  }

  const BinaryOperator* bo;
  if ((bo = it->get<BinaryOperator>()) != NULL) {
    return bo->getOpcode() == clang::BO_LAnd || bo->getOpcode() == clang::BO_LOr;
  }

  return false;
}


BuiltinType::Kind getBuiltinKind(const QualType &type) {
  QualType canon = type.getCanonicalType();
  assert(canon.getTypePtr());
  if (const BuiltinType *bt = canon.getTypePtr()->getAs<BuiltinType>()) {
    return bt->getKind();
  } else {
    llvm::errs() << "error: non-builtin type " << type.getAsString() << "\n";
    return DEFAULT_NUMERIC_TYPE;
  }
}

bool isPointerType(const QualType &type) {
  QualType canon = type.getCanonicalType();
  assert(canon.getTypePtr());
  return canon.getTypePtr()->isPointerType() ||
         canon.getTypePtr()->isArrayType();
}

// returns (name, isIncomplete)
pair<string, bool> getPointeeType(const QualType &type) {
  //FIXME: use getPointeeOrArrayElementType?
  if (const PointerType *pt = type.getTypePtr()->getAs<PointerType>()) {
    QualType t = pt->getPointeeType();
    bool isIncomplete = t.getTypePtr()->isIncompleteType();
    //NOTE: clang says for va_list that it is complete, but gcc fails
    if (t.getAsString() == "struct __va_list_tag")
      isIncomplete = true;
    return std::make_pair(t.getAsString(), isIncomplete);
  } else if (const ArrayType *at = dyn_cast<ArrayType>(type.getTypePtr())) {
    QualType t = at->getElementType();
    bool isIncomplete = t.getTypePtr()->isIncompleteType();
     //NOTE: clang says for va_list that it is complete, but gcc fails
    if (t.getAsString() == "struct __va_list_tag")
      isIncomplete = true;
    return std::make_pair(t.getAsString(), isIncomplete);
  } else {
    llvm::errs() << "error: non-pointer and non-array type " << type.getAsString() << "\n";
    return std::make_pair(DEFAULT_POINTEE_TYPE, true);
  }
}

// TODO: support C++ types, compiler extensions
string clangKindToString(const BuiltinType::Kind kind) {
  switch (kind) {
  case BuiltinType::Char_U:
    return "char";
  case BuiltinType::UChar:
    return "unsigned char";
  case BuiltinType::WChar_U:
    return "wchar_t";
  case BuiltinType::UShort:
    return "unsigned short";
  case BuiltinType::UInt:
    return "unsigned int";
  case BuiltinType::ULong:
    return "unsigned long";
  case BuiltinType::ULongLong:
    return "unsigned long long";
  case BuiltinType::Char_S:
    return "char";
  case BuiltinType::SChar:
    return "char";
  case BuiltinType::WChar_S:
    return "wchar_t";
  case BuiltinType::Short:
    return "short";
  case BuiltinType::Int:
    return "int";
  case BuiltinType::Long:
    return "long";
  case BuiltinType::LongLong:
    return "long long";
  default:
    llvm::errs() << "warning: unsupported builtin type " << kind << "\n";
    return clangKindToString(DEFAULT_NUMERIC_TYPE);
  }
}

class StmtToJSON : public StmtVisitor<StmtToJSON> {
  json::Document::AllocatorType *allocator;
  PrintingPolicy policy;
  stack<json::Value> path;

public:
  StmtToJSON(const PrintingPolicy &policy,
             json::Document::AllocatorType *allocator):
    policy(policy),
    allocator(allocator) {}

  json::Value getValue() {
    assert(path.size() == 1);
    return std::move(path.top());
  }

  void Visit(Stmt* S) {
    StmtVisitor<StmtToJSON>::Visit(S);
  }

  void VisitBinaryOperator(BinaryOperator *Node) {
    json::Value node(json::kObjectType);
    
    //TODO?
    node.AddMember("kind", json::Value().SetString("operator"), *allocator);
    if (isPointerType(Node->getType())) {
      node.AddMember("type", json::Value().SetString("pointer"), *allocator);
      pair<string, bool> t = getPointeeType(Node->getType());
      node.AddMember("rawType", json::Value().SetString(t.first.c_str(), *allocator), *allocator);
      node.AddMember("incomplete", json::Value().SetBool(t.second), *allocator);
    } else {
      node.AddMember("type", json::Value().SetString("integer"), *allocator);
      string t = clangKindToString(getBuiltinKind(Node->getType()));
      node.AddMember("rawType", json::Value().SetString(t.c_str(), *allocator), *allocator);
    }
    json::Value repr;
    string opcode_str = BinaryOperator::getOpcodeStr(Node->getOpcode()).lower();
    repr.SetString(opcode_str.c_str(), *allocator);
    node.AddMember("repr", repr, *allocator);

    json::Value args(json::kArrayType);
    Visit(Node->getLHS());
    Visit(Node->getRHS());
    json::Value right = std::move(path.top());
    path.pop();
    json::Value left = std::move(path.top());
    path.pop();
    args.PushBack(left, *allocator);
    args.PushBack(right, *allocator);
    
    node.AddMember("args", args, *allocator);

    path.push(std::move(node));
  }

  void VisitUnaryOperator(UnaryOperator *Node) {
    json::Value node(json::kObjectType);

    node.AddMember("kind", json::Value().SetString("operator"), *allocator);
    node.AddMember("type", json::Value().SetString("integer"), *allocator);
    //NOTE: assume unary operator is non-pointer
    string t = clangKindToString(getBuiltinKind(Node->getType()));
    node.AddMember("rawType", json::Value().SetString(t.c_str(), *allocator), *allocator);
    json::Value repr;
    string opcode_str = UnaryOperator::getOpcodeStr(Node->getOpcode());
    repr.SetString(opcode_str.c_str(), *allocator);
    node.AddMember("repr", repr, *allocator);

    llvm::errs() << opcode_str << "\n";

    json::Value args(json::kArrayType);
    Visit(Node->getSubExpr());
    json::Value arg = std::move(path.top());
    path.pop();
    args.PushBack(arg, *allocator);
    node.AddMember("args", args, *allocator);

    path.push(std::move(node));
  }

  void VisitImplicitCastExpr(ImplicitCastExpr *Node) {
    Visit(Node->getSubExpr());
  }

  void VisitCastExpr(CastExpr *Node) {
    // NOTE: currently, two types of cases are supported: 
    // *  casts of atoms (variables and memeber exprs)
    // *  special case for NULL
    // I need to save the cast, since it affect the semantics, but without side effects
   json::Value node(json::kObjectType);
   node.AddMember("kind", json::Value().SetString("variable"), *allocator);
   if (isPointerType(Node->getType())) {
     node.AddMember("type", json::Value().SetString("pointer"), *allocator);
     pair<string, bool> t = getPointeeType(Node->getType());     
     node.AddMember("rawType", json::Value().SetString(t.first.c_str(), *allocator), *allocator);
     node.AddMember("incomplete", json::Value().SetBool(t.second), *allocator);
   } else {
     node.AddMember("type", json::Value().SetString("integer"), *allocator);
     string t = clangKindToString(getBuiltinKind(Node->getType()));
     node.AddMember("rawType", json::Value().SetString(t.c_str(), *allocator), *allocator);
   }
   json::Value repr;
   repr.SetString(toString(Node).c_str(), *allocator);
   node.AddMember("repr", repr, *allocator);
   
   path.push(std::move(node));
  }

  void VisitParenExpr(ParenExpr *Node) {
    Visit(Node->getSubExpr());
  }

  void VisitMemberExpr(MemberExpr *Node) {
    json::Value node(json::kObjectType);

    if (Node->isArrow()) {
      node.AddMember("kind", json::Value().SetString("dereference"), *allocator);
    } else {
      node.AddMember("kind", json::Value().SetString("variable"), *allocator);
    }
    if (isPointerType(Node->getType())) {
      node.AddMember("type", json::Value().SetString("pointer"), *allocator);
      pair<string, bool> t = getPointeeType(Node->getType());
      node.AddMember("rawType", json::Value().SetString(t.first.c_str(), *allocator), *allocator);
      node.AddMember("incomplete", json::Value().SetBool(t.second), *allocator);
    } else {
      node.AddMember("type", json::Value().SetString("integer"), *allocator);
      string t = clangKindToString(getBuiltinKind(Node->getType()));
      node.AddMember("rawType", json::Value().SetString(t.c_str(), *allocator), *allocator);
    }
    json::Value repr;
    repr.SetString(toString(Node).c_str(), *allocator);
    node.AddMember("repr", repr, *allocator);
    if (Node->isArrow()) {
      json::Value baseRepr;
      baseRepr.SetString(toString(Node->getBase()).c_str(), *allocator);
      node.AddMember("base", baseRepr, *allocator);
    }

    path.push(std::move(node));
  }

  void VisitIntegerLiteral(IntegerLiteral *Node) {
    json::Value node(json::kObjectType);

    node.AddMember("kind", json::Value().SetString("constant"), *allocator);
    node.AddMember("type", json::Value().SetString("integer"), *allocator);
    string t = clangKindToString(getBuiltinKind(Node->getType()));
    node.AddMember("rawType", json::Value().SetString(t.c_str(), *allocator), *allocator);
    json::Value repr;
    repr.SetString(toString(Node).c_str(), *allocator);
    node.AddMember("repr", repr, *allocator);

    path.push(std::move(node));
  }

  void VisitCharacterLiteral(CharacterLiteral *Node) {
    json::Value node(json::kObjectType);

    node.AddMember("kind", json::Value().SetString("constant"), *allocator);
    node.AddMember("type", json::Value().SetString("integer"), *allocator);
    string t = clangKindToString(getBuiltinKind(Node->getType()));
    node.AddMember("rawType", json::Value().SetString("char"), *allocator);
    json::Value repr;
    repr.SetString(toString(Node).c_str(), *allocator);
    node.AddMember("repr", repr, *allocator);

    path.push(std::move(node));
  }

  void VisitDeclRefExpr(DeclRefExpr *Node) {
    json::Value node(json::kObjectType);
    node.AddMember("kind", json::Value().SetString("variable"), *allocator);
    if (isPointerType(Node->getType())) {
      node.AddMember("type", json::Value().SetString("pointer"), *allocator);
      pair<string, bool> t = getPointeeType(Node->getType());      
      node.AddMember("rawType", json::Value().SetString(t.first.c_str(), *allocator), *allocator);
      node.AddMember("incomplete", json::Value().SetBool(t.second), *allocator);
    } else {
      node.AddMember("type", json::Value().SetString("integer"), *allocator);
      string t = clangKindToString(getBuiltinKind(Node->getType()));
      node.AddMember("rawType", json::Value().SetString(t.c_str(), *allocator), *allocator);
    }
    json::Value repr;
    repr.SetString(toString(Node).c_str(), *allocator);
    node.AddMember("repr", repr, *allocator);

    path.push(std::move(node));
  }

  void VisitArraySubscriptExpr(ArraySubscriptExpr *Node) {
    json::Value node(json::kObjectType);
    node.AddMember("kind", json::Value().SetString("variable"), *allocator);
    if (isPointerType(Node->getType())) {
      node.AddMember("type", json::Value().SetString("pointer"), *allocator);
      pair<string, bool> t = getPointeeType(Node->getType());
      node.AddMember("rawType", json::Value().SetString(t.first.c_str(), *allocator), *allocator);
      node.AddMember("incomplete", json::Value().SetBool(t.second), *allocator);
    } else {
      node.AddMember("type", json::Value().SetString("integer"), *allocator);
      string t = clangKindToString(getBuiltinKind(Node->getType()));
      node.AddMember("rawType", json::Value().SetString(t.c_str(), *allocator), *allocator);
    }
    json::Value repr;
    repr.SetString(toString(Node).c_str(), *allocator);
    node.AddMember("repr", repr, *allocator);

    path.push(std::move(node));
  }

};


json::Value stmtToJSON(const clang::Stmt *stmt,
                       json::Document::AllocatorType &allocator) {
    PrintingPolicy policy = PrintingPolicy(LangOptions());
    StmtToJSON T(policy, &allocator);
    T.Visit(const_cast<Stmt*>(stmt));
    return T.getValue();
}


json::Value locToJSON(unsigned fileId,
                      unsigned bl,
                      unsigned bc,
                      unsigned el,
                      unsigned ec,
                      json::Document::AllocatorType &allocator) {
  json::Value entry(json::kObjectType);
  entry.AddMember("fileId", json::Value().SetInt(fileId), allocator);
  entry.AddMember("beginLine", json::Value().SetInt(bl), allocator);
  entry.AddMember("beginColumn", json::Value().SetInt(bc), allocator);
  entry.AddMember("endLine", json::Value().SetInt(el), allocator);
  entry.AddMember("endColumn", json::Value().SetInt(ec), allocator);
  return entry;
}


bool isSuitableComponentType(const QualType &type) {
  if (const PointerType *pt = type.getTypePtr()->getAs<PointerType>()) {
    return !(pt->getPointeeType().getTypePtr()->isFunctionType());
  }
  return type.getTypePtr()->isIntegerType() || type.getTypePtr()->isCharType();
}


class CollectComponents : public StmtVisitor<CollectComponents> {
private:
  json::Document::AllocatorType *allocator;
  bool ignoreCasts;
  bool suitableTypes;
  vector<json::Value> collected;

public:
  CollectComponents(json::Document::AllocatorType *allocator, bool ignoreCasts, bool suitableTypes):
    allocator(allocator),
    ignoreCasts(ignoreCasts),
    suitableTypes(suitableTypes) {}

  vector<json::Value> getCollected() {
    return std::move(collected);
  }

  void Visit(Stmt* S) {
    StmtVisitor<CollectComponents>::Visit(S);
  }

  void VisitBinaryOperator(BinaryOperator *Node) {
    Visit(Node->getLHS());
    Visit(Node->getRHS());
  }

  void VisitUnaryOperator(UnaryOperator *Node) {
    Visit(Node->getSubExpr());
  }

  void VisitImplicitCastExpr(ImplicitCastExpr *Node) {
    Visit(Node->getSubExpr());
  }

  void VisitCastExpr(CastExpr *Node) {
    if (! ignoreCasts)
      collected.push_back(stmtToJSON(Node, *allocator));
  }

  void VisitParenExpr(ParenExpr *Node) {
    Visit(Node->getSubExpr());
  }

  void VisitIntegerLiteral(IntegerLiteral *Node) {}

  void VisitCharacterLiteral(CharacterLiteral *Node) {}

  void VisitMemberExpr(MemberExpr *Node) {
    if (!suitableTypes || isSuitableComponentType(Node->getType()))
      collected.push_back(stmtToJSON(Node, *allocator));
  }

  void VisitDeclRefExpr(DeclRefExpr *Node) {
    if (!suitableTypes || isSuitableComponentType(Node->getType()))
      collected.push_back(stmtToJSON(Node, *allocator));
  }

  void VisitArraySubscriptExpr(ArraySubscriptExpr *Node) {
    if (!suitableTypes || isSuitableComponentType(Node->getType()))
      collected.push_back(stmtToJSON(Node, *allocator));
  }

};


vector<json::Value> collectFromExpression(const Stmt *stmt,
                                          json::Document::AllocatorType &allocator,
                                          bool ignoreCasts,
                                          bool suitableTypes) {
  CollectComponents T(&allocator, ignoreCasts, suitableTypes);
  T.Visit(const_cast<Stmt*>(stmt));
  return T.getCollected();
}


json::Value varDeclToJSON(VarDecl *vd, json::Document::AllocatorType &allocator) {
  json::Value node(json::kObjectType);
  node.AddMember("kind", json::Value().SetString("variable"), allocator);
  if (isPointerType(vd->getType())) {
    node.AddMember("type", json::Value().SetString("pointer"), allocator);
    pair<string, bool> t = getPointeeType(vd->getType());
    node.AddMember("rawType", json::Value().SetString(t.first.c_str(), allocator), allocator);
    node.AddMember("incomplete", json::Value().SetBool(t.second), allocator);
  } else {
    node.AddMember("type", json::Value().SetString("integer"), allocator);
    string t = clangKindToString(getBuiltinKind(vd->getType()));
    node.AddMember("rawType", json::Value().SetString(t.c_str(), allocator), allocator);
  }
  json::Value repr;
  string name = vd->getName();
  repr.SetString(name.c_str(), allocator);
  node.AddMember("repr", repr, allocator);
  return node;
}


vector<json::Value> collectVisible(const ast_type_traits::DynTypedNode &node,
                                   unsigned line,
                                   ASTContext* context,
                                   json::Document::AllocatorType &allocator) {
  vector<json::Value> result;

  const FunctionDecl* fd;
  if ((fd = node.get<FunctionDecl>()) != NULL) {

    // adding function parameters
    for (auto it = fd->param_begin(); it != fd->param_end(); ++it) {
      auto vd = cast<VarDecl>(*it);
      if (isSuitableComponentType(vd->getType())) {
        result.push_back(varDeclToJSON(vd, allocator));
      }
    }

    if (cfg.useGlobalVariables) {
      auto parents = context->getParents(node);
      if (parents.size() > 0) {
        const ast_type_traits::DynTypedNode parent = *(parents.begin()); // FIXME: for now only first
        const TranslationUnitDecl* tu;
        if ((tu = parent.get<TranslationUnitDecl>()) != NULL) {
          for (auto it = tu->decls_begin(); it != tu->decls_end(); ++it) {
            if (isa<VarDecl>(*it)) {
              VarDecl* vd = cast<VarDecl>(*it);
              unsigned beginLine = getDeclExpandedLine(vd, context->getSourceManager());
              if (line > beginLine && isSuitableComponentType(vd->getType())) {
                result.push_back(varDeclToJSON(vd, allocator));
              }
            }
          }
        }
      }
    }
    
  } else {

    const CompoundStmt* cstmt;
    if ((cstmt = node.get<CompoundStmt>()) != NULL) {
      for (auto it = cstmt->body_begin(); it != cstmt->body_end(); ++it) {

        if (isa<BinaryOperator>(*it)) {
          BinaryOperator* op = cast<BinaryOperator>(*it);
          SourceRange expandedLoc = getExpandedLoc(op, context->getSourceManager());
          unsigned beginLine = context->getSourceManager().getExpansionLineNumber(expandedLoc.getBegin());
          // FIXME: support declarations with initialization
          // FIXME: support augmented assignments:
          // FIXME: is it redundant if we use collect funnction on whole statement
          if (line > beginLine &&
              BinaryOperator::getOpcodeStr(op->getOpcode()).lower() == "=" &&
              isa<DeclRefExpr>(op->getLHS())) {
            DeclRefExpr* dref = cast<DeclRefExpr>(op->getLHS());
            VarDecl* vd;
            if ((vd = cast<VarDecl>(dref->getDecl())) != NULL && isSuitableComponentType(vd->getType())) {
              result.push_back(varDeclToJSON(vd, allocator));
            }
          }
        }

        if (isa<DeclStmt>(*it)) {
          DeclStmt* dstmt = cast<DeclStmt>(*it);
          SourceRange expandedLoc = getExpandedLoc(dstmt, context->getSourceManager());
          unsigned beginLine = context->getSourceManager().getExpansionLineNumber(expandedLoc.getBegin());
          for (auto it = dstmt->decl_begin(); it != dstmt->decl_end(); ++it) {
            Decl* d = *it;
            if (isa<VarDecl>(d)) {
              VarDecl* vd = cast<VarDecl>(d);
              // NOTE: hasInit because don't want to use garbage
              if (line > beginLine && vd->hasInit() && isSuitableComponentType(vd->getType())) {
                result.push_back(varDeclToJSON(vd, allocator));
              }
            }
          }
        }

        Stmt* stmt = cast<Stmt>(*it);
        SourceRange expandedLoc = getExpandedLoc(stmt, context->getSourceManager());
        unsigned beginLine = context->getSourceManager().getExpansionLineNumber(expandedLoc.getBegin());
        if (line > beginLine) {
          vector<json::Value> fromExpr = collectFromExpression(*it, allocator, true, true);
          for (auto &c : fromExpr) {
            result.push_back(std::move(c));
          }
          
          // FIXME: is it redundant?
          // TODO: should be generalized for other cases:
          if (isa<IfStmt>(*it)) {
            IfStmt* ifStmt = cast<IfStmt>(*it);
            Stmt* thenStmt = ifStmt->getThen();
            if (isa<CallExpr>(*thenStmt)) {
              CallExpr* callExpr = cast<CallExpr>(thenStmt);
              for (auto a = callExpr->arg_begin(); a != callExpr->arg_end(); ++a) {
                auto e = cast<Expr>(*a);
                vector<json::Value> fromParamExpr = collectFromExpression(e, allocator, true, true);
                for (auto &c : fromParamExpr) {
                  result.push_back(std::move(c));
                }
              }
            }
          }
        }
        
      }
    }
    
    auto parents = context->getParents(node);
    if (parents.size() > 0) {
      const ast_type_traits::DynTypedNode parent = *(parents.begin()); // TODO: for now only first
      vector<json::Value> parentResult = collectVisible(parent, line, context, allocator);
      for (auto &c : parentResult) {
        result.push_back(std::move(c));
      }
    }
  }
  
  return result;
}


vector<json::Value> collectComponents(const Stmt *stmt,
                                      unsigned line,
                                      ASTContext *context,
                                      json::Document::AllocatorType &allocator) {
  vector<json::Value> fromExpr = collectFromExpression(stmt, allocator, false, false);

  const ast_type_traits::DynTypedNode node = ast_type_traits::DynTypedNode::create(*stmt);
  vector<json::Value> visible = collectVisible(node, line, context, allocator);

  vector<json::Value> result;

  for (auto &c : fromExpr) {
    bool newComponent = true;
    for (auto &old : result) {
      if (c["repr"] == old["repr"])
        newComponent = false;
    }
    if (newComponent)
      result.push_back(std::move(c));
  }  

  for (auto &c : visible) {
    bool newComponent = true;
    for (auto &old : result) {
      if (c["repr"] == old["repr"])
        newComponent = false;
    }
    if (newComponent)
      result.push_back(std::move(c));
  }  

  return result;
}


/*
  Structure:
  "non-pointer types arguments, pointer arguments, pointee sizes"
*/
string makeArgumentList(vector<json::Value> &components) {
  std::ostringstream result;

  map<string, vector<json::Value*>> typesToValues;
  vector<json::Value*> pointers;
  vector<string> completePointeeTypes;
  vector<string> nonPointerTypes;
  vector<string> dereferences;
  map<string, string> baseOfDereference;
  for (auto &c : components) {
    string kind = c["kind"].GetString();
    if (kind == "dereference") {
      string repr = c["repr"].GetString();
      if (std::find(dereferences.begin(), dereferences.end(), repr) == dereferences.end()) {
        dereferences.push_back(repr);
        string base = c["base"].GetString();
        baseOfDereference[repr] = base;
      }
    }
    string type = c["type"].GetString();
    if (type == "pointer") {
      pointers.push_back(&c);
      bool isIncomplete = c["incomplete"].GetBool();
      if (! isIncomplete) {
        string rawType = c["rawType"].GetString();
        if (std::find(completePointeeTypes.begin(), completePointeeTypes.end(), rawType) == completePointeeTypes.end()) {
          completePointeeTypes.push_back(rawType);
        }
      }
    } else {
      string rawType = c["rawType"].GetString();
      if (std::find(nonPointerTypes.begin(), nonPointerTypes.end(), rawType) == nonPointerTypes.end()) {
        nonPointerTypes.push_back(rawType);
      }
      if (typesToValues.find(rawType) == typesToValues.end()) {
        typesToValues.insert(std::make_pair(rawType, vector<json::Value*>()));
      }
      typesToValues[rawType].push_back(&c);
    }
  }
  
  std::stable_sort(nonPointerTypes.begin(), nonPointerTypes.end());
  std::stable_sort(completePointeeTypes.begin(), completePointeeTypes.end());
  std::stable_sort(dereferences.begin(), dereferences.end());

  bool firstArray = true;
  for (auto &type : nonPointerTypes) {
    if (firstArray) {
      firstArray = false;
    } else {
      result << ", ";
    }
    result << "(" << type << "[]){";
    bool firstElement = true;
    for (auto c : typesToValues[type]) {
      if (firstElement) {
        firstElement = false;
      } else {
        result << ", ";
      }
      string repr = (*c)["repr"].GetString();
      if (baseOfDereference.count(repr)) {
        result << "(!" << baseOfDereference[repr]
               << " ? 0 : " << repr << ")";
      } else {
        result << repr;
      }
    }
    result << "}";
  }
  if (! pointers.empty()) {
    if (firstArray) {
      firstArray = false;
    } else {
      result << ", ";
    }
    result << "(void*[]){";
    bool firstPointerElement = true;
    for (auto c : pointers) {
      if (firstPointerElement) {
        firstPointerElement = false;
      } else {
        result << ", ";
      }
      string repr = (*c)["repr"].GetString();
      if (baseOfDereference.count(repr)) {
        result << "(!" << baseOfDereference[repr]
               << " ? (void*)0 : " << repr << ")";
      } else {
        result << repr;
      }
    }
    result << "}";
    result << ", ";
    result << "(int[]){";
    bool firstTypeElement = true;
    for (auto t : completePointeeTypes) {
      if (firstTypeElement) {
        firstTypeElement = false;
      } else {
        result << ", ";
      }
      result << "sizeof(" << t << ")";
    }
    result << "}";
  }
  if (! dereferences.empty()) {
    if (firstArray) {
      firstArray = false;
    } else {
      result << ", ";
    }
    result << "(int[]){";
    bool firstNULLDereferenceCondition = true;
    for (auto d : dereferences) {
      if (firstNULLDereferenceCondition) {
        firstNULLDereferenceCondition = false;
      } else {
        result << ", ";
      }
      result << "!" << baseOfDereference[d];
    }
    result << "}";
  }

  return result.str();
}
