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
#include <unordered_set>
#include <fstream>

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/writer.h>
#include <map>

#include "Config.h"
#include "TransformGlobal.h"
#include "TransformUtil.h"
#include "SearchSpaceMatchers.h"
#include "SchemaApplication.h"

using namespace clang;
using namespace ast_matchers;

namespace json = rapidjson;

using std::vector;
using std::string;

json::Document schemaApplications;

std::unordered_set<std::string> interestingLocations;

void initInterestingLocations() {
  std::ifstream infile(cfg.profileFile);
  std::string line;
  while(std::getline(infile, line)) {
    interestingLocations.insert(line);
  }
}

bool isInterestingLocation(unsigned fileId, unsigned beginLine, unsigned beginColumn, unsigned endLine, unsigned endColumn) {
  std::ostringstream location;
  location << fileId << " "
           << beginLine << " "
           << beginColumn << " "
           << endLine << " "
           << endColumn;

  if(interestingLocations.find(location.str()) == interestingLocations.end())
    return false;

  return true;
}

/*
  Clang sometimes (for unknown reasons) starts the same file action or matches the same location twice, which causes crashes or invalid results.
  This is to avoid doing the same twice.
*/
static bool alreadyTransformed = false;
static std::unordered_set<Location> alreadyMatched;

bool SchemaApplicationAction::BeginSourceFileAction(CompilerInstance &CI, StringRef Filename) {
  if (alreadyTransformed) {
    return false;
  }
  alreadyTransformed = true;

  schemaApplications.SetArray();
  initInterestingLocations();
  return true;
}

void SchemaApplicationAction::EndSourceFileAction() {
  FileID ID = TheRewriter.getSourceMgr().getMainFileID();
  if (cfg.inplaceModification) {
    overwriteMainChangedFile(TheRewriter);
    // I am not sure what the difference is, but this case requires explicit check:
    //TheRewriter.overwriteChangedFiles();
  } else {
      TheRewriter.getEditBuffer(ID).write(llvm::outs());
  }

  std::ofstream ofs(cfg.outputFile);
  json::OStreamWrapper osw(ofs);
  json::Writer<json::OStreamWrapper> writer(osw);
  schemaApplications.Accept(writer);
}

std::unique_ptr<ASTConsumer> SchemaApplicationAction::CreateASTConsumer(CompilerInstance &CI, StringRef file) {
    TheRewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
    return llvm::make_unique<SchemaApplicationASTConsumer>(TheRewriter);
}


SchemaApplicationASTConsumer::SchemaApplicationASTConsumer(Rewriter &R) : ExpressionSchemaHandler(R), IfGuardSchemaHandler(R) {
  Matcher.addMatcher(ExpressionSchemaMatcher, &ExpressionSchemaHandler);    
  Matcher.addMatcher(IfGuardSchemaMatcher, &IfGuardSchemaHandler);
}

void SchemaApplicationASTConsumer::HandleTranslationUnit(ASTContext &Context) {
  Matcher.matchAST(Context);
}


IfGuardSchemaApplicationHandler::IfGuardSchemaApplicationHandler(Rewriter &Rewrite) : Rewrite(Rewrite) {}

