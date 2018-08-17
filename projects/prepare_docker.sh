#!/bin/bash

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

SUBJECT=$1
get_buggy_file(){
    local LINE
    local BUG_NUMBER
    local FILED_BUG_NUMBER
    local BUGGY_FILE

    BUG_NUMBER=$1
    while read LINE ;  do
        FILED_BUG_NUMBER=`echo -n $LINE | cut --delimiter=, --fields=1`
        if [ x$BUG_NUMBER = x$FILED_BUG_NUMBER ] ; then
            if [ $SUBJECT = "ffmpeg" ]; then
              BUGGY_FILE=`echo -n $LINE | cut --delimiter=, --fields=5`
            else
              BUGGY_FILE=`echo -n $LINE | cut --delimiter=, --fields=4`
            fi
            break;
        fi
    done < $SCRIPT_DIR/all_issue_ids_${SUBJECT}.txt
    echo $BUGGY_FILE
}

get_binary_name(){
    local LINE
    local BUG_NUMBER
    local FILED_BUG_NUMBER
    local BINARY_NAME

    BUG_NUMBER=$1
    while read LINE ;  do
        FILED_BUG_NUMBER=`echo -n $LINE | cut --delimiter=, --fields=1`
        if [ x$BUG_NUMBER = x$FILED_BUG_NUMBER ] ; then
            if [ $SUBJECT = "ffmpeg" ]; then
              BINARY_NAME=`echo -n $LINE | cut --delimiter=, --fields=6`
            fi
            break;
        fi
    done < $SCRIPT_DIR/all_issue_ids_${SUBJECT}.txt
    echo $BINARY_NAME
}


for d in ${SUBJECT}_*; do
  #d=proj4_1793
  pushd $d > /dev/null
  directory_name=`pwd | rev | cut --delimiter=/ -f 1 | rev`
  buggy_number=`echo $directory_name | cut -d'_' -f 2`
  echo $buggy_number
  buggy_file=$(get_buggy_file $buggy_number)
  #echo $buggy_file
  rm -rf driver*
  if [ $SUBJECT = "ffmpeg" ]; then
    mv ${SUBJECT}_* ${SUBJECT}_testcase 2> /dev/null
  else
    mv ${SUBJECT}_*_testcase ${SUBJECT}_testcase 2> /dev/null
  fi
  cp -r ../${SUBJECT}/* .
  cp -r ../scripts .
  sed "s|BUGGY_FILE|BUGGY_FILE=${buggy_file}|" build.sh > build1.sh
  sed "s|SUBJECT_TAG|${directory_name}|" build1.sh > build2.sh
  if [ $SUBJECT = "ffmpeg" ]; then
    binary_name=$(get_binary_name $buggy_number)
    sed "s|BINARY=|BINARY=${binary_name}|" build2.sh > build3.sh
    mv build3.sh build2.sh
  fi
  rm -rf build.sh
  rm -rf build1.sh
  mv build2.sh build.sh
  chmod u+x build.sh
  popd > /dev/null
done
