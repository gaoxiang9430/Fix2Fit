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
#include <chrono>

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

#include <boost/program_options.hpp>

#include <boost/log/trivial.hpp>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/writer.h>

#include "Config.h"
#include "Util.h"


namespace po = boost::program_options;
namespace fs = boost::filesystem;
namespace json = rapidjson;

using std::vector;
using std::string;


enum class ExperimentStatus {
  SUCCESS, FAILURE, ERROR, TIMEOUT
};


std::string statusToString(ExperimentStatus status) {
  switch (status) {
  case ExperimentStatus::SUCCESS:
    return "SUCCESS";
  case ExperimentStatus::FAILURE:
    return "FAILURE";
  case ExperimentStatus::ERROR:
    return "ERROR";
  case ExperimentStatus::TIMEOUT:
    return "TIMEOUT";
  }
}

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


std::string formatMilliseconds(unsigned long milliseconds) {
  std::ostringstream out;
  if (milliseconds >= 1000) {
    unsigned seconds = (milliseconds / 1000) % 60;
    unsigned minutes = ((milliseconds / (1000 * 60)) % 60);
    unsigned hours = ((milliseconds / (1000 * 60 * 60)) % 24);
    unsigned days = (milliseconds / (1000 * 60 * 60 * 24));
    if ((days == 0) && (hours != 0)){
      out << hours << "h " << minutes << "m " << seconds << "s";
    } else if ((hours == 0) && (minutes != 0)) {
      out << minutes << "m " << seconds << "s";
    } else if ((days == 0) && (hours == 0) && (minutes == 0)) {
      out << seconds << "s";
    } else {
      out << days << "d " << hours << "h " << minutes << "m " << seconds << "s";
    }
  } else {
    out << "<1s";
  }
  return out.str();
}


bool fetch(std::string defectId, const json::Document &benchmark) {
  auto defect = benchmark.GetObject()[defectId.c_str()].GetObject();
  std::string fetch = defect["fetch"].GetString();
  std::stringstream fetchCmd;
  fetchCmd << fetch << " >/dev/null 2>&1";
  BOOST_LOG_TRIVIAL(debug) << defectId << "/fetch: " << fetchCmd.str();
  unsigned long fetchStatus = std::system(fetchCmd.str().c_str());
  if (WEXITSTATUS(fetchStatus) != 0) {
    return false;
  }
  return true;
}


bool setUp(std::string defectId, const json::Document &benchmark) {
  auto defect = benchmark.GetObject()[defectId.c_str()].GetObject();
  std::string setUp;
  if (defect.HasMember("set-up")) {
    setUp = defect["set-up"].GetString();
  } else {
    BOOST_LOG_TRIVIAL(debug) << defectId << "/set-up is undefined";
    return true;
  }
  std::stringstream setUpCmd;
  setUpCmd << setUp  << " >/dev/null 2>&1";
  BOOST_LOG_TRIVIAL(debug) << defectId << "/set-up: " << setUpCmd.str();
  unsigned long setUpStatus = std::system(setUpCmd.str().c_str());
  if (WEXITSTATUS(setUpStatus) != 0) {
    return false;
  }
  return true;
}


bool tearDown(std::string defectId, const json::Document &benchmark) {
  auto defect = benchmark.GetObject()[defectId.c_str()].GetObject();
  std::string tearDown;
  if (defect.HasMember("tear-down")) {
    tearDown = defect["tear-down"].GetString();
  } else {
    BOOST_LOG_TRIVIAL(debug) << defectId << "/tear-down is undefined";
    return true;
  }
  std::stringstream tearDownCmd;
  tearDownCmd << tearDown  << " >/dev/null 2>&1";
  BOOST_LOG_TRIVIAL(debug) << defectId << "/tear-down: " << tearDownCmd.str();
  unsigned long tearDownStatus = std::system(tearDownCmd.str().c_str());
  if (WEXITSTATUS(tearDownStatus) != 0) {
    return false;
  }
  return true;
}


