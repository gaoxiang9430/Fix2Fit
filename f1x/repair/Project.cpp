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
#include <iomanip>
#include <sys/wait.h>

#include <boost/filesystem/fstream.hpp>
#include <boost/log/trivial.hpp>

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/writer.h>

#include "Config.h"
#include "Project.h"
#include "Util.h"
#include "Global.h"

namespace fs = boost::filesystem;
namespace json = rapidjson;

using std::string;
using std::vector;
using std::map;
using std::shared_ptr;


const string PLACEHOLDER = "F1X_EXPRESSION_PLACEHOLDER";


void replacePlaceholderInFile(const fs::path &file, const string replacement) {
  fs::path replacementFile = fs::path(cfg.dataDir) / "replacement.c";
  {
    fs::ifstream in(file);
    fs::ofstream out(replacementFile);
    string line;
    size_t len = PLACEHOLDER.length();
    while (getline(in, line)) {
      size_t pos = line.find(PLACEHOLDER);
      if (pos != string::npos)
        line.replace(pos, len, replacement);
      out << line << '\n';
    }
  }
  fs::rename(replacementFile, file);
}


bool projectFilesInCompileDB(vector<ProjectFile> files) {
  fs::path compileDB("compile_commands.json");
  json::Document db;
  {
    fs::ifstream ifs(compileDB);
    json::IStreamWrapper isw(ifs);
    db.ParseStream(isw);
  }

  for (auto &file : files) {
    bool present = false;

    for (auto& entry : db.GetArray()) {
      string dbFile = entry.GetObject()["file"].GetString();
      if (fs::equivalent(fs::path(dbFile), file.relpath)) {
        present = true;
      }
    }

    if (! present) {
      BOOST_LOG_TRIVIAL(warning) << "file " << file.relpath
                                 << " is not in compilation database";
      return false;
    }
  }

  return true; 
}

/* 
   1. Add Clang headers.
      By default a Clang tool searches for headers in a strange place, so we add them explicitly
   2. Add default definitions
      Sometime if the transformed program contains undefined symbolic, transformation fails
      (e.g. gmp-13420-13421, it may depend on compiler flags)
 */
void adjustCompileDB() {
  fs::path compileDB("compile_commands.json");
  json::Document db;
  {
    fs::ifstream ifs(compileDB);
    json::IStreamWrapper isw(ifs);
    db.ParseStream(isw);
  }

  for (auto& entry : db.GetArray()) {
    // assume there is always a first space in which we insert our include
    // FIXME: add escape for the spaces in the include path
    string command = entry.GetObject()["command"].GetString();
    string includeCmd =  "-I" + F1X_CLANG_INCLUDE;
    if (command.find(includeCmd) == std::string::npos) {
      unsigned index = command.find(" ");
      command = command.substr(0, index) + " " + includeCmd + " " + command.substr(index);
    }
    string defineCmd =  "-D__f1xapp=0ul";
    if (command.find(defineCmd) == std::string::npos) {
      unsigned long index = command.find(" ");
      command = command.substr(0, index) + " " + defineCmd + " " + command.substr(index);
    }
    entry.GetObject()["command"].SetString(command.c_str(), db.GetAllocator());
  }
  
  {
    fs::ofstream ofs(compileDB);
    json::OStreamWrapper osw(ofs);
    json::Writer<json::OStreamWrapper> writer(osw);
    db.Accept(writer);
  }
}


Project::Project(const std::vector<ProjectFile> &files,
                 const std::string &buildCmd):
  files(files),
  buildCmd(buildCmd) {
  saveOriginalFiles();
  patchTemplateDir = fs::path(cfg.dataDir) / "templates";
  fs::create_directory(patchTemplateDir);
  }

Project::~Project() {
  restoreOriginalFiles();
}

vector<ProjectFile> Project::getFiles() const {
  return files;
}

bool Project::buildInEnvironment(const std::map<std::string, std::string> &environment,
                                 const std::string &baseCmd) {
  InEnvironment env(environment);

  std::stringstream cmd;
  cmd << " ( " << baseCmd << " ) ";
  if (cfg.verbose) {
    cmd << " >&2";
  } else {
    cmd << " >/dev/null 2>&1";
  }

  BOOST_LOG_TRIVIAL(debug) << "cmd: " << cmd.str();
  unsigned long status = std::system(cmd.str().c_str());

  return WEXITSTATUS(status) == 0;
}

bool reusableCompilationDatabaseExists() {
  fs::path compileDB("compile_commands.json");
  if (! fs::exists(compileDB)) {
    return false;
  }
  json::Document db;
  {
    fs::ifstream ifs(compileDB);
    json::IStreamWrapper isw(ifs);
    db.ParseStream(isw);
  }

  return (! db.GetArray().Empty());
}

