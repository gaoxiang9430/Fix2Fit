#!/bin/bash

assert-equal-with-sanitizer () {
    outfile=$(mktemp /tmp/f1x-sio-out.XXXXXX)
    errfile=$(mktemp /tmp/f1x-sio-err.XXXXXX)
    $1 >$outfile 2>$errfile
    if diff -q $outfile <(echo -ne "$2") > /dev/null; then
        if grep -q "runtime error" $errfile; then
            exit 1 # correct output, runtime error
        else
            exit 0 # correct output, no runtime error
        fi
    else
        exit 1 # wrong output
    fi
}

case "$1" in
    n1)
        assert-equal-with-sanitizer "./program" "-2083255926"
        ;;
    *)
        exit 1
        ;;
esac
