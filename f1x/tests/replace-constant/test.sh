#!/bin/bash

assert-equal () {
    diff -q <($1) <(echo -ne "$2") > /dev/null
}

case "$1" in
    p1)
        assert-equal "./program 0" '1\n'
        ;;
    p2)
        assert-equal "./program 1" '1\n'
        ;;
    p3)
        assert-equal "./program 6" '0\n'
        ;;
    n1)
        assert-equal "./program 2" '1\n'
        ;;
    n2)
        assert-equal "./program 3" '1\n'
        ;;
    n3)
        assert-equal "./program 4" '1\n'
        ;;
    n4)
        assert-equal "./program 5" '1\n'
        ;;
    *)
        exit 1
        ;;
esac
