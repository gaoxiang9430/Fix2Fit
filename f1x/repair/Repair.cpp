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

#include <memory>
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <iomanip>
#include <map>
#include <set>
#include <vector>

#include <boost/filesystem/fstream.hpp>
#include <boost/log/trivial.hpp>

#include "Repair.h"
#include "Global.h"
#include "Typing.h"
#include "Util.h"
#include "Project.h"
#include "Runtime.h"
#include "Profiler.h"
#include "Synthesis.h"
//#include "SearchEngine.h"
#include "FaultLocalization.h"
#include "Prioritization.h"

namespace fs = boost::filesystem;
using std::vector;
using std::string;
using std::pair;
using std::shared_ptr;
using std::unordered_map;
using std::unordered_set;
using std::string;
using std::to_string;

const string APPLICATIONS_FILE_PREFIX = "applications";


void prioritize(vector<Patch> &searchSpace,
                unordered_map<PatchID, double> &cost) {
  std::stable_sort(searchSpace.begin(),
                   searchSpace.end(),
                   [&cost](const Patch &a, const Patch &b) -> bool {
                     return cost[a.id] < cost[b.id];
                   });
}


//FIXME: this should be a mapping from a patch to a set of patches and should be computed during search space generation
shared_ptr<unordered_map<unsigned long, unordered_set<PatchID>>> getPartitionable(const std::vector<Patch> &searchSpace) {
  shared_ptr<unordered_map<unsigned long, unordered_set<PatchID>>> result(new unordered_map<ulong, unordered_set<PatchID>>);
  for (auto &el : searchSpace) {
    unsigned long locId = el.app->id;
    if (! result->count(locId)) {
      (*result)[locId] = unordered_set<PatchID>();
    }
    (*result)[locId].insert(el.id);
  }
  return result;
}


bool validatePatch(Project &project,
                   TestingFramework &tester,
                   const std::vector<std::string> &tests,
                   Patch &patch) {

    bool appSuccess = project.applyPatch(patch);
    if (! appSuccess) {
      BOOST_LOG_TRIVIAL(warning) << "patch application returned non-zero code";
    }
    bool rebuildSuccess = project.build();
    if (! rebuildSuccess) {
      BOOST_LOG_TRIVIAL(warning) << "compilation with patch returned non-zero exit code";
    }

    BOOST_LOG_TRIVIAL(info) << "validating patch " << visualizePatchID(patch.id);
    vector<string> failingTests;

    for (auto &test : tests) {
      if (tester.execute(test) != TestStatus::PASS) {
        failingTests.push_back(test);
      }
    }

    project.restoreOriginalFiles();
    
    if (!failingTests.empty()) {
      BOOST_LOG_TRIVIAL(warning) << "generated patch failed validation";
      for (auto &t : failingTests) {
        BOOST_LOG_TRIVIAL(info) << "failed test: " << t;
      }
      return false;
    }
    return true;
}

//TODO: call AFLGo to generate a test case that can reach target location loc
/**
 * @param loc: the location of corresponding patch
 * @param originalPartiion: the partition of existing tests, a mapping from test to equivalent patch set
 * @param isPassing: the execution result of last test (Pass: true, Fail: false)
 * @param partition: equivalent patch set of the last test (NULL: for the first query)
 */
