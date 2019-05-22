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
#include <sstream>
#include <cstdlib>
#include <sys/wait.h>

#include <boost/filesystem/fstream.hpp>

#include <boost/log/trivial.hpp>

#include "Util.h"
#include "Profiler.h"
#include "Global.h"

namespace fs = boost::filesystem;
using std::string;
using std::vector;
using std::set;
using std::unordered_map;


string locToString(const Location &loc) {
  std::ostringstream out;
  out << loc.fileId << " "
      << loc.beginLine << " "
      << loc.beginColumn << " "
      << loc.endLine << " "
      << loc.endColumn;
  return out.str();
}


boost::filesystem::path Profiler::getHeader() {
  return fs::path(cfg.dataDir) / PROFILE_HEADER_FILE_NAME;
}

boost::filesystem::path Profiler::getSource() {
  return fs::path(cfg.dataDir) / PROFILE_SOURCE_FILE_NAME;
}

unordered_map<Location, vector<unsigned>> Profiler::getRelatedTestIndexes() {
  return relatedTestIndexes;
}

bool Profiler::compile() {
  BOOST_LOG_TRIVIAL(debug) << "compiling profile runtime";
  {
    fs::ofstream source(getSource());
    source << "#include <fstream>" << "\n"
           << "#include <unordered_set>" << "\n"
           << "#include \"" << PROFILE_HEADER_FILE_NAME << "\"" << "\n"
           << "struct __f1x_loc {" << "\n"
           << "  unsigned long fileId;" << "\n"
           << "  unsigned long beginLine;" << "\n"
           << "  unsigned long beginColumn;" << "\n"
           << "  unsigned long endLine;" << "\n"
           << "  unsigned long endColumn;" << "\n"
           << "  bool operator==(const __f1x_loc &other) const {" << "\n"
           << "return (fileId == other.fileId" << "\n"
           << "&& beginLine == other.beginLine" << "\n"
           << "&& beginColumn == other.beginColumn" << "\n"
           << "&& endLine == other.endLine" << "\n"
           << "&& endColumn == other.endColumn);" << "\n"
           << "}" << "\n"
           << "};" << "\n"
           << "template <class T>" << "\n"
           << "inline void hash_combine(std::size_t & s, const T & v)" << "\n"
           << "{" << "\n"
           << "std::hash<T> h;" << "\n"
           << "s ^= h(v) + 0x9e3779b9 + (s << 6) + (s >> 2);" << "\n"
           << "}" << "\n"
           << "namespace std {" << "\n"
           << "template<>" << "\n"
           << "struct hash<__f1x_loc> {" << "\n"
           << "inline size_t operator()(const __f1x_loc& id) const {" << "\n"
           << "size_t value = 0;" << "\n"
           << "hash_combine(value, id.fileId);" << "\n"
           << "hash_combine(value, id.beginLine);" << "\n"
           << "hash_combine(value, id.beginColumn);" << "\n"
           << "hash_combine(value, id.endLine);" << "\n"
           << "hash_combine(value, id.endColumn);" << "\n"
           << "return value;" << "\n"
           << "}" << "\n"
           << "};" << "\n"
           << "}" << "\n"
           << "std::unordered_set<__f1x_loc> __f1x_locs;" << "\n";

    source << "void __f1x_trace(unsigned long fid, unsigned long bl, unsigned long bc, unsigned long el, unsigned long ec) {"  << "\n"
           << "__f1x_loc loc = {fid, bl, bc, el, ec};" << "\n"
           << "if (! __f1x_locs.count(loc)) {" << "\n"
           << "std::ofstream ofs(\""<< (fs::path(cfg.dataDir) / TRACE_FILE_NAME).string() << "\", std::ofstream::out | std::ofstream::app);" << "\n"
           << "ofs << fid << \" \" << bl << \" \" <<  bc << \" \" << el << \" \" << ec << \"\\n\";" << "\n"
           << "__f1x_locs.insert(loc);" << "\n"
           << "}" << "\n"
           << "}" << "\n";

    fs::ofstream header(getHeader());
    header << "#ifdef __cplusplus" << "\n"
           << "extern \"C\" {" << "\n"
           << "#endif" << "\n";
    header << "void __f1x_trace(unsigned long fid, unsigned long bl, unsigned long bc, unsigned long el, unsigned long ec);\n";
    header << "#ifdef __cplusplus" << "\n"
           << "}" << "\n"
           << "#endif" << "\n";
  }
  FromDirectory dir(fs::path(cfg.dataDir));
  std::stringstream cmd;
  cmd << RUNTIME_COMPILER
      << " " << RUNTIME_OPTIMIZATION
      << " -fPIC"
      << " " << PROFILE_SOURCE_FILE_NAME
      << " -shared"
      << " -std=c++11" // this is for initializers
      << " -o libf1xrt.so";
  if (cfg.verbose) {
    cmd << " >&2";
  } else {
    cmd << " >/dev/null 2>&1";
  }
  BOOST_LOG_TRIVIAL(debug) << "cmd: " << cmd.str();
  unsigned long status = std::system(cmd.str().c_str());
  if(!cfg.binaryPath.empty()){
    std::stringstream cmdCopyLib;
    cmdCopyLib << "cp"
               << " " << "libf1xrt.so"
               << " " << cfg.binaryPath;
    std::system(cmdCopyLib.str().c_str());
  }

  return WEXITSTATUS(status) == 0;
}

