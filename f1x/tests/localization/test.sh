#!/bin/bash

assert-equal () {
    diff -q <($1) <(echo -ne "$2") > /dev/null
}

case "$1" in
    n1)
        assert-equal "./program 1" '2\n'
        ;;
    n2)
        assert-equal "./program 2" '3\n'
        ;;
    n3)
        assert-equal "./program 3" '4\n'
        ;;
    *)
        exit 1
        ;;
esac
