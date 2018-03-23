#!/usr/bin/env bash
#
# Usage: run.sh [project_name] [bug_number]
#

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

cd $SCRIPT_DIR

# Function definitions

make_ffmpeg_project() {
    local LINE
    local BUG_NUMBER
    local FILED_BUG_NUMBER
    local FFMPEG_HASH
    local X264_HASH
    local ENTRY_DIR

    INITIAL_DIR=`pwd`
    
    BUG_NUMBER=$1
    FFMPEG_HASH=
    X264_HASH=
    while read LINE ;  do
	FILED_BUG_NUMBER=`echo -n $LINE | cut --delimiter=, --fields=1`
	if [ x$BUG_NUMBER = x$FILED_BUG_NUMBER ] ; then
	    FFMPEG_HASH=`echo -n $LINE | cut --delimiter=, --fields=2`
	    X264_HASH=`echo -n $LINE | cut --delimiter=, --fields=3`
	    break;
	fi
    done < $SCRIPT_DIR/all_issue_ids_ffmpeg.txt

    if [ x$FFMPEG_HASH != x ] ; then
	cd $SCRIPT_DIR/projects/ffmpeg_$BUG_NUMBER
	git clone https://github.com/FFmpeg/FFmpeg.git "ffmpeg_$BUG_NUMBER"_codes
	cd "ffmpeg_$BUG_NUMBER"_codes
	git reset --hard $FFMPEG_HASH
	cd $SCRIPT_DIR/projects/ffmpeg_$BUG_NUMBER
	git clone https://github.com/mirror/x264.git x264_prev
	cd x264_prev
	git reset --hard $X264_HASH
    fi

    # Restore the supposedly original state
    cd $INITIAL_DIR
}

build_project() {
    local PROJECT_NAME
    local BUG_NUMBER
    local INITIAL_DIR
    
    PROJECT_NAME=$1
    BUG_NUMBER=$2
    INITIAL_DIR=`pwd`
    
    if [ x$PROJECT_NAME = xffmpeg ] ; then
	make_ffmpeg_project $BUG_NUMBER
    fi
    sudo python $SCRIPT_DIR/infra/helper.py build_image $PROJECT_NAME $BUG_NUMBER
    sudo python $SCRIPT_DIR/infra/helper.py build_fuzzers --sanitizer address $PROJECT_NAME $BUG_NUMBER
    # For running the fuzzer
    # sudo python infra/helper.py run_fuzzer "$PROJECT_NAME"_$BUG_NUMBER "$PROJECT_NAME"_fuzzer

    cd $INITIAL_DIR
}

# The main procedure

PROJECT_NAME=$1

BUG_NUMBER=$2

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
		build_project $PROJECT_NAME $BUG_NUMBER
	    fi
	else
	    for BUG_NUMBER in `ls -d "projects/$PROJECT_NAME"_* | sed s/[^[:digit:]]/\ /g`
	    do
		build_project $PROJECT_NAME $BUG_NUMBER
	    done
	fi
    fi
else
    for PROJECT_NAME in ffmpeg libarchive
    do
	for BUG_NUMBER in `ls -d "projects/$PROJECT_NAME"_* | sed s/[^[:digit:]]/\ /g`
	do
	    build_project $PROJECT_NAME $BUG_NUMBER
	done
    done
fi

