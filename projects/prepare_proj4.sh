#!/bin/bash

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

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
    done < $SCRIPT_DIR/all_issue_ids_proj4.txt
    echo $BUGGY_FILE
}

for d in proj4_*; do
  #d=proj4_1793
  pushd $d > /dev/null
  directory_name=`pwd | rev | cut --delimiter=/ -f 1 | rev`
  buggy_number=`echo $directory_name | cut -d'_' -f 2`
  echo $buggy_number
  buggy_file=$(get_buggy_file $buggy_number)
  echo $buggy_file
  rm -rf driver*
  mv proj4_*_testcase proj4_testcase
  cp ../proj4/* .
  cp -r ../scripts .
  rm scripts/build.sh
  sed "s|BUGGY_FILE|BUGGY_FILE=${buggy_file}|" build.sh > build1.sh
  rm -rf build.sh
  mv build1.sh build.sh
  chmod u+x build.sh
  popd > /dev/null
done



