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

#include <string>
#include <cstdlib>
#include <sstream>
#include <memory>
#include <chrono>
#include <iostream>

#include <boost/log/trivial.hpp>
#include <boost/filesystem/fstream.hpp>

#include "Config.h"
#include "Global.h"
#include "SearchEngine.h"
#include "Util.h"

using std::unordered_set;
using std::unordered_map;
using std::shared_ptr;
using std::string;
using std::to_string;
using std::vector;

namespace fs = boost::filesystem;

const unsigned SHOW_PROGRESS_STEP = 10;

SearchEngine::SearchEngine(const std::vector<Patch> &searchSpace,
                           const std::vector<std::string> &tests,
                           TestingFramework &tester,
                           Runtime &runtime,
                           shared_ptr<unordered_map<unsigned long, unordered_set<PatchID>>> partitionable,
                           std::unordered_map<Location, std::vector<unsigned>> relatedTestIndexes, const boost::filesystem::path &patchOutput):
  searchSpace(searchSpace),
  tests(tests),
  tester(tester),
  runtime(runtime),
  partitionable(partitionable),
  relatedTestIndexes(relatedTestIndexes),
  patchOutput(patchOutput) {
  
  stat.explorationCounter = 0;
  stat.executionCounter = 0;
  stat.timeoutCounter = 0;
  stat.nonTimeoutCounter = 0;
  stat.nonTimeoutTestTime = 0;

  progress = 0;
  partitionIndex = 0;
  totalBrokenPartition = 0;
  numTestReducePlausiblePatches = 0;
  numTestBreakPartition = 0;
  savedPartitionIndex = 0;
  numCorrectPartition = 0;

  //FIXME: I should use evaluation table instead
  failing = {};
  passing = {};
  for (auto &test : tests) {
    passing[test] = {};
  }

  coverageDir = fs::path(cfg.dataDir) / "patch-coverage";
  fs::create_directory(coverageDir);
  vLocs.reserve(relatedTestIndexes.size());
}

string locToString2(const Location &loc) {
  std::ostringstream out;
  out << loc.fileId << " "
      << loc.beginLine << " "
      << loc.beginColumn << " "
      << loc.endLine << " "
      << loc.endColumn;
  return out.str();
}

void SearchEngine::showProgress(unsigned long current, unsigned long total) {
    if ((100 * current) / total >= progress) {
      BOOST_LOG_TRIVIAL(info) << "exploration progress: " << progress << "%";
      progress += SHOW_PROGRESS_STEP;
    }
}


SearchStatistics SearchEngine::getStatistics() {
  return stat;
}

std::unordered_map<std::string, std::unordered_map<PatchID, std::shared_ptr<Coverage>>> SearchEngine::getCoverageSet() {
  return coverageSet;
}

//FIXME: this probably does not work. Tests should be prioritized based on global score
void SearchEngine::prioritizeTest(std::vector<unsigned> &testOrder, unsigned index) {
    std::vector<unsigned>::iterator it = testOrder.begin() + index;
    unsigned temp = testOrder[index];
    testOrder.erase(it);
    testOrder.insert(testOrder.begin(), temp);
}