std::string cmd(std::string defectId, const json::Document &benchmark, const json::Document &alltests, unsigned timeout, fs::path output, bool verbose) {
  auto defect = benchmark.GetObject()[defectId.c_str()].GetObject();
  std::string source = defect["source"].GetString();
  vector<std::string> files;
  for (json::SizeType i = 0; i < defect["files"].GetArray().Size(); i++)
    files.push_back(defect["files"].GetArray()[i].GetString());
  std::string build;
  if (defect.HasMember("build")) {
    build = defect["build"].GetString();
  }
  unsigned testTimeout = defect["test-timeout"].GetUint();
  std::string driver = defect["driver"].GetString();
  auto tests = alltests.GetObject()[defectId.c_str()].GetObject();
  vector<std::string> negative;
  for (json::SizeType i = 0; i < tests["negative"].GetArray().Size(); i++)
    negative.push_back(tests["negative"].GetArray()[i].GetString());
  vector<std::string> positive;
  for (json::SizeType i = 0; i < tests["positive"].GetArray().Size(); i++)
    positive.push_back(tests["positive"].GetArray()[i].GetString());

  std::stringstream f1xCmd;
  f1xCmd << "timeout " << timeout << "s" << " f1x " << source;
  f1xCmd << " --files";
  for (auto &file: files) {
    f1xCmd << " " << file;
  }
  f1xCmd << " --tests";
  for (auto &test: negative) {
    f1xCmd << " " << test;
  }
  for (auto &test: positive) {
    f1xCmd << " " << test;
  }
  f1xCmd << " --test-timeout " << testTimeout; 
  f1xCmd << " --driver " << driver;
  if (!build.empty()) {
    f1xCmd << " --build \"" << build << "\"";
  }
  fs::path patchFile = output / (defectId + ".patch");
  f1xCmd << " --output " << patchFile.string();
  if (verbose) {
    f1xCmd << " >&2";
  } else {
    f1xCmd << " --enable-cleanup";
    f1xCmd << " >/dev/null 2>&1";
  }

  return f1xCmd.str();
}


std::pair<ExperimentStatus, unsigned long> run(std::string defectId, const json::Document &benchmark, const json::Document &alltests, unsigned timeout, fs::path output, bool verbose) {
  std::string f1xCmd = cmd(defectId, benchmark, alltests, timeout, output, verbose);
  BOOST_LOG_TRIVIAL(debug) << defectId << "/run: " << f1xCmd;

  std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
  unsigned long f1xStatus = std::system(f1xCmd.c_str());
  std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
  
  unsigned int defectTime = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
  ExperimentStatus result;
  if (WEXITSTATUS(f1xStatus) == 0) {
    result = ExperimentStatus::SUCCESS;
  } else if (WEXITSTATUS(f1xStatus) == FAILURE_EXIT_CODE) {
    result = ExperimentStatus::FAILURE;
  } else if (WEXITSTATUS(f1xStatus) == TIMEOUT_EXIT_CODE) {
    result = ExperimentStatus::TIMEOUT;
  } else {
    result = ExperimentStatus::ERROR;
  }

  return std::make_pair(result, defectTime);
}


struct ExperimentPlan {
  bool fetch;
  bool setUp;
  bool run;
  bool tearDown;
  bool remove;
};


std::pair<ExperimentStatus, unsigned long> experiment(std::string defectId, 
                                                      fs::path root, 
                                                      fs::path output,
                                                      const json::Document &benchmark,
                                                      const json::Document &alltests,
                                                      unsigned timeout,
                                                      bool verbose,
                                                      ExperimentPlan plan) {

  InEnvironment env({ {"CC", "f1x-cc"},
                      {"CXX", "f1x-cxx"},
                      {"F1X_BENCH_OUTPUT", output.string()} });

  if (fs::exists(output)) {
    fs::create_directory(output);
  }

  fs::path defectDir(defectId);

  if (plan.fetch) {
    bool fetchSuccess = fetch(defectId, benchmark);
    if (! fetchSuccess) {
      BOOST_LOG_TRIVIAL(warning) << defectId << "/fetch returned non-zero exit code";
    }
  }

  if (plan.setUp) {
    bool setUpSuccess = setUp(defectId, benchmark);
    if (! setUpSuccess) {
      BOOST_LOG_TRIVIAL(warning) << defectId << "/set-up returned non-zero exit code";
    }
  }

  std::pair<ExperimentStatus, unsigned long> result;
  
  if (plan.run) {
    result = run(defectId, benchmark, alltests, timeout, output, verbose);
  }

  if (plan.tearDown) {
    bool tearDownSuccess = tearDown(defectId, benchmark);
    if (! tearDownSuccess) {
      BOOST_LOG_TRIVIAL(warning) << defectId << "/tear-down returned non-zero exit code";
    }
  }

  if (plan.remove) {
    BOOST_LOG_TRIVIAL(debug) << "removing " << defectDir;
    if (fs::exists(defectDir)) {
      fs::remove_all(defectDir);
    }
  }

  return result;
}


