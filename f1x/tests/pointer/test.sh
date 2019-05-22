#!/bin/bash

assert-equal () {
    diff -q <($1) <(echo -ne "$2") > /dev/null
}

case "$1" in
    p1)
        assert-equal "./program 0" '1\n'
        ;;
    n1)
        assert-equal "./program 1" '1\n'
        ;;
    n2)
        assert-equal "./program 2" '0\n'
        ;;
    *)
        exit 1
        ;;
esac