std::basic_string<char> generateTest(Project project, const Patch patch, unordered_map<__string, unordered_set<PatchID>> * originalPartition,
                                     bool isPassing, unordered_set<PatchID> *partition){
  Location loc = patch.app->location;
  BOOST_LOG_TRIVIAL(info) << "patch location " << loc.beginLine;
  string F1X_APP = to_string(patch.app->id);
  string F1X_ID_BASE = to_string(patch.id.base);
  string F1X_ID_INT2 = to_string(patch.id.int2);
  string F1X_ID_BOOL2 = to_string(patch.id.bool2);
  string F1X_ID_COND2 = to_string(patch.id.cond3);
  string F1X_ID_PARAM = to_string(patch.id.param);
  BOOST_LOG_TRIVIAL(info) << "Environment Variables: " << F1X_APP << " " << F1X_ID_BASE << " " << F1X_ID_INT2 << " " << F1X_ID_BOOL2 << " " << F1X_ID_COND2 << " " << F1X_ID_PARAM;
  BOOST_LOG_TRIVIAL(info) << "working directory: " << cfg.dataDir;
  
  std::stringstream callAFLGOfromF1X;
  callAFLGOfromF1X << "callAFLGOfromF1X";
  bool findNewTest = project.buildInEnvironment({ {"F1X_APP", to_string(patch.app->id)}, 
                                          {"F1X_ID_BASE", to_string(patch.id.base)}, 
                                          {"F1X_ID_INT2", to_string(patch.id.int2)},
                                          {"F1X_ID_BOOL2", to_string(patch.id.bool2)},
                                          {"F1X_ID_COND2", to_string(patch.id.cond3)},
                                          {"F1X_ID_PARAM", to_string(patch.id.param)},
                                          {"LD_LIBRARY_PATH", cfg.dataDir}}, 
                                        callAFLGOfromF1X.str());
  BOOST_LOG_TRIVIAL(info) << "find Next test: " << findNewTest;
  return "random"; //fake test
}

bool validateByFuzzing(Project project, SearchEngine *engine, Patch patch, int patchIndex, unordered_set<PatchID> allPatch,
                      unordered_map<__string, unordered_set<PatchID>> * originalPartition){
  unordered_map<__string, unordered_set<PatchID>> executionStat;
  bool isPassing = true;
  //geneate first test case by fuzzing
  auto test = generateTest(project, patch, originalPartition, isPassing, NULL); 
  //while (true) {
    //isPassing &= engine->evaluatePatchWithNewTest(patch, test, patchIndex, &executionStat);
    //BOOST_LOG_TRIVIAL(info) << "partition Size " << executionStat[test].size();
    //if(isPassing == false)
    //  break;

    //generate new test case
    //test = generateTest(loc, originalPartition, isPassing, &executionStat[test]);
 //}
  return isPassing;
}