unsigned long SearchEngine::findNext(const std::vector<Patch> &searchSpace,
                                     unsigned long from) {

  unsigned long index = from;
  for (; index < searchSpace.size(); index++) {
    stat.explorationCounter++;
    showProgress(index, searchSpace.size());

    const Patch &elem = searchSpace[index];

    if (cfg.valueTEQ) {
      if (failing.count(elem.id))
        continue;
    }

    InEnvironment env({ { "F1X_APP", to_string(elem.app->id) },
                        { "F1X_ID_BASE", to_string(elem.id.base) },
                        { "F1X_ID_INT2", to_string(elem.id.int2) },
                        { "F1X_ID_BOOL2", to_string(elem.id.bool2) },
                        { "F1X_ID_COND3", to_string(elem.id.cond3) },
                        { "F1X_ID_PARAM", to_string(elem.id.param) } });

    bool passAll = true;

    //TODO: build the relationship for the newly generated tests
    std::vector<unsigned> testOrder = relatedTestIndexes[elem.app->location];

    unordered_set<PatchID> equivPartitionWithElem;

    for (unsigned orderIndex = 0; orderIndex < testOrder.size(); orderIndex++) {
      auto test = tests[testOrder[orderIndex]];

      unordered_set<PatchID> partition;
      if (cfg.valueTEQ) {
        if (passing[test].count(elem.id)){
          bool alreadyIncluded = false;
          for (auto it=currentPartition.begin(); it!=currentPartition.end(); ++it){
            if(it->second.count(elem.id)){
              alreadyIncluded = true;
              break;
            }
          }
          if(!alreadyIncluded)
            partition = passing[test];
        }
        else{
          //FIXME: select only unexplored candidates
          runtime.setPartition((*partitionable)[elem.app->id]);
          passAll = executeCandidate(elem, partition, test, index);
        }
      }
      
      if (!passAll) {
        if (cfg.testPrioritization == TestPrioritization::MAX_FAILING) {
          prioritizeTest(relatedTestIndexes[elem.app->location], orderIndex);
        }
        break;
      }else{
        //update current partition
        if(partition.size() > 0){
          if(orderIndex == 0)
            equivPartitionWithElem = partition;//init equiv. partition with patch elem
          else
            //if two patches reside in the equiv. partition of all test, they are equiv.
            equivPartitionWithElem = mergePartition2(equivPartitionWithElem, partition);
        }
      }
    }

    if (passAll) {
      if(equivPartitionWithElem.size() > 0){ //record passed equiv. partition
        correctProbabilityPartition[partitionIndex] = 1;
        partitionCorrect[partitionIndex] = 1;
        numIncorrectTest[partitionIndex] = 0;
        numCorrectTest[partitionIndex] = 0;
        currentPartition[partitionIndex++] = equivPartitionWithElem;
        numCorrectPartition ++;
        BOOST_LOG_TRIVIAL(info) << "partition size : " << equivPartitionWithElem.size();
      }
      vLocs.insert(elem.app->location);
      locToId[locToString2(elem.app->location)] = elem.app->id;
      return index;
    }
  }

  return index;
}

bool SearchEngine::executeCandidate(const Patch elem, unordered_set<PatchID> &newPartition,
                                     std::basic_string<char> &test, int index){
  BOOST_LOG_TRIVIAL(debug) << "executing candidate " << visualizePatchID(elem.id) 
                           << " with test " << test;

  bool passAll = true;
  
  std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

  TestStatus status = tester.execute(test);

  std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

  stat.executionCounter++;
  if (status != TestStatus::TIMEOUT) {
    stat.nonTimeoutCounter++;
    stat.nonTimeoutTestTime += std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
  } else {
    stat.timeoutCounter++;
  }

  switch (status) {
  case TestStatus::PASS:
    BOOST_LOG_TRIVIAL(debug) << "PASS";
    break;
  case TestStatus::FAIL:
    BOOST_LOG_TRIVIAL(debug) << "FAIL";
    break;
  case TestStatus::TIMEOUT:
    BOOST_LOG_TRIVIAL(debug) << "TIMEOUT";
  }

  passAll = (status == TestStatus::PASS);

  if (cfg.valueTEQ) {
    unordered_set<PatchID> partition = runtime.getPartition();
    if (partition.empty()) {
      //NOTE: it should contain at least the current element
      //BOOST_LOG_TRIVIAL(warning) << "partitioning failed for "
      //                           << visualizePatchID(elem.id)
      //                           << " with test " << test;
      //FIXME: should not directly return true (for testing)
      passAll = true;
    }

    if (cfg.patchPrioritization == PatchPrioritization::SEMANTIC_DIFF) {
      fs::path coverageFile = coverageDir / (test + "_" + std::to_string(index) + ".xml");
      std::shared_ptr<Coverage> curCoverage(new Coverage(extractAndSaveCoverage(coverageFile)));

      if (!coverageSet.count(test))
        coverageSet[test] = std::unordered_map<PatchID, std::shared_ptr<Coverage>>();

      coverageSet[test][elem.id] = curCoverage;
      for (auto &id : partition)
        coverageSet[test][id] = curCoverage;
    }

    newPartition.insert(partition.begin(), partition.end());

    if (passAll) {
      passing[test].insert(elem.id);
      passing[test].insert(partition.begin(), partition.end());

    } else {
      failing.insert(elem.id);
      failing.insert(partition.begin(), partition.end());
    }
  }
  return passAll;
}

const char* SearchEngine::getWorkingDir(){
  return cfg.dataDir.c_str();
}