void IfGuardSchemaApplicationHandler::run(const MatchFinder::MatchResult &Result) {
  if (const Stmt *stmt = Result.Nodes.getNodeAs<clang::Stmt>(BOUND)) {
    SourceManager &srcMgr = Rewrite.getSourceMgr();
    const LangOptions &langOpts = Rewrite.getLangOpts();
   
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

    if (!isInterestingLocation(cfg.fileId, beginLine, beginColumn, endLine, endColumn))
      return;
    
    unsigned long appId = f1xapp(cfg.baseAppId, cfg.fileId);
    cfg.baseAppId++;
                 
    llvm::errs() << beginLine << " "
                 << beginColumn << " "
                 << endLine << " "
                 << endColumn << "\n"
                 << appId << "\n"
                 << toString(stmt) << "\n";

    json::Value app(json::kObjectType);
    app.AddMember("schema", json::Value().SetString("if_guard"), schemaApplications.GetAllocator());
    json::Value exprJSON(json::kObjectType);
    exprJSON.AddMember("kind", json::Value().SetString("constant"), schemaApplications.GetAllocator());
    exprJSON.AddMember("type", json::Value().SetString("integer"), schemaApplications.GetAllocator());
    exprJSON.AddMember("rawType", json::Value().SetString("int"), schemaApplications.GetAllocator());
    exprJSON.AddMember("repr", json::Value().SetString("1"), schemaApplications.GetAllocator());
    app.AddMember("expression", exprJSON, schemaApplications.GetAllocator());
    app.AddMember("appId", json::Value().SetInt(appId), schemaApplications.GetAllocator());
    json::Value locJSON = locToJSON(cfg.fileId, beginLine, beginColumn, endLine, endColumn, schemaApplications.GetAllocator());
    app.AddMember("location", locJSON, schemaApplications.GetAllocator());
    app.AddMember("context", json::Value().SetString("condition"), schemaApplications.GetAllocator());
    json::Value componentsJSON(json::kArrayType);    
    vector<json::Value> components = collectComponents(stmt, beginLine, Result.Context, schemaApplications.GetAllocator());
    string arguments = makeArgumentList(components);
    for (auto &component : components) {
      componentsJSON.PushBack(component, schemaApplications.GetAllocator());
    }
    app.AddMember("components", componentsJSON, schemaApplications.GetAllocator());
    schemaApplications.PushBack(app, schemaApplications.GetAllocator());

	  unsigned long origLength = Rewrite.getRangeSize(expandedLoc);
    std::ostringstream stringStream;
    
    bool addBrackets = isChildOfNonblock(stmt, Result.Context);
    if (addBrackets)
    	stringStream << "{ ";

    //FIXME: should I use location or appid for the runtime function name?
    stringStream << "if ("
                 << "!(__f1xapp == " << appId << "ul) || "
                 << "__f1x_" << cfg.fileId << "_" << beginLine << "_" << beginColumn << "_" << endLine << "_" << endColumn
                 << "(" << arguments << ")"
                 << ") "
                 << toString(stmt);

    if (addBrackets) {
      stringStream << "; }";
      const char *followingData = srcMgr.getCharacterData(expandedLoc.getBegin());
      int followingDataSize = strlen(followingData);
      for (int i = origLength; i < followingDataSize; i++) {
        origLength++;
        char curChar = *(followingData+i);
        if (curChar == ';')
          break;
        else if (curChar == ' ' || curChar == '\t' || curChar == '\n')
          continue;
        else
          return;
      }
    }
    std::string replacement = stringStream.str();

    Rewrite.ReplaceText(expandedLoc.getBegin(), origLength, replacement);
  }
}


ExpressionSchemaApplicationHandler::ExpressionSchemaApplicationHandler(Rewriter &Rewrite) : Rewrite(Rewrite) {}

void ExpressionSchemaApplicationHandler::run(const MatchFinder::MatchResult &Result) {
  if (const Expr *expr = Result.Nodes.getNodeAs<clang::Expr>(BOUND)) {
    SourceManager &srcMgr = Rewrite.getSourceMgr();
    const LangOptions &langOpts = Rewrite.getLangOpts();

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
    std::pair<FileID, unsigned> decLoc = srcMgr.getDecomposedExpansionLoc(expandedLoc.getBegin());
    if (srcMgr.getMainFileID() != decLoc.first)
      return;

    if (!isInterestingLocation(cfg.fileId, beginLine, beginColumn, endLine, endColumn))
      return;

    unsigned long appId = f1xapp(cfg.baseAppId, cfg.fileId);
    cfg.baseAppId++;

    llvm::errs() << beginLine << " "
                 << beginColumn << " "
                 << endLine << " "
                 << endColumn << "\n"
                 << appId << "\n"
                 << toString(expr) << "\n";

    json::Value app(json::kObjectType);
    app.AddMember("schema", json::Value().SetString("expression"), schemaApplications.GetAllocator());
    json::Value exprJSON = stmtToJSON(expr, schemaApplications.GetAllocator());
    app.AddMember("expression", exprJSON, schemaApplications.GetAllocator());
    app.AddMember("appId", json::Value().SetInt(appId), schemaApplications.GetAllocator());
    json::Value locJSON = locToJSON(cfg.fileId, beginLine, beginColumn, endLine, endColumn, schemaApplications.GetAllocator());
    app.AddMember("location", locJSON, schemaApplications.GetAllocator());
    json::Value context;
    if (inConditionContext(expr, Result.Context)) {
      context = json::Value().SetString("condition");
    } else {
      context = json::Value().SetString("unknown");
    }
    app.AddMember("context", context, schemaApplications.GetAllocator());
    json::Value componentsJSON(json::kArrayType);
    vector<json::Value> components = collectComponents(expr, beginLine, Result.Context, schemaApplications.GetAllocator());
    string arguments = makeArgumentList(components);
    for (auto &component : components) {
      componentsJSON.PushBack(component, schemaApplications.GetAllocator());
    }
    app.AddMember("components", componentsJSON, schemaApplications.GetAllocator());
    schemaApplications.PushBack(app, schemaApplications.GetAllocator());
    
    std::ostringstream stringStream;
    stringStream << "(__f1xapp == " << appId << "ul ? "
                 << "__f1x_" << cfg.fileId << "_" << beginLine << "_" << beginColumn << "_" << endLine << "_" << endColumn
                 << "(" << arguments << ")"
                 << " : " << toString(expr) << ")";
    string replacement = stringStream.str();

    Rewrite.ReplaceText(expandedLoc, replacement);
  }
  
}
