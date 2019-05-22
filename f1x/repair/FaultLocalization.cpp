#include <rapidxml/rapidxml.hpp>
#include <rapidxml/rapidxml_utils.hpp>

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/writer.h>

#include <boost/log/trivial.hpp>
#include <boost/filesystem/fstream.hpp>

#include <string>
#include <sstream>

#include "FaultLocalization.h"
#include "Global.h"

namespace fs = boost::filesystem;
namespace json = rapidjson;
using namespace rapidxml;

using std::vector;
using std::string;
using std::unordered_map;
using std::unordered_set;


const bool USE_CUSTOM_SCORE = true;


Coverage extractAndSaveCoverage(fs::path coverageFile) {
  Coverage coverage;

  std::stringstream cmd;
  cmd << "gcovr --delete --xml --output=" << coverageFile.string();
  if (cfg.useLLVMCov)
    cmd << " --gcov-executable=f1x-llvm-cov";
  cmd << " >/dev/null 2>&1";
  BOOST_LOG_TRIVIAL(debug) << "cmd: " << cmd.str();
  unsigned long status = std::system(cmd.str().c_str());
  if (WEXITSTATUS(status) != 0) {
    throw std::runtime_error("gcovr failed");
  }

  rapidxml::file<> xmlFile(coverageFile.string().c_str());
  rapidxml::xml_document<> doc;
  doc.parse<0>(xmlFile.data());

  xml_node<> *root = doc.first_node()->first_node("packages")->first_node();
  if (!root)
    throw std::runtime_error("empty coverage file");
  xml_node<> *classRoot = root->first_node()->first_node();
  while(classRoot) {
    std::stringstream filename;
    filename << classRoot->first_attribute("filename")->value();
    string file = relativeTo(fs::current_path(), fs::path(filename.str())).string();
    unordered_set<unsigned> lines;
    xml_node<> *lineRoot = classRoot->first_node("lines")->first_node();
    while(lineRoot) {
      std::stringstream hits;
      hits << lineRoot->first_attribute("hits")->value();
      if (std::stoul(hits.str()) > 0) {
        std::stringstream line;
        line << lineRoot->first_attribute("number")->value();
        lines.insert(std::stoul(line.str()));
      }
      lineRoot = lineRoot->next_sibling();
    }
    coverage[file] = lines;
    classRoot = classRoot->next_sibling();
  }

  return coverage;
}


// From "Empirical Evaluation of the Tarantula Automatic Fault-Localization Technique" by James A. Jones and Mary Jean Harrold
double tarantula(unsigned passedStmt, unsigned failedStmt, unsigned totalPassed, unsigned totalFailed) {
  double a = (totalFailed == 0 ? 0 : (double) failedStmt / (double) totalFailed);
  double b = (totalPassed == 0 ? 0 : (double) passedStmt / (double) totalPassed);
  return (a + b == 0 ? 0 : a / (a + b));
}


// assuming statement should be executed by all failing tests
double tarantula_custom(unsigned passedStmt, unsigned failedStmt, unsigned totalPassed, unsigned totalFailed) {
  double a = (totalFailed == failedStmt ? 1 : 0);
  double b = (totalPassed == 0 ? 0 : (double) passedStmt / (double) totalPassed);
  return (a + b == 0 ? 0 : a / (a + b));
}


FaultLocalization::FaultLocalization(const std::vector<std::string> &tests, 
                                     const TestingFramework &tester):
  tests(tests),
  tester(tester) {
  coverageDir = fs::path(cfg.dataDir) / "coverage";
  fs::create_directory(coverageDir);
}


vector<fs::path> FaultLocalization::localize(vector<fs::path> allFiles) {
  // test -> file -> set of locations
  unordered_map<string, Coverage> coverage;
  unordered_set<string> passedTests;

  for (auto &test : tests) {
    TestStatus status = tester.execute(test);

    fs::path coverageFile = coverageDir / (test + ".xml");
    try {
      switch (status) {
      case TestStatus::PASS:
        passedTests.insert(test);
        coverage[test] = extractAndSaveCoverage(coverageFile);
        break;
      case TestStatus::FAIL:
        coverage[test] = extractAndSaveCoverage(coverageFile);
        break;
      case TestStatus::TIMEOUT:
        //FIXME: skipping this case because it requires runtime support
        BOOST_LOG_TRIVIAL(warning) << "localization for tests with timeout is not supported";
        break;
      }
    } catch (const std::exception& e) {
      BOOST_LOG_TRIVIAL(warning) << "failed to extract coverage: " << e.what();
    }
  }

  // file -> line -> score
  unordered_map<string, unordered_map<unsigned, double>> score;

  unsigned totalPassed = passedTests.size();
  unsigned totalFailed = tests.size() - passedTests.size();

  //FIXME: should I make sure that all paths are canonical?
  for (auto &file : allFiles) {
    unordered_set<unsigned> allLines;
    unordered_map<unsigned, unsigned> numPassed;
    unordered_map<unsigned, unsigned> numFailed;
    unordered_map<unsigned, double> lineScore;
    string filename = file.string();
    for (auto &test : tests) {
      if (coverage[test].find(filename) != coverage[test].end()) {
        for (unsigned line : coverage[test][filename]) {
          allLines.insert(line);
          if (passedTests.count(test)) {
            if (numPassed.find(line) != numPassed.end()){
              numPassed[line]++;
            } else {
              numPassed[line] = 1;
            }
          } else {
            if (numFailed.find(line) != numFailed.end()){
              numFailed[line]++;
            } else {
              numFailed[line] = 1;
            }
          }
        }
      }
    }

    for (unsigned line : allLines) {
      unsigned passedStmt = 0;
      if (numPassed.find(line) != numPassed.end()){
        passedStmt = numPassed[line];
      }
      unsigned failedStmt = 0;
      if (numFailed.find(line) != numFailed.end()){
        failedStmt = numFailed[line];
      }
      if (USE_CUSTOM_SCORE)
        lineScore[line] = tarantula_custom(passedStmt, failedStmt, totalPassed, totalFailed);
      else
        lineScore[line] = tarantula(passedStmt, failedStmt, totalPassed, totalFailed);
    }

    score[filename] = lineScore;
  }

  // file -> score
  unordered_map<string, double> fileScore;

  for (auto &file : allFiles) {
    string filename = file.string();
    double sum = 0.0;
    for (auto &lineEntry : score[filename])
      sum += lineEntry.second;
    fileScore[filename] = sum;
    BOOST_LOG_TRIVIAL(debug) << "file " << filename << " score: " << sum;
  }

  std::stable_sort(allFiles.begin(),
                   allFiles.end(), 
                   [&fileScore](const fs::path &a, const fs::path &b) -> bool {
                     string aName = a.string();
                     string bName = b.string();
                     return fileScore[aName] > fileScore[bName];
                   });

  unsigned length;
  
  for (length = 0;
       length < allFiles.size()
         && length < cfg.filesToLocalize
         && fileScore[allFiles[length].string()] > 0.0;
       length++) {}

  return vector<fs::path>(allFiles.begin(), allFiles.begin() + length);
}