int main (int argc, char *argv[]) {
  po::positional_options_description positional;
  positional.add("defect", -1);
  
  // Declare supported options.
  po::options_description general("Usage: f1x-bench [DEFECT] OPTIONS -- TOOL_OPTIONS\n\nSupported options");
  general.add_options()
    ("output", po::value<string>()->value_name("PATH"), "output directory (default: results-TIME)")
    ("timeout", po::value<unsigned>()->value_name("SEC"), "timeout for individual defect (default: none)")
    ("fetch", "perform fetch command and exit")
    ("set-up", "perform set-up command and exit")
    ("run", "perform run command and exit")
    ("tear-down", "perform tear-down command and exit")
    ("cmd", "print f1x command and exit")
    ("verbose", "produce extended output")
    ("help", "produce help message and exit")
    ("version", "print version and exit")
    ;

  po::options_description hidden("Hidden options");
  hidden.add_options()  
    ("defect", po::value<string>(), "defect identifier")
    ;

  po::options_description allOptions("All options");
  allOptions.add(general).add(hidden);

  po::variables_map vm;

  try {
    po::store(po::command_line_parser(argc, argv).options(allOptions).positional(positional).run(), vm);
    po::notify(vm);
  } catch(po::error& e) {
    BOOST_LOG_TRIVIAL(error) << e.what() << " (use --help)";
    return 0;
  }

  if (vm.count("help")) {
    std::cout << general << std::endl;
    return 1;
  }

  if (vm.count("version")) {
    std::cout << "f1x-bench " << F1X_VERSION_MAJOR <<
                    "." << F1X_VERSION_MINOR <<
                    "." << F1X_VERSION_PATCH << std::endl;
    return 1;
  }

  bool verbose = false;
  if (vm.count("verbose")) {
    verbose = true;
  }

  initializeTrivialLogger(verbose);

  bool fetchOnly = false;
  if (vm.count("fetch")) {
    if (!vm.count("defect")) {
      BOOST_LOG_TRIVIAL(error) << "defect is not specified";
      return 0;
    } else {
      fetchOnly = true;
    }
  }

  bool setUpOnly = false;
  if (vm.count("set-up")) {
    if (!vm.count("defect")) {
      BOOST_LOG_TRIVIAL(error) << "defect is not specified";
      return 0;
    } else {
      setUpOnly = true;
    }
  }

  bool runOnly = false;
  if (vm.count("run")) {
    if (!vm.count("defect")) {
      BOOST_LOG_TRIVIAL(error) << "defect is not specified";
      return 0;
    } else {
      runOnly = true;
    }
  }

  bool tearDownOnly = false;
  if (vm.count("tear-down")) {
    if (!vm.count("defect")) {
      BOOST_LOG_TRIVIAL(error) << "defect is not specified";
      return 0;
    } else {
      tearDownOnly = true;
    }
  }

  bool cmdOnly = false;
  if (vm.count("cmd")) {
    if (!vm.count("defect")) {
      BOOST_LOG_TRIVIAL(error) << "defect is not specified";
      return 0;
    } else {
      cmdOnly = true;
    }
  }

  fs::path root(fs::current_path());
  BOOST_LOG_TRIVIAL(debug) << "benchmark root: " << root.string();
  if (! fs::exists(root / "benchmark.json")) {
    BOOST_LOG_TRIVIAL(error) << "benchmark.json does not exist";
    return 1;
  }
  if (! fs::exists(root / "tests.json")) {
    BOOST_LOG_TRIVIAL(error) << "tests.json does not exist";
    return 1;
  }

  unsigned timeout = 0; // means no timeout
  if (vm.count("timeout")) {
    timeout = vm["timeout"].as<unsigned>();
  }

  fs::path output;
  if (!vm.count("output")) {
    std::time_t now = std::time(0);
    struct std::tm tstruct;
    char timeRepr[80];
    tstruct = *localtime(&now);
    strftime(timeRepr, sizeof(timeRepr), "-%Y_%m_%d-%H_%M_%S", &tstruct);
    std::stringstream name;
    name << "experiment" << timeRepr;
    output = fs::path(name.str());
  } else {
    output = fs::path(vm["output"].as<string>());
  }

  output = fs::absolute(output);
  if (fs::exists(output)) {
    BOOST_LOG_TRIVIAL(debug) << "output directory " << output << " will be overwritten";
  } else {
    BOOST_LOG_TRIVIAL(debug) << "output directory: " << output;
    fs::create_directory(output);
  }

  fs::path benchmarkFile = root / "benchmark.json";
  fs::path testsFile = root / "tests.json";

  json::Document benchmark;
  {
    fs::ifstream ifs(benchmarkFile);
    json::IStreamWrapper isw(ifs);
    benchmark.ParseStream(isw);
  }

  json::Document alltests;
  {
    fs::ifstream ifs(testsFile);
    json::IStreamWrapper isw(ifs);
    alltests.ParseStream(isw);
  }



  vector<std::string> defects;

  if (vm.count("defect")) {
    if (cmdOnly) {
      std::string f1xCmd = cmd(vm["defect"].as<string>(), benchmark, alltests, timeout, output, verbose);
      std::cout << f1xCmd << std::endl;
      return 0;
    }
    defects.push_back(vm["defect"].as<string>());
  } else {
    for (json::Value::ConstMemberIterator itr = benchmark.MemberBegin(); itr != benchmark.MemberEnd(); ++itr) {
      defects.push_back(itr->name.GetString());
    }
  }

  unsigned long totalTime = 0;
  unsigned long totalRepaired = 0;
  unsigned long totalErrors = 0;
  unsigned long totalTimeouts = 0;

  for (auto &defect : defects) {
    unsigned long defectTime = 0;

    ExperimentPlan plan{ !(setUpOnly || runOnly || tearDownOnly),
                         !(fetchOnly || runOnly || tearDownOnly),
                         !(fetchOnly || setUpOnly || tearDownOnly),
                         !(fetchOnly || setUpOnly || runOnly),
                         !(fetchOnly || setUpOnly || runOnly || tearDownOnly) };

    std::pair<ExperimentStatus, unsigned long> result = experiment(defect, root, output, benchmark, alltests, timeout, verbose, plan);
    
    if (fetchOnly || setUpOnly || tearDownOnly)
      continue;
      
    totalTime += result.second;

    BOOST_LOG_TRIVIAL(info) << defect 
                            << "\t" << statusToString(result.first)
                            << "\t" << formatMilliseconds(result.second);

    if (result.first == ExperimentStatus::SUCCESS) {
      totalRepaired++;
    }
    if (result.first == ExperimentStatus::ERROR) {
      totalErrors++;
    }
    if (result.first == ExperimentStatus::TIMEOUT) {
      totalTimeouts++;
    }
    
  }

  if (defects.size() > 1 && !(fetchOnly || setUpOnly || tearDownOnly)) {
    BOOST_LOG_TRIVIAL(info) << "time: " << formatMilliseconds(totalTime);
    BOOST_LOG_TRIVIAL(info) << "repaired/errors/timeouts/total: " << totalRepaired << "/" << totalErrors << "/" << totalTimeouts << "/" << defects.size();
  }

  return 0;
}
