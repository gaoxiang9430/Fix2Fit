#!/usr/bin/env bash

Usage="Usage: run.sh config_file [core_id]

              config_file: (refer config template scripts/template.config)"

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
pushd "$SCRIPT_DIR/.." > /dev/null || exit 1

config_file=$1
ASSIGNED_CORE=$2

if [ ! -f "$config_file" ]; then
    echo "FAIL: config file does not exist"
    exit 1
fi

PROJECT_NAME=$(cat "$config_file" | python -c "import sys, json; print json.load(sys.stdin)['project']")
BUG_NUMBER=$(cat "$config_file" | python -c "import sys, json; print json.load(sys.stdin)['id']")
PROJ_HASH=$(cat "$config_file" | python -c "import sys, json; print json.load(sys.stdin)['bug_commit_id']")
SANITIZER_TYPE=$(cat "$config_file" | python -c "import sys, json; print json.load(sys.stdin)['sanitizer']")
GITHUB_LINK=$(cat "$config_file" | python -c "import sys, json; print json.load(sys.stdin)['github_link']")
BUGGY_FILE=$(cat "$config_file" | python -c "import sys, json; print json.load(sys.stdin)['buggy_file']")
BINARY=$(cat "$config_file" | python -c "import sys, json; print json.load(sys.stdin)['binary']")

echo "READ bug information"
echo "PROJECT_NAME: $PROJECT_NAME"
echo "BUG_NUMBER: $BUG_NUMBER"
echo "BUGGU_COMMIT_ID: $PROJ_HASH"
echo "SANITIZER_TYPE: $SANITIZER_TYPE"
echo "GITHUB_LINK: $GITHUB_LINK"

if [ "x$PROJECT_NAME" != "x" ] ; then
    if [ "x$BUG_NUMBER" != "x" ] ; then
        PROJECT_DIR=$(ls -d projects/"$PROJECT_NAME"_"$BUG_NUMBER")
        if [ "x$PROJECT_DIR" = "x" ] ; then
            echo "Project $PROJECT_NAME with bug number $BUG_NUMBER not found."
            exit 1
        fi
	  else
	      echo "$Usage"
	      exit 1
    fi
else
  	echo "$Usage"
	  exit 1
fi

# clone project and set it to correct version
if [ "x$GITHUB_LINK" != "x" ] ; then
    pushd "$SCRIPT_DIR"/../projects/"${PROJECT_NAME}"_"${BUG_NUMBER}" > /dev/null || exit
    git clone "${GITHUB_LINK}" "$PROJECT_NAME"
    echo "Clone $PROJECT_NAME Done"
    pushd "$PROJECT_NAME" > /dev/null || exit 1
    if [ "x$PROJ_HASH" != "x" ] ; then
        git reset --hard "$PROJ_HASH"
    fi
    popd > /dev/null || exit
    popd > /dev/null || exit
fi

# create build file
cp "$SCRIPT_DIR"/../scripts/build.sh "$SCRIPT_DIR"/../projects/"${PROJECT_NAME}"_"${BUG_NUMBER}"
pushd "$SCRIPT_DIR"/../projects/"${PROJECT_NAME}"_"${BUG_NUMBER}" > /dev/null || exit
sed -i "s|\$BUGGY_FILE|$BUGGY_FILE|" build.sh
sed -i "s|\$SUBJECT|$PROJECT_NAME|" build.sh
sed -i "s|\$BINARY|$BINARY|" build.sh
popd > /dev/null || exit

# build docker image
python "$SCRIPT_DIR"/../infra/helper.py build_image "$PROJECT_NAME" "$BUG_NUMBER"
if [ "x$ASSIGNED_CORE" != "x" ] ; then
    "$SCRIPT_DIR"/../infra/helper.py build_fuzzers --engine afl --core "$ASSIGNED_CORE" --sanitizer "$SANITIZER_TYPE" "$PROJECT_NAME" "$BUG_NUMBER" #--no_tty
else
    "$SCRIPT_DIR"/../infra/helper.py build_fuzzers --engine afl --sanitizer "$SANITIZER_TYPE" "$PROJECT_NAME" "$BUG_NUMBER" #--no_tty
fi

# copy execution log to local
container_id=$(docker ps --all | grep "${PROJECT_NAME}"_"${BUG_NUMBER}" | awk '{print $1}')
output_dir=$SCRIPT_DIR/../output/"${PROJECT_NAME}"_"${BUG_NUMBER}"
rm -rf "$output_dir"
mkdir -p "$output_dir"
docker cp "$container_id":/src/scripts/original.txt "$output_dir"
# docker cp "$container_id":/src/scripts/aflgo_part.txt "$output_dir"
docker cp "$container_id":/src/scripts/patches "$output_dir"