RepairStatus repair(Project &project,
                    TestingFramework &tester,
                    const std::vector<std::string> &tests,
                    const boost::filesystem::path &patchOutput,
                    SearchEngine * &engine) {

  pair<bool, bool> initialBuildStatus = project.initialBuild();
  if (! initialBuildStatus.first) {
    BOOST_LOG_TRIVIAL(warning) << "compilation returned non-zero exit code";
  }
  if (! initialBuildStatus.second) {
    BOOST_LOG_TRIVIAL(error) << "failed to infer compile commands";
    return RepairStatus::ERROR;
  }

  //NOTE: checking here because it can be compiled
  if (!tester.driverIsOK()) {
    BOOST_LOG_TRIVIAL(error) << "driver does not exist or not executable";
    return RepairStatus::ERROR;
  }

  if (project.getFiles().empty()) {
    BOOST_LOG_TRIVIAL(info) << "localizing suspicious files";
    FaultLocalization faultLocal(tests,tester);
    vector<fs::path> allFiles = project.filesFromCompilationDB();
    vector<fs::path> localized = faultLocal.localize(allFiles);
    if (localized.size() == 0) {
      BOOST_LOG_TRIVIAL(warning) << "no files localized";
      return RepairStatus::FAILURE;
    }
    BOOST_LOG_TRIVIAL(info) << "number of localized files: " << localized.size();
    std::vector<ProjectFile> projectFiles;
    for (auto &file : localized) {
      projectFiles.push_back(ProjectFile{file, 0, 0});
    }
    project.setFiles(projectFiles);
  }

  fs::path traceFile = fs::path(cfg.dataDir) / TRACE_FILE_NAME;

  BOOST_LOG_TRIVIAL(info) << "instrumenting source files for profiling";
  for (auto &file : project.getFiles()) {
    bool profileInstSuccess = project.instrumentFile(file, traceFile);
    if (! profileInstSuccess) {
      BOOST_LOG_TRIVIAL(warning) << "profiling instrumentation of " << file.relpath << " returned non-zero exit code";
    }
  }
  project.saveProfileInstumentedFiles();

  Profiler profiler;

  bool profilerBuildSuccess = profiler.compile();
  if (! profilerBuildSuccess) {
    BOOST_LOG_TRIVIAL(error) << "profiler runtime compilation failed";
    return RepairStatus::ERROR;
  }

  bool profileRebuildSucceeded = project.buildWithRuntime(profiler.getHeader());
  if (! profileRebuildSucceeded) {
    BOOST_LOG_TRIVIAL(warning) << "compilation with profiler runtime returned non-zero exit code";
  }

  project.restoreOriginalFiles();

  BOOST_LOG_TRIVIAL(info) << "profiling project";
  vector<string> negativeTests;
  unsigned long numPositive = 0;
  unsigned long numNegative = 0;
  for (int i = 0; i < tests.size(); i++) {
    auto test = tests[i];
    profiler.clearTrace();
    TestStatus status = tester.execute(test);
    if (status == TestStatus::PASS)
      numPositive++;
    else {
      numNegative++;
      negativeTests.push_back(test);
    }
    if (status == TestStatus::TIMEOUT)
      BOOST_LOG_TRIVIAL(warning) << "test " << test << " timeout during profiling";
    profiler.mergeTrace(i, (status == TestStatus::PASS));
  }
  if (numNegative == 0) {
    BOOST_LOG_TRIVIAL(error) << "no negative tests";
    return RepairStatus::NO_NEGATIVE_TESTS;
  }

  BOOST_LOG_TRIVIAL(info) << "number of positive tests: " << numPositive;
  BOOST_LOG_TRIVIAL(info) << "number of negative tests: " << numNegative;
  BOOST_LOG_TRIVIAL(info) << "negative tests: " << prettyPrintTests(negativeTests);

  if(!cfg.binaryPath.empty() && !cfg.binaryName.empty()){
    BOOST_LOG_TRIVIAL(info) << "binary path " << cfg.binaryPath;
    BOOST_LOG_TRIVIAL(info) << "binary name " << cfg.binaryName;
    std::stringstream cmdCopyBinary;
    cmdCopyBinary << "cp"
                  << " " << cfg.binaryPath << "/" << cfg.binaryName
                  << " " << cfg.binaryPath << "/" << cfg.binaryName << "_profile";
    unsigned long status = std::system(cmdCopyBinary.str().c_str());
    BOOST_LOG_TRIVIAL(info) << "copy profile executable: " << WEXITSTATUS(status);
  }

  if (cfg.patchPrioritization == PatchPrioritization::SEMANTIC_DIFF)
    project.deleteCoverageFiles();
 
  fs::path profile = profiler.getProfile();

  auto relatedTestIndexes = profiler.getRelatedTestIndexes();
  BOOST_LOG_TRIVIAL(info) << "number of locations: " << relatedTestIndexes.size();
  
  vector<fs::path> saFiles;

  BOOST_LOG_TRIVIAL(info) << "applying transfomation schemas to source files";
  for (int i=0; i<project.getFiles().size(); i++) {
    std::stringstream schemaAppFile;
    schemaAppFile << APPLICATIONS_FILE_PREFIX << i << ".json";
    fs::path saFile = fs::path(cfg.dataDir) / schemaAppFile.str();
    saFiles.push_back(saFile);
    bool instrSuccess = project.instrumentFile(project.getFiles()[i], saFile, &profile);
    if (! instrSuccess) {
      BOOST_LOG_TRIVIAL(warning) << "transformation returned non-zero exit code";
    }
    if (! fs::exists(saFile)) {
      BOOST_LOG_TRIVIAL(error) << "failed to extract candidate locations";
      return RepairStatus::ERROR;
    }
  }

  project.saveInstrumentedFiles();

  BOOST_LOG_TRIVIAL(debug) << "loading candidate locations";
  vector<shared_ptr<SchemaApplication>> sas = loadSchemaApplications(saFiles);
  
  BOOST_LOG_TRIVIAL(debug) << "inferring types";
  for (auto sa : sas) {
    Type context;
    if (sa->context == LocationContext::CONDITION)
      context = Type::BOOLEAN;
    else
      context = Type::ANY;
    sa->original = correctTypes(sa->original, context);
  }

  vector<Patch> searchSpace;

  Runtime runtime;

  BOOST_LOG_TRIVIAL(info) << "generating search space";
  {
    fs::ofstream os(runtime.getSource());
    fs::ofstream oh(runtime.getHeader());
    searchSpace = generateSearchSpace(sas, os, oh);
  }

  BOOST_LOG_TRIVIAL(info) << "search space size: " << searchSpace.size();

  bool runtimeSuccess = runtime.compile();

  if (! runtimeSuccess) {
    BOOST_LOG_TRIVIAL(error) << "runtime compilation failed";
    return RepairStatus::ERROR;
  }

  bool rebuildSucceeded = project.buildWithRuntime(runtime.getHeader());

  if (! rebuildSucceeded) {
    BOOST_LOG_TRIVIAL(warning) << "compilation with runtime returned non-zero exit code";
  }

  project.restoreOriginalFiles();

  unordered_map<PatchID, double> cost;

  for (auto &el : searchSpace)
    cost[el.id] = syntacticDiff(el);

  BOOST_LOG_TRIVIAL(info) << "prioritizing search space";
  prioritize(searchSpace, cost);

  if (!cfg.searchSpaceFile.empty()) {
    auto path = fs::path(cfg.searchSpaceFile);
    BOOST_LOG_TRIVIAL(info) << "dumping search space: " << path;
    vector<fs::path> filePaths;
    for (auto &pFile: project.getFiles())
      filePaths.push_back(pFile.relpath);
    dumpSearchSpace(searchSpace, path, filePaths, cost);
  }

  shared_ptr<unordered_map<unsigned long, unordered_set<PatchID>>> partitionable = getPartitionable(searchSpace);
  //SearchEngine engine(tests, tester, runtime, partitionable, relatedTestIndexes);
  engine = new SearchEngine(searchSpace, tests, tester, runtime, partitionable, relatedTestIndexes, patchOutput);

  unsigned long last = 0;
  unordered_set<AppID> fixLocations;
  unordered_set<AppID> moreThanOneFound;

  vector<Patch> plausiblePatches;

  // generate plausible patches
  while (last < searchSpace.size()) {
    last = engine->findNext(searchSpace, last);
    if (last == searchSpace.size())
      break;

    if (cfg.outputTop && plausiblePatches.size() >= cfg.outputTop) {
      BOOST_LOG_TRIVIAL(info) << "found enough patches";
      break;
    }

    Patch patch = searchSpace[last];

    if (!moreThanOneFound.count(patch.app->id) || cfg.verbose) {
      fs::path relpath = project.getFiles()[patch.app->location.fileId].relpath;
      if (!fixLocations.count(patch.app->id) || cfg.verbose) {
        BOOST_LOG_TRIVIAL(info) << "plausible patch: " << visualizeChange(patch) 
                                << " in " << relpath.string() << ":" << patch.app->location.beginLine;
      } else {
        BOOST_LOG_TRIVIAL(info) << "more patches found in " << relpath.string() << ":" << patch.app->location.beginLine;
      }
    }

    //NOTE: if we generate all patches, then just save the current one and apply/validate later;
    // if we generate a single patch, then validate now and continue search if it fails

    if (! cfg.generateAll) {
      bool valid = true;
      if (cfg.validatePatches)
        valid = validatePatch(project, tester, tests, patch);
//      if (cfg.validatePatchesByFuzzing)
//        valid &= validateByFuzzing(project, engine, patch, last, (*partitionable)[patch.app->id], &executionStat);
      if (valid) {
        fixLocations.insert(patch.app->id);
        plausiblePatches.push_back(patch);
        break;
      } else {
        project.restoreInstrumentedFiles();
        project.buildWithRuntime(runtime.getHeader());
      }
    } else {
      bool valid = true;
//      if (cfg.validatePatchesByFuzzing)
//        valid &= validateByFuzzing(project, engine, patch, last, (*partitionable)[patch.app->id], &executionStat);
      if(valid){
        if (fixLocations.count(patch.app->id))
          moreThanOneFound.insert(patch.app->id);
        fixLocations.insert(patch.app->id);
        plausiblePatches.push_back(patch);
      }
    }

    last++;
  }

  // validate patches if needed
  if (cfg.validatePatches && cfg.generateAll && plausiblePatches.size() > 0) {
    vector<Patch> validPatches;
    for (auto &patch : plausiblePatches) {
      if (validatePatch(project, tester, tests, patch)) {
        validPatches.push_back(patch);
      }
    }
    plausiblePatches = validPatches;
  }

  if (cfg.patchPrioritization == PatchPrioritization::SEMANTIC_DIFF) {
    auto coverageSet = engine->getCoverageSet();
    for (auto &testCoverage : coverageSet) {
      BOOST_LOG_TRIVIAL(debug) << "test: " << testCoverage.first;
      std::unordered_map<PatchID, std::shared_ptr<Coverage>> patchCoverage = testCoverage.second;
      for (auto &patch : plausiblePatches) {
        BOOST_LOG_TRIVIAL(debug) << "patch: " << visualizePatchID(patch.id);
        Coverage coverage = *patchCoverage[patch.id];
        for (auto &entry : coverage) {
          BOOST_LOG_TRIVIAL(debug) << "file: " << entry.first;
          for (auto &line : entry.second) {
            BOOST_LOG_TRIVIAL(debug) << "line: " << line;
          }
        }
      }
    }
  }

  if (plausiblePatches.size() > 0) {
    BOOST_LOG_TRIVIAL(info) << "computing source diffs";
    if (! cfg.generateAll) {
      unsigned fileId = plausiblePatches[0].app->location.fileId;
      project.applyPatch(plausiblePatches[0]);
      project.computeDiff(project.getFiles()[fileId], patchOutput);
      project.restoreOriginalFiles();
    } else {
      if (! fs::exists(patchOutput)) {
        fs::create_directory(patchOutput);
      }
      unordered_set<unsigned long> patchLocations;
      for (int i=0; i<plausiblePatches.size(); i++) {
        if (cfg.outputOnePerLocation && patchLocations.count(plausiblePatches[i].app->id))
          continue;
        patchLocations.insert(plausiblePatches[i].app->id);
        //fs::path patchFile = patchOutput / (std::to_string(i) + ".patch");
        fs::path patchFile = patchOutput / (visualizePatchID(plausiblePatches[i].id) + ".patch");
        project.applyPatch(plausiblePatches[i]);
        unsigned fileId = plausiblePatches[i].app->location.fileId;
        project.computeDiff(project.getFiles()[fileId], patchFile);
        project.restoreOriginalFiles();
      }
    }
  }

  SearchStatistics stat = engine->getStatistics();

  BOOST_LOG_TRIVIAL(info) << "candidates evaluated: " << stat.explorationCounter;
  BOOST_LOG_TRIVIAL(info) << "tests executed: " << stat.executionCounter;
  BOOST_LOG_TRIVIAL(info) << "executions with timeout: " << stat.timeoutCounter;
  if (stat.nonTimeoutTestTime != 0) {
    double executionsPerSec = (stat.nonTimeoutCounter * 1000.0) / stat.nonTimeoutTestTime;
    BOOST_LOG_TRIVIAL(info) << "execution speed: " << std::setprecision(3) << executionsPerSec << " exe/sec";
  }
  BOOST_LOG_TRIVIAL(info) << "number of equivalent partition: " << engine->partitionIndex;
  BOOST_LOG_TRIVIAL(info) << "plausible patches: " << plausiblePatches.size();
  BOOST_LOG_TRIVIAL(info) << "fix locations: " << fixLocations.size();

  if (plausiblePatches.size() > 0)
    return RepairStatus::SUCCESS;
  else
    return RepairStatus::FAILURE;
}
