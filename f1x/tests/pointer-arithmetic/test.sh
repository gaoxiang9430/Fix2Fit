#!/bin/bash

assert-equal () {
    diff -q <($1) <(echo -ne "$2") > /dev/null
}

case "$1" in
    n1)
        assert-equal "./program 1 2" '1\n'
        ;;
    p1)
        assert-equal "./program 2 1" '0\n'
        ;;
    p2)
        assert-equal "./program 2 2" '0\n'
        ;;
    *)
        exit 1
        ;;
esac