void Profiler::clearTrace() {
  fs::path traceFile = fs::path(cfg.dataDir) / TRACE_FILE_NAME;
  fs::ofstream out;
  out.open(traceFile, std::ofstream::out | std::ofstream::trunc);
  out.close();
}

void Profiler::mergeTrace(unsigned testIndex, bool isPassing) {
  fs::path traceFile = fs::path(cfg.dataDir) / TRACE_FILE_NAME;
  fs::ifstream infile(traceFile);
  set<string> covered;
  if(infile) {
    string line;
    while (std::getline(infile, line)) {
      Location loc;
      std::istringstream iss(line);
      iss >> loc.fileId >> loc.beginLine >> loc.beginColumn >> loc.endLine >> loc.endColumn;
      if(! relatedTestIndexes.count(loc)) {
        relatedTestIndexes[loc] = vector<unsigned>();
      }
      vector<unsigned> current = relatedTestIndexes[loc];
      if (std::find(current.begin(), current.end(), testIndex) == current.end()) {
        //NOTE: put failing in the beginning, passing in the end
        if (isPassing)
          relatedTestIndexes[loc].push_back(testIndex);
        else
          relatedTestIndexes[loc].insert(relatedTestIndexes[loc].begin(), testIndex);
      }
      covered.insert(locToString(loc));
    }
  }
  if (covered.empty()) {
    BOOST_LOG_TRIVIAL(debug) << "test no. " << testIndex << " produces empty trace";
  }

  if (!isPassing) {
    if (interestingLocations.empty()) {
      interestingLocations.insert(covered.begin(), covered.end());
    } else {
      //NOTE: here is intentionally intersection:
      set<string> newInteresting;
      for (auto &loc : interestingLocations) {
        if (covered.count(loc)) {
          newInteresting.insert(loc);
        }
      }
      auto it = relatedTestIndexes.begin();
      while (it != relatedTestIndexes.end()) {
        if (! covered.count(locToString(it->first))) {
          it = relatedTestIndexes.erase(it);
        } else {
          it++;
        }
      }
      std::swap(interestingLocations, newInteresting);
    }
  }
}

fs::path Profiler::getProfile() {
  fs::path profileFile = fs::path(cfg.dataDir)/ PROFILE_FILE_NAME;
  fs::ofstream outfile(profileFile, std::ios::app);

  //NOTE: removing uninteresting test indexes
  unordered_map<Location, vector<unsigned>>::iterator it = relatedTestIndexes.begin();
  while(it != relatedTestIndexes.end()) {
    if(interestingLocations.find(locToString(it->first)) == interestingLocations.end()) {
      it = relatedTestIndexes.erase(it);
    } else {
      it++;
    }
  }

  for (auto &loc : interestingLocations) {
    outfile << loc << "\n";
  }

  return profileFile;
}
