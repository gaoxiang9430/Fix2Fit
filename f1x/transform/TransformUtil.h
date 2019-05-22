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

#include "clang/AST/AST.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Lex/PPCallbacks.h"


#include <rapidjson/document.h>


//NOTE: this is a hack to collect ifdef locations from preprocessor
extern std::shared_ptr<std::vector<clang::SourceRange>> globalConditionalsPP;

// http://stackoverflow.com/questions/19195183/how-to-properly-hash-the-custom-struct
template <class T>
inline void hash_combine(std::size_t & s, const T & v) {
  std::hash<T> h;
  s ^= h(v) + 0x9e3779b9 + (s << 6) + (s >> 2);
}

struct Location {
  unsigned fileId;
  unsigned beginLine;
  unsigned beginColumn;
  unsigned endLine;
  unsigned endColumn;

  bool operator==(const Location &other) const { 
    return (fileId == other.fileId
            && beginLine == other.beginLine
            && beginColumn == other.beginColumn
            && endLine == other.endLine
            && endColumn == other.endColumn);
  }
};

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

unsigned getDeclExpandedLine(const clang::Decl *decl, clang::SourceManager &srcMgr);

bool insideMacro(const clang::Stmt *expr, clang::SourceManager &srcMgr, const clang::LangOptions &langOpts);

class PPConditionalRecoder : public clang::PPCallbacks {
public:
  PPConditionalRecoder(std::shared_ptr<std::vector<clang::SourceRange>> conditionals);
  void Endif(clang::SourceLocation Loc, clang::SourceLocation IfLoc);

private:
  std::shared_ptr<std::vector<clang::SourceRange>> conditionals;
};

bool intersectConditionalPP(const clang::Stmt* stmt, 
                            clang::SourceManager &srcMgr, 
                            const std::shared_ptr<std::vector<clang::SourceRange>> conditions);


clang::SourceRange getExpandedLoc(const clang::Stmt *expr, clang::SourceManager &srcMgr);

std::string toString(const clang::Stmt *stmt);

bool overwriteMainChangedFile(clang::Rewriter &TheRewriter);

/*
  The purpose of this function is to determine if the statement is a child of 
  - compound statment
  - if/while/for statement
  - label statement (inside switch)
  it should not be, for example, the increment of for loop
 */
bool isTopLevelStatement(const clang::Stmt *stmt, clang::ASTContext *context);

bool isChildOfNonblock(const clang::Stmt *stmt, clang::ASTContext *context);

bool inConditionContext(const clang::Expr *expr, clang::ASTContext *context);

rapidjson::Value stmtToJSON(const clang::Stmt *stmt,
                            rapidjson::Document::AllocatorType &allocator);


rapidjson::Value locToJSON(unsigned fileId, unsigned bl, unsigned bc, unsigned el, unsigned ec,
                           rapidjson::Document::AllocatorType &allocator);


std::vector<rapidjson::Value> collectComponents(const clang::Stmt *stmt,
                                                unsigned line,
                                                clang::ASTContext *context,
                                                rapidjson::Document::AllocatorType &allocator);


std::string makeArgumentList(std::vector<rapidjson::Value> &components);

unsigned long f1xapp(unsigned long baseId, unsigned fileId);
bool inRange(unsigned line);
