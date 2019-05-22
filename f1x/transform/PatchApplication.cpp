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
#include <iostream>
#include <unordered_set>

#include "TransformGlobal.h"
#include "TransformUtil.h"
#include "SearchSpaceMatchers.h"
#include "PatchApplication.h"
#include "Config.h"

using namespace clang;
using namespace ast_matchers;

/*
  Clang sometimes (for unknown reasons) starts the same file action or matches the same location twice, which causes crashes or invalid results.
  This is to avoid doing the same twice.
*/
static bool alreadyTransformed = false;
static std::unordered_set<Location> alreadyMatched;

void PatchApplicationAction::EndSourceFileAction() {
  if (alreadyTransformed) {
    return;
  }
  alreadyTransformed = true;

  FileID ID = TheRewriter.getSourceMgr().getMainFileID();
  if (cfg.inplaceModification) {
    overwriteMainChangedFile(TheRewriter);
    // I am not sure what the difference is, but this case requires explicit check:
    //TheRewriter.overwriteChangedFiles();
  } else {
      TheRewriter.getEditBuffer(ID).write(llvm::outs());
  }
}

std::unique_ptr<ASTConsumer> PatchApplicationAction::CreateASTConsumer(CompilerInstance &CI, StringRef file) {
    TheRewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
    return llvm::make_unique<PatchApplicationASTConsumer>(TheRewriter);
}


PatchApplicationASTConsumer::PatchApplicationASTConsumer(Rewriter &R) : 
  ExpressionSchemaHandler(R),
  IfGuardSchemaHandler(R) {
  Matcher.addMatcher(ExpressionSchemaMatcher, &ExpressionSchemaHandler);    
  Matcher.addMatcher(IfGuardSchemaMatcher, &IfGuardSchemaHandler);
}

void PatchApplicationASTConsumer::HandleTranslationUnit(ASTContext &Context) {
  Matcher.matchAST(Context);
}


IfGuardPatchApplicationHandler::IfGuardPatchApplicationHandler(Rewriter &Rewrite) : Rewrite(Rewrite) {}

void IfGuardPatchApplicationHandler::run(const MatchFinder::MatchResult &Result) {
  if (const Stmt *stmt = Result.Nodes.getNodeAs<clang::Stmt>(BOUND)) {
      SourceManager &srcMgr = Rewrite.getSourceMgr();

      SourceRange expandedLoc = getExpandedLoc(stmt, srcMgr);

      unsigned beginLine = srcMgr.getExpansionLineNumber(expandedLoc.getBegin());
      unsigned beginColumn = srcMgr.getExpansionColumnNumber(expandedLoc.getBegin());
      unsigned endLine = srcMgr.getExpansionLineNumber(expandedLoc.getEnd());
      unsigned endColumn = srcMgr.getExpansionColumnNumber(expandedLoc.getEnd());

      Location current{cfg.fileId, beginLine, beginColumn, endLine, endColumn};
      if (alreadyMatched.count(current))
        return;
      alreadyMatched.insert(current);

      // NOTE: to avoid extracting locations from headers:
      std::pair<FileID, unsigned> decLoc = srcMgr.getDecomposedExpansionLoc(expandedLoc.getBegin());
      if (srcMgr.getMainFileID() != decLoc.first)
        return;

      if (beginLine == cfg.beginLine &&
          beginColumn == cfg.beginColumn &&
          endLine == cfg.endLine &&
          endColumn == cfg.endColumn) {

        std::stringstream replacement;
        
        unsigned origLength = Rewrite.getRangeSize(expandedLoc);
        bool addBrackets = isChildOfNonblock(stmt, Result.Context);
        if (addBrackets)
    	    replacement << "{ ";
    	    
        replacement << "if (" << cfg.patch << ") " << toString(stmt);

        llvm::errs() << beginLine << " " << beginColumn << " " << endLine << " " << endColumn << "\n"
          << "<   " << toString(stmt) << "\n"
          << ">   " << replacement.str() << "\n";
        
        if (addBrackets) {
        	replacement << "; }";
        	const char *followingData = srcMgr.getCharacterData(expandedLoc.getBegin());
        	unsigned followingDataSize = strlen(followingData);
          for (int i = origLength; i < followingDataSize; i++) {
            origLength++;
            char curChar = *(followingData + i);
            if (curChar == ';')
              break;
            else if (curChar == ' ' || curChar == '\t' || curChar == '\n')
              continue;
            else
              return;
          }
        }

        Rewrite.ReplaceText(expandedLoc.getBegin(), origLength, replacement.str());
      }
  }
}


ExpressionPatchApplicationHandler::ExpressionPatchApplicationHandler(Rewriter &Rewrite) : Rewrite(Rewrite) {}

void ExpressionPatchApplicationHandler::run(const MatchFinder::MatchResult &Result) {
  if (const Expr *expr = Result.Nodes.getNodeAs<clang::Expr>(BOUND)) {
      SourceManager &srcMgr = Rewrite.getSourceMgr();

      SourceRange expandedLoc = getExpandedLoc(expr, srcMgr);

      unsigned beginLine = srcMgr.getExpansionLineNumber(expandedLoc.getBegin());
      unsigned beginColumn = srcMgr.getExpansionColumnNumber(expandedLoc.getBegin());
      unsigned endLine = srcMgr.getExpansionLineNumber(expandedLoc.getEnd());
      unsigned endColumn = srcMgr.getExpansionColumnNumber(expandedLoc.getEnd());

      Location current{cfg.fileId, beginLine, beginColumn, endLine, endColumn};
      if (alreadyMatched.count(current))
        return;
      alreadyMatched.insert(current);

      // NOTE: to avoid extracting locations from headers:
      std::pair<FileID, unsigned long> decLoc = srcMgr.getDecomposedExpansionLoc(expandedLoc.getBegin());
      if (srcMgr.getMainFileID() != decLoc.first)
        return;

      //FIXME: do I need to cast here?
      if (beginLine == cfg.beginLine &&
          beginColumn == cfg.beginColumn &&
          endLine == cfg.endLine &&
          endColumn == cfg.endColumn) {
        llvm::errs() << beginLine << " " << beginColumn << " " << endLine << " " << endColumn << "\n"
          << "<   " << toString(expr) << "\n"
          << ">   " << cfg.patch << "\n";
        
        Rewrite.ReplaceText(expandedLoc, cfg.patch);
      }
  }
}

