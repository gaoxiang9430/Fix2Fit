#!/usr/bin/env bash

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

cd $SCRIPT_DIR

export PROJECT_NAME=libarchive
for PROJECT_NUM in `ls -d projects/libarchive_* | sed s/[^[:digit:]]/\ /g`
do
	export PROJECT_NAME=libarchive
	sudo python infra/helper.py build_image $PROJECT_NAME $PROJECT_NUM
	sudo python infra/helper.py build_fuzzers --sanitizer address $PROJECT_NAME $PROJECT_NUM
	# For running the fuzzer
	# sudo python infra/helper.py run_fuzzer libarchive_$PROJECT_NUM libarchive_fuzzer	
done