const char* c_getWorkingDir(struct C_SearchEngine* engine){
  SearchEngine* a = SearchEngine_TO_CPP(engine);
  return a->getWorkingDir();
}

void SearchEngine::getPatchLoc(int &length, char *& array){
  //int *locs = (int*) malloc(vLocs.size() * sizeof(int));
  length = vLocs.size();
  //int index = 0;
  string out = "";
  for (unordered_set<Location>::iterator it = vLocs.begin() ; it != vLocs.end(); ++it){
    //locs[index++] = it->beginLine;
    out = out + locToString2(*it) + "#";
  }
  const char * temp = out.c_str();
  char *locs = (char*) malloc(strlen(temp) * sizeof(char) + 1);
  BOOST_LOG_TRIVIAL(info) << "locations : " << temp;
  strcpy(locs, temp);
  array = locs;
}

void c_getPatchLoc(struct C_SearchEngine* engine, int *length, char** array){
  SearchEngine* a = SearchEngine_TO_CPP(engine);
  a->getPatchLoc(*length, *array);
}

int c_fuzzPatch(struct C_SearchEngine* engine, char* test, char* reachedLocs, struct C_ExecutionStat* executionStat){
  SearchEngine* a = SearchEngine_TO_CPP(engine);
  std::string str_test(test);
  BOOST_LOG_TRIVIAL(debug) << "execute with test : " << str_test;
  int executionState = a->evaluatePatchWithNewTest(str_test, reachedLocs, executionStat);

  return executionState;
}

int SearchEngine::evaluatePatchWithNewTest(__string &test, char* reachedLocs, struct C_ExecutionStat* executionStat) {
  unsigned long index = 0;
  unordered_set<AppID> reachablePatches;
  unordered_set<PatchID> unReachablePatches;

  int tempPatchIndex = 1;
  unordered_map<PatchID, int> tempPatchPar;
  unordered_map<PatchID, int> tempPatchResult;

  //extract the reached locations by executing this test
  char *loc = strtok(reachedLocs, "#");
  while(loc != NULL){
    reachablePatches.insert(locToId[loc]);
    loc = strtok(NULL,"#");
  }

  bool notFindCrash = true;
  passing[test] = {};
  for (; index < searchSpace.size(); index++) {
    const Patch &elem = searchSpace[index];

    if (cfg.valueTEQ) {
      if (failing.count(elem.id) || unReachablePatches.count(elem.id))
        continue;
//      if (!reachablePatches.count(elem.app->id))
//        continue;
    }
    InEnvironment env({ { "F1X_APP", to_string(elem.app->id) },
                        { "F1X_ID_BASE", to_string(elem.id.base) },
                        { "F1X_ID_INT2", to_string(elem.id.int2) },
                        { "F1X_ID_BOOL2", to_string(elem.id.bool2) },
                        { "F1X_ID_COND3", to_string(elem.id.cond3) },
                        { "F1X_ID_PARAM", to_string(elem.id.param) } });

    bool passAll = true;

    if (cfg.valueTEQ) {
      if (passing[test].count(elem.id))
        continue;

      //FIXME: select only unexplored candidates
      runtime.setPartition((*partitionable)[elem.app->id]);
      runtime.setOutputResult(1);
    }
    unordered_set<PatchID> partition;
    passAll = executeCandidate(elem, partition, test, index);
    notFindCrash &= passAll;

    //unreachable patch location
    if(partition.size() == 0)
      unReachablePatches.insert((*partitionable)[elem.app->id].begin(), (*partitionable)[elem.app->id].end());
    else{
      if(!passAll)
        removeFailedPatches(partition);

      //whether the value of patch expression is same as original expression or not
      int outputResult = runtime.getOutputResult();

      //set partition info (map from patchId to partition id)
      for(PatchID patchId: partition){
        if(passAll)
          tempPatchPar[patchId] = tempPatchIndex;
        else
          tempPatchPar[patchId] = tempPatchIndex * -1;
        tempPatchResult[patchId] = outputResult;
      }
      tempPatchIndex++;
    }
  }
  passing.erase(test);

  int sizeofBrokenPartition = 0;
  int numBrokenParition = mergePartition(tempPatchPar, tempPatchResult, sizeofBrokenPartition);
  totalBrokenPartition += numBrokenParition;
  if(numBrokenParition > 0)
    numTestBreakPartition++;

  //output correct partition size
  if(!notFindCrash){
    numTestReducePlausiblePatches ++;
    for(auto it1 = currentPartition.begin(); it1!=currentPartition.end(); ++it1)
      // only output correct partition
      if(partitionCorrect[it1->first]){
        std::cout << "partition size: " << it1->second.size()
                                << " num of correct test: " << numCorrectTest[it1->first] 
                                << " num of incorrect test: " << numIncorrectTest[it1->first]
                                << " percentage: " << numCorrectTest[it1->first]/(double)(numCorrectTest[it1->first]+numIncorrectTest[it1->first])
                                << "\n";
      }
    savePathIdforRanking();
  }

  BOOST_LOG_TRIVIAL(debug) << "Number of broken partition is : " << numBrokenParition;
  BOOST_LOG_TRIVIAL(debug) << "Search Space size : " << searchSpace.size();
  BOOST_LOG_TRIVIAL(debug) << "failing size : " << failing.size();

  executionStat->numPlausiblePatch = searchSpace.size() - failing.size();
  executionStat->numPartition = numCorrectPartition;
  executionStat->numTestReducePlausiblePatches = numTestReducePlausiblePatches;
  executionStat->numTestBreakPartition = numTestBreakPartition;
  executionStat->numBrokenPartition = numBrokenParition;
  executionStat->totalNumBrokenPartition = totalBrokenPartition;
  executionStat->sizeofBrokenPartition = sizeofBrokenPartition;
  return 0;
}