std::pair<bool, bool> Project::initialBuild() {
  BOOST_LOG_TRIVIAL(info) << "building project and inferring compile commands";

  std::stringstream cmd;
  // FIXME: ideally, I should use "bear --append", but due to its implementation this corrupts compile db
  if (reusableCompilationDatabaseExists()) {
    BOOST_LOG_TRIVIAL(info) << "using existing compilation database (compile_commands.json)";
    cmd << buildCmd;
  } else {
    cmd << "f1x-bear sh -c \"" << buildCmd << "\"";
  }
  bool compilationSuccess = buildInEnvironment({ {"CC", "f1x-cc"}, {"CXX", "f1x-cxx"} }, cmd.str());

  bool inferenceSuccess = fs::exists("compile_commands.json");

  if (inferenceSuccess) {
    if (projectFilesInCompileDB(files)) {
      adjustCompileDB();
    } else {
      inferenceSuccess = false;
    }
  }

  return std::make_pair(compilationSuccess, inferenceSuccess);
}

bool Project::build() {
  BOOST_LOG_TRIVIAL(info) << "building project";

  bool success = buildInEnvironment({ {"CC", "f1x-cc"}, {"CXX", "f1x-cxx"} }, buildCmd);

  return success;
}

bool Project::buildWithRuntime(const fs::path &header) {
  BOOST_LOG_TRIVIAL(info) << "building project with f1x runtime";

  bool success = buildInEnvironment({ {"CC", "f1x-cc"},
                                      {"CXX", "f1x-cxx"},
                                      {"F1X_RUNTIME_H", header.string()},
                                      {"F1X_RUNTIME_LIB", cfg.dataDir},
                                      {"LD_LIBRARY_PATH", cfg.dataDir} },
                                    buildCmd);

  return success;
}

void Project::saveFilesWithPrefix(const string &prefix) {
  for (int i = 0; i < files.size(); i++) {
    auto destination = fs::path(cfg.dataDir) / fs::path(prefix + std::to_string(i) + ".c");
    if(fs::exists(destination)) {
      fs::remove(destination);
    }
    fs::copy(files[i].relpath, destination);
  }
}

void Project::restoreFilesWithPrefix(const string &prefix) {
  for (int i = 0; i < files.size(); i++) {
    if(fs::exists(files[i].relpath)) {
      fs::remove(files[i].relpath);
    }
    fs::copy(fs::path(cfg.dataDir) / fs::path(prefix + std::to_string(i) + ".c"), files[i].relpath);
  }
}

void Project::saveOriginalFiles() {
  saveFilesWithPrefix("original");
}

void Project::saveInstrumentedFiles() {
  saveFilesWithPrefix("instrumented");
}

void Project::saveProfileInstumentedFiles() {
  saveFilesWithPrefix("profile_instrumented");
}


void Project::restoreOriginalFiles() {
  restoreFilesWithPrefix("original");
}

void Project::restoreInstrumentedFiles() {
  restoreFilesWithPrefix("instrumented");
}

void Project::deleteCoverageFiles() {
  std::stringstream cmdGcovr;
  cmdGcovr << "gcovr --delete --xml";
  if (cfg.useLLVMCov)
    cmdGcovr << " --gcov-executable=f1x-llvm-cov";
  cmdGcovr << " >/dev/null 2>&1";
  BOOST_LOG_TRIVIAL(debug) << "cmd: " << cmdGcovr.str();
  unsigned long status = std::system(cmdGcovr.str().c_str());
  if (WEXITSTATUS(status) != 0) {
    BOOST_LOG_TRIVIAL(warning) << "failed to delete coverage files";
  }
}

void Project::computeDiff(const ProjectFile &file,
                          const fs::path &output) {
  {
    fs::path a = fs::path("a") / file.relpath;
    fs::path b = fs::path("b") / file.relpath;
    fs::ofstream ofs(output);
    ofs << "--- " << a.string() << "\n"
        << "+++ " << b.string() << "\n";
  }
  unsigned id = getFileId(file);
  
  fs::path fromFile = fs::path(cfg.dataDir) / fs::path("original" + std::to_string(id) + ".c");
  fs::path toFile = fs::path(cfg.dataDir) / fs::path("patched" + std::to_string(id) + ".c");
  string cmd = "diff " + fromFile.string() + " " + toFile.string() + " >> " + output.string();
  BOOST_LOG_TRIVIAL(debug) << "cmd: " << cmd;
  std::system(cmd.c_str());
}

