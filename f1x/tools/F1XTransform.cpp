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

#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
// Declares llvm::cl::extrahelp.
#include "llvm/Support/CommandLine.h"

#include "TransformGlobal.h"
#include "TransformUtil.h"
#include "ProfileInstrumentation.h"
#include "SchemaApplication.h"
#include "PatchApplication.h"
#include "Config.h"

using namespace clang::tooling;
using namespace llvm;

// CommonOptionsParser declares HelpMessage with a description of the common
// command-line options related to the compilation database and input files.
// It's nice to have this help message in all tools.
static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);

// A help message for this specific tool can be added afterwards.
static cl::extrahelp MoreHelp("\nf1x-transform is a tool used internally by f1x\n");

// Apply a custom category to all command-line options so that they are the
// only ones displayed.
static llvm::cl::OptionCategory F1XCategory("f1x-transform options");

static cl::opt<bool>
Global("global", cl::desc("use global variables"), cl::cat(F1XCategory));

// Search space instrumentation options:

static cl::opt<std::string>
Instrument("instrument", cl::desc("instrument search space with profile"), cl::cat(F1XCategory));

static cl::opt<bool>
Profile("profile", cl::desc("instrument search space for profile"), cl::cat(F1XCategory));

static cl::opt<bool>
DisableGuard("disable-guard", cl::desc("don't instrument guards"), cl::cat(F1XCategory));

static cl::opt<unsigned>
FileId("file-id", cl::desc("file id"), cl::cat(F1XCategory));

static cl::opt<unsigned>
FromLine("from-line", cl::desc("from line"), cl::cat(F1XCategory));

static cl::opt<unsigned>
ToLine("to-line", cl::desc("to line"), cl::cat(F1XCategory));

static cl::opt<std::string>
Output("output", cl::desc("output file"), cl::cat(F1XCategory));


// Patch application options:

static cl::opt<bool>
Apply("apply", cl::desc("apply patch"), cl::cat(F1XCategory));

static cl::opt<unsigned>
BeginLine("bl", cl::desc("begin line"), cl::cat(F1XCategory));

static cl::opt<unsigned>
BeginColumn("bc", cl::desc("begin column"), cl::cat(F1XCategory));

static cl::opt<unsigned>
EndLine("el", cl::desc("end line"), cl::cat(F1XCategory));

static cl::opt<unsigned>
EndColumn("ec", cl::desc("end column"), cl::cat(F1XCategory));

static cl::opt<std::string>
Patch("patch", cl::desc("replacement"), cl::cat(F1XCategory));


int main(int argc, const char **argv) {
  CommonOptionsParser OptionsParser(argc, argv, F1XCategory);
  ClangTool Tool(OptionsParser.getCompilations(),
                 OptionsParser.getSourcePathList());

  std::unique_ptr<FrontendActionFactory> FrontendFactory;

  cfg.fileId = FileId;
  cfg.fromLine = FromLine;
  cfg.toLine = ToLine;
  cfg.profileFile = Instrument;
  cfg.outputFile = Output;
  cfg.beginLine = BeginLine;
  cfg.beginColumn = BeginColumn;
  cfg.endLine = EndLine;
  cfg.endColumn = EndColumn;
  cfg.patch = Patch;
  cfg.useGlobalVariables = Global;
  if (DisableGuard) {
    cfg.addGuards = false;
  }

  if (Apply)
    FrontendFactory = newFrontendActionFactory<PatchApplicationAction>();
  else if(Profile)
    FrontendFactory = newFrontendActionFactory<ProfileInstrumentationAction>();
  else if (Instrument != "")
    FrontendFactory = newFrontendActionFactory<SchemaApplicationAction>();
  else {
    errs() << "error: specify -profile -instrument FILE or -apply options\n";
    return 1;
  }
  
  return Tool.run(FrontendFactory.get());
}
