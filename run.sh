#!/usr/bin/env bash

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

cd $SCRIPT_DIR

for PROJECT_NAME in ffmpeg libarchive
do
	for PROJECT_NUM in `ls -d "projects/$PROJECT_NAME"_* | sed s/[^[:digit:]]/\ /g`
	do
		sudo python infra/helper.py build_image $PROJECT_NAME $PROJECT_NUM
		sudo python infra/helper.py build_fuzzers --sanitizer address $PROJECT_NAME $PROJECT_NUM
		# For running the fuzzer
		# sudo python infra/helper.py run_fuzzer libarchive_$PROJECT_NUM libarchive_fuzzer
	done
done
