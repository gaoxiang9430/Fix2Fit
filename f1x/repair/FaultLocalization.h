#pragma once

#include <unordered_map>
#include <unordered_set>

#include "Project.h"
#include "Util.h"

typedef std::unordered_map<std::string, std::unordered_set<unsigned>> Coverage;

Coverage extractAndSaveCoverage(boost::filesystem::path coverageFile);

class FaultLocalization {
public:
  FaultLocalization(const std::vector<std::string> &tests, const TestingFramework &tester);
  std::vector<boost::filesystem::path> localize(std::vector<boost::filesystem::path> allFiles);

private:
  TestingFramework tester;
  std::vector<std::string> tests;
  boost::filesystem::path coverageDir;
};
