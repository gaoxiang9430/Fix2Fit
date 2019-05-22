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

#include <sstream>
#include <fstream>
#include <iostream>
#include <unordered_set>

#include "Config.h"
#include "TransformGlobal.h"
#include "TransformUtil.h"
#include "SearchSpaceMatchers.h"
#include "ProfileInstrumentation.h"

using namespace clang;
using namespace ast_matchers;


/*
  Clang sometimes (for unknown reasons) starts the same file action or matches the same location twice, which causes crashes or invalid results.
  This is to avoid doing the same twice.
*/
static bool alreadyTransformed = false;
static std::unordered_set<Location> alreadyMatched;

bool ProfileInstrumentationAction::BeginSourceFileAction(CompilerInstance &CI, StringRef Filename) {
  if (alreadyTransformed) {
    return false;
  }
  alreadyTransformed = true;

  std::unique_ptr<PPConditionalRecoder> recorder(new PPConditionalRecoder(globalConditionalsPP));

  Preprocessor &pp = CI.getPreprocessor();
  pp.addPPCallbacks(std::move(recorder));

  return true;
}

void ProfileInstrumentationAction::EndSourceFileAction() {
  FileID ID = TheRewriter.getSourceMgr().getMainFileID();
  if (cfg.inplaceModification) {
    overwriteMainChangedFile(TheRewriter);
    // I am not sure what the difference is, but this case requires explicit check:
    //TheRewriter.overwriteChangedFiles();
  } else {
      TheRewriter.getEditBuffer(ID).write(llvm::outs());
  }
}

std::unique_ptr<ASTConsumer> ProfileInstrumentationAction::CreateASTConsumer(CompilerInstance &CI, StringRef file) {
    TheRewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
    return llvm::make_unique<ProfileInstrumentationASTConsumer>(TheRewriter);
}


ProfileInstrumentationASTConsumer::ProfileInstrumentationASTConsumer(Rewriter &R) :
  ExpressionSchemaHandler(R),
  IfGuardSchemaHandler(R) {
  Matcher.addMatcher(ExpressionSchemaMatcher, &ExpressionSchemaHandler);    
  if (cfg.addGuards) Matcher.addMatcher(IfGuardSchemaMatcher, &IfGuardSchemaHandler);
}

void ProfileInstrumentationASTConsumer::HandleTranslationUnit(ASTContext &Context) {
  Matcher.matchAST(Context);
}


IfGuardSchemaProfileHandler::IfGuardSchemaProfileHandler(Rewriter &Rewrite) : Rewrite(Rewrite) {}

void IfGuardSchemaProfileHandler::run(const MatchFinder::MatchResult &Result) {
  if (const Stmt *stmt = Result.Nodes.getNodeAs<clang::Stmt>(BOUND)) {
      SourceManager &srcMgr = Rewrite.getSourceMgr();
      
      const LangOptions &langOpts = Rewrite.getLangOpts();
      if (insideMacro(stmt, srcMgr, langOpts) || 
          intersectConditionalPP(stmt, srcMgr, globalConditionalsPP))
        return;

      if(!isTopLevelStatement(stmt, Result.Context))
        return;

      SourceRange expandedLoc = getExpandedLoc(stmt, srcMgr);

      unsigned beginLine = srcMgr.getExpansionLineNumber(expandedLoc.getBegin());
      unsigned beginColumn = srcMgr.getExpansionColumnNumber(expandedLoc.getBegin());
      unsigned endLine = srcMgr.getExpansionLineNumber(expandedLoc.getEnd());
      unsigned endColumn = srcMgr.getExpansionColumnNumber(expandedLoc.getEnd());
      
      if (!inRange(beginLine))
        return;

      Location current{cfg.fileId, beginLine, beginColumn, endLine, endColumn};
      if (alreadyMatched.count(current))
        return;
      alreadyMatched.insert(current);

      // NOTE: to avoid extracting locations from headers:
      std::pair<FileID, unsigned> decLoc = srcMgr.getDecomposedExpansionLoc(expandedLoc.getBegin());
      if (srcMgr.getMainFileID() != decLoc.first)
        return;

      std::ostringstream replacement;
      replacement << "({ __f1x_trace(" << cfg.fileId << ", "
                                       << beginLine << ", "
                                       << beginColumn << ", "
                                       << endLine << ", "
                                       << endColumn << "); "
                  << toString(stmt) << "; })";

      Rewrite.ReplaceText(expandedLoc, replacement.str());
  }
}


ExpressionSchemaProfileHandler::ExpressionSchemaProfileHandler(Rewriter &Rewrite) : Rewrite(Rewrite) {}

void ExpressionSchemaProfileHandler::run(const MatchFinder::MatchResult &Result) {
  if (const Expr *expr = Result.Nodes.getNodeAs<clang::Expr>(BOUND)) {
    SourceManager &srcMgr = Rewrite.getSourceMgr();
    const LangOptions &langOpts = Rewrite.getLangOpts();

    if (insideMacro(expr, srcMgr, langOpts) || 
        intersectConditionalPP(expr, srcMgr, globalConditionalsPP))
      return;

    SourceRange expandedLoc = getExpandedLoc(expr, srcMgr);

    unsigned beginLine = srcMgr.getExpansionLineNumber(expandedLoc.getBegin());
    unsigned beginColumn = srcMgr.getExpansionColumnNumber(expandedLoc.getBegin());
    unsigned endLine = srcMgr.getExpansionLineNumber(expandedLoc.getEnd());
    unsigned endColumn = srcMgr.getExpansionColumnNumber(expandedLoc.getEnd());

    if (!inRange(beginLine))
      return;

    Location current{cfg.fileId, beginLine, beginColumn, endLine, endColumn};
    if (alreadyMatched.count(current))
      return;
    alreadyMatched.insert(current);

    // NOTE: to avoid extracting locations from headers:
    std::pair<FileID, unsigned> decLoc = srcMgr.getDecomposedExpansionLoc(expandedLoc.getBegin());
    if (srcMgr.getMainFileID() != decLoc.first)
      return;

    std::ostringstream stringStream;
    stringStream << "({ __f1x_trace(" << cfg.fileId << ", " 
                                      << beginLine << ", "
                                      << beginColumn << ", " 
                                      << endLine << ", "
                                      << endColumn << "); "
                 << toString(expr) << "; })";
    
    Rewrite.ReplaceText(expandedLoc, stringStream.str());
  }
}

