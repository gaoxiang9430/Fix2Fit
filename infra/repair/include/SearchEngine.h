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

#ifdef __cplusplus
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <map>
#include <vector>
#include "Util.h"
#include "Project.h"
#include "Runtime.h"
#include "FaultLocalization.h"

#include <boost/filesystem.hpp>

struct SearchStatistics {
  unsigned long explorationCounter;
  unsigned long executionCounter;
  unsigned long timeoutCounter;
  unsigned long nonTimeoutCounter;
  unsigned long nonTimeoutTestTime;
};

class SearchEngine {
 public:
  SearchEngine(const std::vector<Patch> &searchSpace,
               const std::vector<std::string> &tests,
               TestingFramework &tester,
               Runtime &runtime,
               std::shared_ptr<std::unordered_map<unsigned long, std::unordered_set<PatchID>>> partitionable,
               std::unordered_map<Location, std::vector<unsigned>> relatedTestIndexes,
               const boost::filesystem::path &patchOutput, Location crashLine);

  unsigned long findNext(const std::vector<Patch> &searchSpace, unsigned long fromIdx);
  std::unordered_map<std::string, std::unordered_map<PatchID, std::shared_ptr<Coverage>>> getCoverageSet();
  SearchStatistics getStatistics();
  void showProgress(unsigned long current, unsigned long total);
  int evaluatePatchWithNewTest(__string &test, char* reachedLocs, struct C_ExecutionStat*);
  const char* getWorkingDir();
  void getPatchLoc(int &length, char *& array);
  int getNumPlausiblePatch();
  unsigned long partitionIndex;
  Location crashLine;

 private:
  bool executeCandidate(const Patch elem, std::unordered_set<PatchID> &partition, __string &test, int index);
  void prioritizeTest(std::vector<unsigned> &testOrder, unsigned index);
  int mergePartition(std::unordered_map<PatchID, int>, std::unordered_map<PatchID, int>, int&);
  std::unordered_set<PatchID> mergePartition2(std::unordered_set<PatchID>, std::unordered_set<PatchID>);
  void removeFailedPatches(std::unordered_set<PatchID>);
  void saveExpectedFilteredParitionSize(double, std::unordered_set<PatchID>);
  void savePathIdforRanking();
  void initPatchDistance();
  std::vector<std::string> tests;
  TestingFramework tester;
  int totalBrokenPartition;
  int numTestReducePlausiblePatches;
  int numTestBreakPartition;
  int numCorrectPartition;
  const std::vector<Patch> searchSpace;
  Runtime runtime;
  SearchStatistics stat;
  unsigned long progress;
  boost::filesystem::path patchOutput;
  std::shared_ptr<std::unordered_map<unsigned long, std::unordered_set<PatchID>>> partitionable;
  std::unordered_set<PatchID> failing;
  std::unordered_map<std::string, std::unordered_set<PatchID>> passing;

  std::unordered_map<unsigned long, std::unordered_set<PatchID>> currentPartition;
  std::unordered_map<unsigned long, int> partitionCorrect;
  std::unordered_map<unsigned long, double> correctProbabilityPartition;
  std::unordered_map<unsigned long, std::unordered_set<PatchID>> brokenPartition;
  std::unordered_map<unsigned long, double> factorOfPartition;
  std::unordered_map<unsigned long, unsigned long> numCorrectTest;
  std::unordered_map<unsigned long, unsigned long> numIncorrectTest;
  int savedPartitionIndex;

  std::unordered_map<std::string, std::unordered_map<PatchID, std::shared_ptr<Coverage>>> coverageSet;
  std::unordered_map<Location, std::vector<unsigned>> relatedTestIndexes;
  boost::filesystem::path coverageDir;
  std::unordered_set<Location> vLocs;
  std::unordered_map<std::string, AppID> locToId;
  std::list<PatchDistance> patchDis;
};
#endif

#ifdef __cplusplus
extern "C" {
#endif
struct C_ExecutionStat{
  int numPlausiblePatch;
  int numPartition;
  int numBrokenPartition;
  int totalNumBrokenPartition;
  int numTestReducePlausiblePatches;
  int numTestBreakPartition;
  int sizeofBrokenPartition;
};
struct C_SearchEngine;
const char* c_getWorkingDir(struct C_SearchEngine*);
int c_fuzzPatch(struct C_SearchEngine*, char*, char*, struct C_ExecutionStat*);
void c_getPatchLoc(struct C_SearchEngine* engine, int* length, char ** array);
void c_getCrashLoc(struct C_SearchEngine* engine, char ** crash_loc);
int c_getNumPlausiblePatch(struct C_SearchEngine* engine);
#ifdef __cplusplus
}
#endif
#define SearchEngine_TO_CPP(o) (reinterpret_cast<SearchEngine*>(o))
#define SearchEngine_TO_C(o)   (reinterpret_cast<struct C_SearchEngine*>(o))
