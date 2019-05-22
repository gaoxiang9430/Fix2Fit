#!/bin/bash

assert-equal () {
    diff -q <($1) <(echo -ne "$2") > /dev/null
}

case "$1" in
    n1)
        assert-equal "./program 2" '1\n0\n'
        ;;
    n2)
        assert-equal "./program 3" '2\n1\n0\n'
        ;;
    n3)
        assert-equal "./program 4" '3\n2\n1\n0\n'
        ;;
    *)
        exit 1
        ;;
esac