int SearchEngine::mergePartition(unordered_map<PatchID, int> patchPar, unordered_map<PatchID, int> patchResult, int & sizeofBrokenPartition){
  int numberBrokenPartition = 0;

  int tempNumCorrectPartition = 0;
  unordered_map<unsigned long, double> tempCorrectProbabilityPartition;
  unordered_map<unsigned long, int> tempPartitionCorrect;
  unordered_map<unsigned long, unsigned long> tempNumCorrectTest;
  unordered_map<unsigned long, unsigned long> tempNumIncorrectTest;
  unordered_map<unsigned long, unordered_set<PatchID>> newPartition;
  unsigned long newPartitionIndex = 0;

  for (auto it1 = currentPartition.begin(); it1!=currentPartition.end(); ++it1){
    unordered_set<PatchID> par = it1->second;
    unordered_set<PatchID> par_temp = par;
    int tempPartitionIndex = newPartitionIndex;
    //int savedPatchSize = 0;
    bool findNewFailPar = false;
    while(par.size()>0){
      unordered_set<PatchID> newPar;
      auto it = par.begin();
      newPar.insert(*it);
      int tempPatchParIndex;
      int tempPatchResult = patchResult[*it];
      if(!patchPar.count(*it))
        tempPatchParIndex = 0;
      else
        tempPatchParIndex = patchPar[*it];
      par_temp.erase(*it);

      it++;
      for(; it!=par.end(); it++){
        if( (!patchPar.count(*it) && tempPatchParIndex == 0) 
            || tempPatchParIndex == patchPar[*it]){
          par_temp.erase(*it);
          newPar.insert(*it);
        }
      }
      //temp_numPartition ++;
      //if(tempPatchParIndex >= 0) {//failing partition will not be added to partition
      if(partitionCorrect[it1->first] && tempPatchParIndex >= 0){
        tempPartitionCorrect[newPartitionIndex] = 1;
        tempNumCorrectPartition ++;
      } else{
        tempPartitionCorrect[newPartitionIndex] = 0;
      }

      //if((tempPatchResult && tempPatchParIndex >= 0) || (!tempPatchResult && tempPatchParIndex < 0)){
      if(tempPatchResult){
        tempNumCorrectTest[newPartitionIndex] = numCorrectTest[it1->first] + 1;
        tempNumIncorrectTest[newPartitionIndex] = numIncorrectTest[it1->first];
      }
      else{
        tempNumCorrectTest[newPartitionIndex] = numCorrectTest[it1->first];
        tempNumIncorrectTest[newPartitionIndex] = numIncorrectTest[it1->first] + 1;
      }

      if(partitionCorrect[it1->first] && tempPatchParIndex < 0)
        findNewFailPar = true;

      newPartition[newPartitionIndex++] = newPar;
      //savedPatchSize += newPar.size();
      //}
      par = par_temp;
    }

    int numNewPar = newPartitionIndex - tempPartitionIndex;
    if(numNewPar > 0){
      for(int i=tempPartitionIndex; i<newPartitionIndex; i++){
        tempCorrectProbabilityPartition[i] = 1/(double)numNewPar * correctProbabilityPartition[it1->first];
      }
      if(numNewPar > 1 || findNewFailPar){ //successfully break one partition into several plausible patch partition
        sizeofBrokenPartition += it1->second.size();
        saveExpectedFilteredParitionSize(correctProbabilityPartition[it1->first]*(numNewPar-1)/(double)numNewPar, it1->second);
      }
    }
  }

  numberBrokenPartition = newPartitionIndex - partitionIndex;

  partitionIndex = newPartitionIndex;
  currentPartition = newPartition;
  correctProbabilityPartition = tempCorrectProbabilityPartition;
  numCorrectPartition = tempNumCorrectPartition;
  partitionCorrect = tempPartitionCorrect;
  numCorrectTest = tempNumCorrectTest;
  numIncorrectTest = tempNumIncorrectTest;

  return numberBrokenPartition;
}

