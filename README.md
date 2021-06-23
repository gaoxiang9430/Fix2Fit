# Fix2Fit: Crash-avoiding Program Repair (Alpha version)

Fix2Fit is an integrated approach for detecting and discarding crashing patches. Fix2Fit fuses test and patch generation into a single process, in which patches are generated with the objective of passing existing tests, and new tests are generated with the objective of filtering out over-fitted patches by distinguishing candidate patches in terms of behavior. The oracle to discard patch candidates is determined by crash-freedom including traditional crash and security vulnerability detected by Sanitizer.

This software is built on top of [OSS-Fuzz](https://github.com/google/oss-fuzz) (please refer `README_OSSFUZZ.md` for the renamed original OSS-Fuzz `README.md`). As with OSS-Fuzz, it mainly fixes the bugs/vulnerabilities detected by fuzzing techniques. Fix2Fit takes inputs the buggy program and a failing test case, generates a set of plausible patches, which fix the bug and does not introduce crash.

### Requirement
1. Python 2.7
2. Docker

### Installation
1. Get Fix2Fit source:
```
$ git clone https://github.com/gaoxiang9430/Fix2Fit.git
$ git submodule update --init --recursive
```

2. Build OSS-fuzz base images
```
$ cd Fix2Fit
$ ./infra/base-images/all.sh
```

3. Build Fix2Fit image
```
$ docker build -t gaoxiang9430/fix2fit .
```
Alternatively, the pre-compiled docker image can be found in the docker hub
```
$ docker pull gaoxiang9430/fix2fit:v0.1
```

### Runing
Create a container and you can find Fix2Fit.py at the /src/script.
```
docker run -it gaoxiang9430/fix2fit:v0.1 /bin/bash
```
To fix a detected bug, Fix2Fit takes as input the buggy program (path), a set of test cases including at least one failing test, a driver to execute the tests and buggy file. The detailed usage is as follows.

```
usage: Fix2Fit.py [-h] -s SOURCE_PATH -t TESTS [TESTS ...] -d DRIVER -f FILE
                  -b BUILD -c CONFIG -T TIMEOUT -B BINARY [-v] [-C]

optional arguments:
  -h, --help            show this help message and exit
  -s SOURCE_PATH, --source-path SOURCE_PATH
                        the path of target project
  -t TESTS [TESTS ...], --tests TESTS [TESTS ...]
                        the list of unique test identifiers (e.g. ID1 ID2 ...)
  -d DRIVER, --driver DRIVER
                        the path to the test driver. The test driver is
                        executed from the project root directory
  -f FILE, --file FILE  the suspicious file that many contain the bug. Fix2Fit
                        allows to restrict the search space to certain parts
                        of the source code files. For the arguments --files
                        main.c:20 lib.c:5-45, the candidate locations will be
                        restricted to the line 20 of main.c and from the line
                        5 to the line 45 (inclusive) of lib.c
  -b BUILD, --build BUILD
                        the build command. The build command is executed from
                        the project root directory
  -c CONFIG, --config CONFIG
                        the config command. The config command is executed
                        from the project root directory
  -T TIMEOUT, --timeout TIMEOUT
                        the fuzzing execution timeout
  -B BINARY, --binary BINARY
                        The path to the binary program from the project root
                        directory
  -v, --verbose         show debug information
  -C, --crash           crash exploration mode (the peruvian rabbit thing)
```
If everything works well, it will produce a set of patches at the SOURCE_PATH/patches directory.

### Runing Example
We include a demo in the docker image.
```
$ cd /benchmark/proj4
$ python3 /src/scripts/Fix2Fit.py -s /benchmark/proj4/ -t /benchmark/proj4/input/testcase -d /out/standard_fuzzer -f src/pj_init.c:368-394 -b ./build.sh -c ./config.sh -B /out/standard_fuzzer -T 1h -C
```

### Publication
**Crash-avoiding Program Repair** Xiang Gao, Sergey Mechtaev, Abhik Roychoudhury [[pdf]](https://www.comp.nus.edu.sg/~gaoxiang/Fix2Fit.pdf)<br>
*-ACM SIGSOFT International Symposium on Software Testing and Analysis (ISSTA) 2019.*

### Contributors
Principal investigator:
- Abhik Roychoudhury

Developers:
- Xiang Gao
- Sergey Mechteav

Contributors:
- Edwin Lesmana
- Andrew Santosa


