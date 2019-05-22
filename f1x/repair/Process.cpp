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

#include "Process.h"

#include <cassert>
#include <unistd.h> // dup2
#include <sys/time.h> // itimerval
#include <sys/types.h> // pid_t
#include <sys/wait.h> // waitpid
#include <sys/stat.h>
#include <fcntl.h> // open
#include <signal.h> // sigaction


#define MAX_ARGS 100


static pid_t child_pid;
static bool child_timed_out = false;
static bool initialized = false;


static void handle_timeout(int sig) {
  child_timed_out = true;
  kill(child_pid, SIGKILL);
}


static void setup_signal_handlers() {
  struct sigaction sa;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sa.sa_handler = handle_timeout;
  sigaction(SIGALRM, &sa, NULL);
}


unsigned run_executable(std::string file, std::vector<std::string> args, unsigned timeout, bool mute_stdout, bool mute_stderr, bool always_kill) {
  if (! initialized) {
    initialized = true;
    setup_signal_handlers();
  }
    
  struct itimerval it;
  int status = 0;
  int dev_null_fd = open("/dev/null", O_RDWR);
  child_timed_out = false;

  child_pid = fork();

  if (!child_pid) { // child process
    dup2(dev_null_fd, STDOUT_FILENO);
    dup2(dev_null_fd, STDERR_FILENO);
    close(dev_null_fd);

    assert(args.size() < MAX_ARGS);
    std::vector<char*> cstrings;
    for(size_t i = 1; i < args.size(); ++i)
      cstrings.push_back(const_cast<char*>(args[i].c_str()));
    cstrings.push_back(NULL);
    auto argv = &cstrings[0];

    execvp(file.c_str(), argv);

    exit(0);
  }

  if (timeout) {
    it.it_value.tv_sec = (timeout / 1000);
    it.it_value.tv_usec = (timeout % 1000) * 1000;
    setitimer(ITIMER_REAL, &it, NULL);
  }

  waitpid(child_pid, &status, 0);

  return 0;
}


unsigned run_shell(std::string cmd, unsigned timeout, bool mute_stdout, bool mute_stderr, bool always_kill) {
  return 0;
}