unordered_set<PatchID> SearchEngine::mergePartition2(unordered_set<PatchID> partition1, unordered_set<PatchID> partition2){
  unordered_set<PatchID> mergedPartition;
  for(auto i = partition1.begin(); i != partition1.end(); i++){
    if(partition2.find(*i) != partition2.end()) 
      mergedPartition.insert(*i);
  }
  return mergedPartition;
}

void SearchEngine::removeFailedPatches(unordered_set<PatchID> partition){
  for(PatchID patchId: partition){
    //if(!failing.count(patchId)){
      fs::path patchFile = patchOutput / (visualizePatchID(patchId) + ".patch");
      string cmd = "rm -rf " + patchFile.string();
      BOOST_LOG_TRIVIAL(debug) << "removing Failed patches --- cmd: " << cmd;
      std::system(cmd.c_str());
    //}
  }
}

void SearchEngine::saveExpectedFilteredParitionSize(double factor, unordered_set<PatchID> partition){

  BOOST_LOG_TRIVIAL(debug) << "save PartitionSize: " << partition.size() << "factor: " << factor;
  fs::path patchFile = patchOutput / "expectedFilteredParitionSize2";
  factorOfPartition[savedPartitionIndex] = factor;
  brokenPartition[savedPartitionIndex++] = partition;
  for (auto it=brokenPartition.begin(); it!=brokenPartition.end(); ++it){
    int index = 0;
    unordered_set<PatchID> tempPartition = it->second;
    for(PatchID patchId: tempPartition){
      if(!failing.count(patchId)) //only count plausible patches
        index++;
    }
    int expectedFilteredParitionSize = (int)(index*factorOfPartition[it->first]);
    if(expectedFilteredParitionSize>0){
      BOOST_LOG_TRIVIAL(debug) << "savedPatchSize: " << index << "factor: " << factorOfPartition[it->first];
      string cmd = "echo " + to_string(expectedFilteredParitionSize) + " >> " + patchFile.string();
      std::system(cmd.c_str());
    }
  }

  fs::path patchFile2 = patchOutput / "expectedFilteredParitionSize";
  string cmd = "mv " + patchFile.string() + " " + patchFile2.string();
  std::system(cmd.c_str());
}

void SearchEngine::savePathIdforRanking(){
  BOOST_LOG_TRIVIAL(debug) << "save Partition ID: ";
  fs::path patchIDFile2 = patchOutput / "partitionID2";

  for(auto it1 = currentPartition.begin(); it1!=currentPartition.end(); ++it1){
    if(partitionCorrect[it1->first]){
      double percentage = numCorrectTest[it1->first]/(double)(numCorrectTest[it1->first]+numIncorrectTest[it1->first]);
      string cmd = "echo \"================================================================================\" >> " + patchIDFile2.string();
      std::system(cmd.c_str());
      cmd = "echo " + to_string(percentage) + " >> " + patchIDFile2.string();
      std::system(cmd.c_str());
      for(auto output_it=it1->second.begin(); output_it!=it1->second.end(); output_it++){
        cmd = "echo " + visualizePatchID(*output_it) + " >> " + patchIDFile2.string();
        std::system(cmd.c_str());
      }
    }
  }
  fs::path patchIDFile = patchOutput / "partitionId";
  string cmd = "mv " + patchIDFile2.string() + " " + patchIDFile.string();
  std::system(cmd.c_str());
}
