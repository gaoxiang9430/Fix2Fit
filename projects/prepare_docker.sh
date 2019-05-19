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
            BUGGY_FILE=`echo -n $LINE | cut --delimiter=, --fields=4`
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
            BINARY_NAME=`echo -n $LINE | cut --delimiter=, --fields=5`
            break;
        fi
    done < $SCRIPT_DIR/all_issue_ids_${SUBJECT}.txt
    echo $BINARY_NAME
}


for d in ${SUBJECT}_*; do
  pushd $d > /dev/null
  directory_name=`pwd | rev | cut --delimiter=/ -f 1 | rev`
  buggy_number=`echo $directory_name | cut -d'_' -f 2`
  echo $buggy_number
  buggy_file=$(get_buggy_file $buggy_number)
  rm -rf driver*

  cp -r ../${SUBJECT}/* .
  cp -r ../scripts .
  cp ../subject_build.sh build.sh

  sed "s|BUGGY_FILE=|BUGGY_FILE=${buggy_file}|" build.sh > build1.sh
  sed "s|SUBJECT=|SUBJECT=${SUBJECT}|" build1.sh > build2.sh

  binary_name=$(get_binary_name $buggy_number)
  sed "s|BINARY=|BINARY=${binary_name}|" build2.sh > build3.sh

  rm -rf build.sh build1.sh build2.sh
  mv build3.sh build.sh
  chmod u+x build.sh
  popd > /dev/null
done
