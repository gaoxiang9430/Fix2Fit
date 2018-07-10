#!/bin/bash
uage="Usage: ./f1xcmd.sh F1X_COMMAND"

if [[ $# > 0 ]]; then
    F1X="$1"
    shift
else
    echo "$usage"
    exit 1
fi

pushd ../$SUBJECT > /dev/null
$F1X -f $BUGGY_FILE -t 1 -T 1000 -d $DRIVER -b ./project_build.sh --output-one-per-loc -a -P /out -N standard_fuzzer |& tee log.txt
popd > /dev/null