bool Project::instrumentFile(const ProjectFile &file,
                             const boost::filesystem::path &outputFile,
                             const boost::filesystem::path *profile) {
  unsigned id = getFileId(file);

  std::stringstream cmd;
  cmd << "f1x-transform " << file.relpath.string();
  
  if(! profile) {
    cmd << " --profile";
  } else {
    cmd << " --instrument " << *profile;
  }

  if(! cfg.addGuards) {
    cmd << " --disable-guard";
  }

  cmd << " --from-line " << file.fromLine
      << " --to-line " << file.toLine
      << " --file-id " << id
      << " --output " + outputFile.string();
  if (cfg.verbose) {
    cmd << " >&2";
  } else {
    cmd << " >/dev/null 2>&1";
  }
  BOOST_LOG_TRIVIAL(debug) << "cmd: " << cmd.str();
  unsigned long status = std::system(cmd.str().c_str());
  return WEXITSTATUS(status) == 0;
}

unsigned Project::getFileId(const ProjectFile &file) {
  unsigned id = 0;
  for (auto &f : files) {
    if (file.relpath != f.relpath)
      id++;
    else
      break;
  }
  assert(id < files.size());
  return id;
}

bool Project::applyPatch(const Patch &patch) {
  bool success = true;
  BOOST_LOG_TRIVIAL(debug) << "applying patch";
  fs::path patchTemplate = patchTemplateDir / (std::to_string(patch.app->id) + ".patch");
  if (fs::exists(patchTemplate)) {
    string cmd = "patch -p1 < " + patchTemplate.string() + " >/dev/null 2>&1";
    BOOST_LOG_TRIVIAL(debug) << "cmd: " << cmd;
    unsigned long status = std::system(cmd.c_str());
    success = (WEXITSTATUS(status) == 0);
  } else {
    unsigned beginLine = patch.app->location.beginLine;
    unsigned beginColumn = patch.app->location.beginColumn;
    unsigned endLine = patch.app->location.endLine;
    unsigned endColumn = patch.app->location.endColumn;
    std::stringstream cmd;
    cmd << "f1x-transform " << files[patch.app->location.fileId].relpath.string() << " --apply"
        << " --bl " << beginLine
        << " --bc " << beginColumn
        << " --el " << endLine
        << " --ec " << endColumn
        << " --patch " << "\"" << PLACEHOLDER << "\"";
    if (cfg.verbose) {
      cmd << " >&2";
    } else {
      cmd << " >/dev/null 2>&1";
    }
    BOOST_LOG_TRIVIAL(debug) << "cmd: " << cmd.str();
    unsigned long status = std::system(cmd.str().c_str());
    success = (WEXITSTATUS(status) == 0);
    saveFilesWithPrefix("patched");
    computeDiff(files[patch.app->location.fileId], patchTemplate);
  }
  replacePlaceholderInFile(files[patch.app->location.fileId].relpath, expressionToString(patch.modified));
  saveFilesWithPrefix("patched");
  return success;
}

vector<fs::path> Project::filesFromCompilationDB() {
  std::vector<fs::path> files;
  fs::path compileDB("compile_commands.json");
  json::Document db;
  fs::ifstream ifs(compileDB);
  json::IStreamWrapper isw(ifs);
  db.ParseStream(isw);

  for (auto &entry : db.GetArray()) {
    std::string fileStr = entry.GetObject()["file"].GetString();
    bool supportedExtension = false;
    for (const string &extension : SOURCE_FILE_EXTENSIONS) {
      if (fileStr.length() > extension.length()
          && fileStr.compare(fileStr.length()-extension.length(), string::npos, extension) == 0) {
        supportedExtension = true;
        break;
      }
    }
    if (supportedExtension)
      files.push_back(relativeTo(fs::current_path(), fs::path(fileStr)));
  }
  return files;
}

void Project::setFiles(const std::vector<ProjectFile> &fs) {
  files = fs;
  saveOriginalFiles();
}


TestingFramework::TestingFramework(const Project &project,
                                   const boost::filesystem::path &driver,
                                   const unsigned long testTimeout):
  project(project),
  driver(driver),
  testTimeout(testTimeout) {}


TestStatus TestingFramework::execute(const std::string &testId) {
  InEnvironment env(map<string, string>{{"LD_LIBRARY_PATH", cfg.dataDir}});
  std::stringstream cmd;
  cmd << "timeout " << std::setprecision(3) << ((double) testTimeout) / 1000.0 << "s"
      << " " << driver.string() << " " << testId;
  if (cfg.verbose) {
    cmd << " >&2";
  } else {
    cmd << " >/dev/null 2>&1";
  }
  BOOST_LOG_TRIVIAL(debug) << "cmd: " << cmd.str();
  unsigned long status = std::system(cmd.str().c_str());
  if (WEXITSTATUS(status) == 0) {
    return TestStatus::PASS;
  } else if (WEXITSTATUS(status) == TIMEOUT_EXIT_CODE) {
    return TestStatus::TIMEOUT;
  } else {
    return TestStatus::FAIL;
  }
}

bool TestingFramework::driverIsOK() {
  if (! fs::exists(driver)) {
    return false;
  }
  if (! isExecutable(driver.string().c_str())) {
    return false;
  }
  return true;
}
