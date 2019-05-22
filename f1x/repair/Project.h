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

#include <boost/filesystem.hpp>
#include "Util.h"


// (!fromLine && !toLine) means no restriction
struct ProjectFile {
  boost::filesystem::path relpath;
  unsigned fromLine;
  unsigned toLine;
};


class Project {
 public:
  /* project saves original files on creation
     the convention is to restore original after every modification */
  Project(const std::vector<ProjectFile> &files,
          const std::string &buildCmd);

  /* restores original files on destruction, just in case of exception */
  ~Project();

  std::pair<bool, bool> initialBuild();
  bool build();
  bool buildWithRuntime(const boost::filesystem::path &header);
  void saveOriginalFiles();
  void saveInstrumentedFiles();
  void saveProfileInstumentedFiles();
  void restoreOriginalFiles();
  void restoreInstrumentedFiles();
  void deleteCoverageFiles();
  void computeDiff(const ProjectFile &file,
                   const boost::filesystem::path &outputFile);
  bool instrumentFile(const ProjectFile &file,
                      const boost::filesystem::path &outputFile,
                      const boost::filesystem::path *profile = nullptr);
  bool applyPatch(const Patch &patch);
  std::vector<ProjectFile> getFiles() const;
  void setFiles(const std::vector<ProjectFile> &files);
  std::vector<boost::filesystem::path> filesFromCompilationDB();
  bool buildInEnvironment(const std::map<std::string, std::string> &env, const std::string &baseCmd);

 private:
  std::vector<ProjectFile> files;
  std::string buildCmd;
  boost::filesystem::path patchTemplateDir;

  void saveFilesWithPrefix(const std::string &prefix);
  void restoreFilesWithPrefix(const std::string &prefix);
  //bool buildInEnvironment(const std::map<std::string, std::string> &env, const std::string &baseCmd);
  unsigned getFileId(const ProjectFile &file);
};


class TestingFramework {
 public:
  TestingFramework(const Project &project,
                   const boost::filesystem::path &driver,
                   const unsigned long testTimeout);
  
  TestStatus execute(const std::string &testId);
  bool driverIsOK();

 private:
  Project project;
  boost::filesystem::path driver;
  unsigned testTimeout;
};
