#!/bin/bash

assert-equal () {
    diff -q <($1) <(echo -ne "$2") > /dev/null
}

case "$1" in
    p1)
        assert-equal "./program 3" '3\n'
        ;;
    p2)
        assert-equal "./program 2" '3\n'
        ;;
    n1)
        assert-equal "./program 1" '2\n'
        ;;
    *)
        exit 1
        ;;
esac
