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

### Runing
To fix a bug detected by OSS-Fuzz, there are several steps:

1. Prepare work dir
<pre>
<b>$ cd projects</b>
<b>$ cp -r [SUBJECT] [SUBJECT]_[BUG_ID]</b>
<i>SUBJECT</i> and <i>BUG_ID</i> are the the <i>project name</i> and <i>bug id</i> in <a href=https://bugs.chromium.org/p/oss-fuzz/issues/list>OSS-Fuzz Issue tracker</a>.
</pre>

2. Create config and build script
<pre>
<b>$ cd [SUBJECT]_[BUG_ID]</b>
<b>create project_config.sh and project_build.sh</b>
<i>project_config.sh</i> and <i>project_build.sh</i> give the scripts to config and compile [SUBJECT],respectively. Those files can be create by spliting [SUBJECT]/<i>build.sh</i>
</pre>

3. Configuration for bug
<pre>
<b>put the reproducible test case in [SUBJECT]_[BUG_ID] directory and name it as <i>testcase</i></b>
<b>create a configuration file by specifying BINARY_NAME, BUGGY_COMMIT_ID, etc. Please refer to the <a href=./scripts/template.config>template</a></b>.
</pre>

4. Run
<pre>
<b>$ ./scripts/run.sh [CONFIGURATION_FILE] [CPU_ID] </b>
<i>CPU_ID</i> is the CPU id on which you execute this process [default 0]
</pre>

### Runing Example
<pre>
<b>$ ./scripts/run.sh ./project/proj4_1793/proj4_1793.config 0 </b>
</pre>

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

