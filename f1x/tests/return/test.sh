#!/bin/bash

assert-equal () {
    diff -q <($1) <(echo -ne "$2") > /dev/null
}

case "$1" in
    n1)
        assert-equal "./program" '1\n'
        ;;
    *)
        exit 1
        ;;
esac
