#!/usr/bin/env bash
#
# Usage: run.sh [project_name] [bug_number]
#

PROJECT_NAME=$1

BUG_NUMBER=$2

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

cd $SCRIPT_DIR

if [ x$PROJECT_NAME != x ] ; then
	PROJECT_DIR_LIST=`ls "projects/$PROJECT_NAME"_*`
	if [ "$PROJECT_DIR_LIST" = "" ] ; then
		echo Project $PROJECT_NAME not found.
		exit 1
	else
		if [ x$BUG_NUMBER != x ] ; then
			PROJECT_DIR=`ls -d "projects/$PROJECT_NAME"_$BUG_NUMBER`
			if [ x$PROJECT_DIR = x ] ; then
				echo Project $PROJECT_NAME with bug number $BUG_NUMBER not found.
				exit 1
			else
				sudo python infra/helper.py build_image $PROJECT_NAME $BUG_NUMBER
				sudo python infra/helper.py build_fuzzers --sanitizer address $PROJECT_NAME $BUG_NUMBER
			fi
		else
			for BUG_NUMBER in `ls -d "projects/$PROJECT_NAME"_* | sed s/[^[:digit:]]/\ /g`
			do
				sudo python infra/helper.py build_image $PROJECT_NAME $BUG_NUMBER
				sudo python infra/helper.py build_fuzzers --sanitizer address $PROJECT_NAME $BUG_NUMBER
				# For running the fuzzer
				# sudo python infra/helper.py run_fuzzer "$PROJECT_NAME"_$BUG_NUMBER "$PROJECT_NAME"_fuzzer
			done
		fi
	fi
else
	for PROJECT_NAME in ffmpeg libarchive
	do
		for BUG_NUMBER in `ls -d "projects/$PROJECT_NAME"_* | sed s/[^[:digit:]]/\ /g`
		do
			sudo python infra/helper.py build_image $PROJECT_NAME $BUG_NUMBER
			sudo python infra/helper.py build_fuzzers --sanitizer address $PROJECT_NAME $BUG_NUMBER
			# For running the fuzzer
			# sudo python infra/helper.py run_fuzzer "$PROJECT_NAME"_$BUG_NUMBER "$PROJECT_NAME"_fuzzer
		done
	done
fi

