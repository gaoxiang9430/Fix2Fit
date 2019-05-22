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
#include <ctime>
#include <sstream>
#include <string>

#include <boost/filesystem.hpp>

#include <boost/program_options.hpp>

#include <boost/log/trivial.hpp>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

#include "F1X.h"
#include "Core.h"
#include "Global.h"
#include "Repair.h"
#include "Util.h"

namespace po = boost::program_options;
namespace fs = boost::filesystem;

using std::vector;
using std::string;


void initializeTrivialLogger(bool verbose) {
  boost::log::add_common_attributes();
  boost::log::register_simple_formatter_factory< boost::log::trivial::severity_level, char >("Severity");
  boost::log::add_console_log(std::cerr, boost::log::keywords::format = "[%TimeStamp%] [%Severity%]\t%Message%");
  if (verbose) {
    boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::debug);    
  } else {
    boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::info);
  }
}


std::vector<ProjectFile> parseFilesArg(const std::vector<std::string> &args) {
  std::vector<ProjectFile> files;
  for (auto &arg : args) {
    boost::filesystem::path file;
    unsigned fromLine = 0;
    unsigned toLine = 0;
    auto colonIndex = arg.find(":");
    if (colonIndex == std::string::npos) {
      file = boost::filesystem::path(arg);
    } else {
      std::string pathStr = arg.substr(0, colonIndex);
      file = boost::filesystem::path(pathStr);
      std::string rangeStr = arg.substr(colonIndex + 1);
      auto dashIndex = rangeStr.find("-");
      if (dashIndex == std::string::npos) {
        try {
          fromLine = std::stoi(rangeStr);
          toLine = fromLine;
        } catch (...) {
          throw parse_error("wrong range format: " + rangeStr);
        }
      } else {
        std::string fromStr = rangeStr.substr(0, dashIndex);
        std::string toStr = rangeStr.substr(dashIndex + 1);
        try {
          fromLine = std::stoi(fromStr);
          toLine = std::stoi(toStr);
        } catch (...) {
          throw parse_error("wrong range format: " + rangeStr);
        }
      }
    }

    file = relativeTo(fs::current_path(), fs::canonical(file));
    if (! boost::filesystem::exists(file)) {
      throw parse_error("source file does not exist: " + file.string());
    }
    files.push_back(ProjectFile{file, fromLine, toLine});
  }
  return files;
}

int repair_main(int argc, char *argv[], SearchEngine * &engine) {
  string buildCmd = "make -e";
  vector<ProjectFile> files;
  fs::path output;
  vector<string> tests;
  unsigned testTimeout;
  fs::path driver;

  po::options_description general("Usage: f1x OPTIONS\n\nSupported options");
  general.add_options()
    ("binary-path,P", po::value<string>()->value_name("PATH"), "the path of the project binary")
    ("binary-name,N", po::value<string>()->value_name("PATH"), "the name of the project binary")
    ("maxConditionParameter,M", po::value<unsigned>()->value_name("NUM"), "maxConditionParameter")
    ("driver,d", po::value<string>()->value_name("PATH"), "test driver")
    ("tests,t", po::value<vector<string>>()->multitoken()->value_name("ID..."), "list of test IDs")
    ("test-timeout,T", po::value<unsigned>()->value_name("MS"), "test execution timeout")
    ("files,f", po::value<vector<string>>()->multitoken()->value_name("PATH..."), "list of source files to repair")
    ("localize,l", po::value<unsigned>()->value_name("NUM"), ("number of files to localize (default: " + std::to_string(cfg.filesToLocalize) + ")").c_str())
    ("build,b", po::value<string>()->value_name("CMD"), ("build command (default: " + buildCmd + ")").c_str())
    ("output,o", po::value<string>()->value_name("PATH"), "output patch file or directory (default: f1x-TIME)")
    ("all,a", "generate all patches")
    ("cost,c", po::value<string>()->value_name("FUNCTION"), "patch prioritization (default: syntactic-diff)")
    ("verbose,v", "produce extended output")
    ("help,h", "produce help message and exit")
    ("version", "print version and exit")
    ("output-stat", po::value<string>()->value_name("PATH"), "output execution statistics")
    ("output-space", po::value<string>()->value_name("PATH"), "[DEBUG] output search space")
    ("output-one-per-loc", "output single optimal patch per location")
    ("output-top", po::value<unsigned>()->value_name("N"), "find top N patches")
    ("enable-cleanup", "remove intermediate data")
    ("enable-metadata", "output patch metadata")
    ("enable-validation", "validate found patches")
    ("enable-assignment", "synthesize assignments")
    ("enable-llvm-cov", "use llvm-cov instead of gcov")
    ("disable-guard", "don't synthesize guards")
    ("disable-vteq", "[DEBUG] don't apply value-based analysis")
    ("disable-dteq", "[DEBUG] don't apply dependency-based analysis")
    ("disable-testprior", "[DEBUG] don't prioritize tests")
    ;

  po::variables_map vm;

  try {
    po::store(po::command_line_parser(argc, argv).options(general).run(), vm);
    po::notify(vm);
  } catch(po::error& e) {
    BOOST_LOG_TRIVIAL(error) << e.what() << " (use --help)";
    return ERROR_EXIT_CODE;
  }

  if (vm.count("help")) {
    std::cout << general << std::endl;
    return ERROR_EXIT_CODE;
  }

  if (vm.count("cost")) {
    std::string costFunction = vm["cost"].as<string>();

    if (costFunction == "syntactic-diff") {
      cfg.patchPrioritization = PatchPrioritization::SYNTACTIC_DIFF;
    } else if (costFunction == "semantic-diff") {
      cfg.patchPrioritization = PatchPrioritization::SEMANTIC_DIFF;
      if (!vm.count("all")) {
        cfg.generateAll = true;
        BOOST_LOG_TRIVIAL(info) << "generating all patches to apply dynamic prioritization";
      }
    } else {
      BOOST_LOG_TRIVIAL(error) << "supported cost functions: syntactic-diff, semantic-diff";
      return ERROR_EXIT_CODE;
    }
  }

  if (vm.count("version")) {
    std::cout << "f1x " << F1X_VERSION_MAJOR <<
                    "." << F1X_VERSION_MINOR <<
                    "." << F1X_VERSION_PATCH << std::endl;
    return ERROR_EXIT_CODE;    
  }

  if (vm.count("verbose")) {
    cfg.verbose = true;
  }
  initializeTrivialLogger(cfg.verbose);

  if (vm.count("all")) {
    cfg.generateAll = true;
  }

  if (vm.count("output-one-per-loc")) {
    cfg.outputOnePerLocation = true;
  }

  if (vm.count("output-top")) {
    cfg.outputTop = vm["output-top"].as<unsigned>();
  }

  if (vm.count("enable-metadata")) {
    cfg.outputPatchMetadata = true;
  }

  if (vm.count("enable-cleanup")) {
    cfg.removeIntermediateData = true;
  }

  if (vm.count("enable-validation")) {
    cfg.validatePatches = true;
  }

  if (vm.count("enable-llvm-cov")) {
    cfg.useLLVMCov = true;
  }

  if (vm.count("disable-vteq")) {
    cfg.valueTEQ = false;
  }

  if (vm.count("disable-testprior")) {
    cfg.testPrioritization = TestPrioritization::FIXED_ORDER;
  }

  if (vm.count("disable-guard")) {
    cfg.addGuards = false;
  }

  if (vm.count("output-space")) {
    cfg.searchSpaceFile = fs::absolute(vm["output-space"].as<string>()).string();
  }

  if (!vm.count("tests")) {
    BOOST_LOG_TRIVIAL(error) << "tests are not specified (use --help)";
    return ERROR_EXIT_CODE;
  }
  tests = vm["tests"].as<vector<string>>();

  if (!vm.count("driver")) {
    BOOST_LOG_TRIVIAL(error) << "test driver is not specified (use --help)";
    return ERROR_EXIT_CODE;
  }
  driver = fs::absolute(vm["driver"].as<string>());

  if (vm.count("binary-path")) {
    cfg.binaryPath = fs::absolute(vm["binary-path"].as<string>()).string();
  }

  if (vm.count("binary-name")) {
    cfg.binaryName = vm["binary-name"].as<string>();
  }

  if (vm.count("maxConditionParameter")) {
    cfg.maxConditionParameter = vm["maxConditionParameter"].as<unsigned>();
  }

  if (!vm.count("test-timeout")) {
    BOOST_LOG_TRIVIAL(error) << "test execution timeout is not specified (use --help)";
    return ERROR_EXIT_CODE;    
  }
  testTimeout = vm["test-timeout"].as<unsigned>();

  if (vm.count("localize")) {
    cfg.filesToLocalize = vm["localize"].as<unsigned>();
  }

  if (vm.count("files")) {
    vector<string> fileArgs = vm["files"].as<vector<string>>();
    try {
      files = parseFilesArg(fileArgs);
    } catch (const parse_error& e) {
      BOOST_LOG_TRIVIAL(error) << e.what();
      return ERROR_EXIT_CODE;    
    }
  }

  if (vm.count("build")) {
    buildCmd = vm["build"].as<string>();
  }

  if (!vm.count("output")) {
    std::time_t now = std::time(0);
    struct std::tm tstruct;
    char timeRepr[80];
    tstruct = *localtime(&now);
    strftime(timeRepr, sizeof(timeRepr), "f1x-%Y_%m_%d-%H_%M_%S", &tstruct);
    std::stringstream name;
    name << timeRepr;
    if (!cfg.generateAll)
      name << ".patch";
    output = fs::path(name.str());
  } else {
    output = fs::path(vm["output"].as<string>());
  }

  output = fs::absolute(output);
  if (fs::exists(output)) {
    BOOST_LOG_TRIVIAL(warning) << "existing " << output << " will be overwritten";
    fs::remove_all(output);
  }

  fs::path dataDir = fs::temp_directory_path() / fs::unique_path();
  fs::create_directory(dataDir);
  BOOST_LOG_TRIVIAL(info) << "intermediate data directory: " << dataDir;
  BOOST_LOG_TRIVIAL(info) << "output data directory: " << output;
  cfg.dataDir = dataDir.string();

  //SearchEngine *engine = NULL;
  RepairStatus repairStatus;
  {
    Project project(files, buildCmd);
    TestingFramework tester(project, driver, testTimeout);
    
    repairStatus = repair(project, tester, tests, output, engine);
  }

  // NOTE: project is already destroyed here
  if (cfg.validatePatchesByFuzzing && cfg.removeIntermediateData) {
    fs::remove_all(dataDir);
  }

  switch (repairStatus) {
  case RepairStatus::SUCCESS:
    if (cfg.generateAll) {
      BOOST_LOG_TRIVIAL(info) << "patches successfully generated : " << output;
    } else {
      BOOST_LOG_TRIVIAL(info) << "patch successfully generated : " << output;
    }
    return SUCCESS_EXIT_CODE;
  case RepairStatus::FAILURE:
    BOOST_LOG_TRIVIAL(info) << "no patch found";
    return FAILURE_EXIT_CODE;
  case RepairStatus::ERROR:
    return ERROR_EXIT_CODE;
  case RepairStatus::NO_NEGATIVE_TESTS:
    BOOST_LOG_TRIVIAL(info) << "nothing to do";
    return NO_NEGATIVE_TESTS_EXIT_CODE;
  }

  return ERROR_EXIT_CODE;
}

int c_repair_main(int argc, char *argv[], C_SearchEngine ** c_engine){
  SearchEngine *engine = NULL;
  int status = repair_main(argc, argv, engine);
  *c_engine = SearchEngine_TO_C(engine);
  return status;
}

int main(int argc, char *argv[]){
  SearchEngine *engine = NULL;
  int status = repair_main(argc, argv, engine);
  return status;
}
