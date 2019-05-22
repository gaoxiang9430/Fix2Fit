#!/usr/bin/env bash

#  This file is part of f1x.
#  Copyright (C) 2016  Sergey Mechtaev, Gao Xiang, Shin Hwei Tan, Abhik Roychoudhury
#
#  f1x is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.

require () {
    hash "$1" 2>/dev/null || { echo "command $1 is not found"; exit 1; }
}

require f1x
require make


get-cmd () {
    case "$1" in
        if-condition)
            echo "f1x --files program.c --driver test.sh --tests n1 p1 p2 --test-timeout 1000"
            ;;
        assign-in-condition)
            echo "f1x --files program.c --driver test.sh --tests n1 p1 p2 --test-timeout 1000"
            ;;
        guarded-assignment)
            echo "f1x --files program.c --driver test.sh --tests n1 p1 p2 --test-timeout 1000"
            ;;
        int-assignment)
            echo "f1x --files program.c --driver test.sh --tests n1 p1 p2 --test-timeout 1000"
            ;;
        dangling-else)
            echo "f1x --files program.c --driver test.sh --tests n1 p1 p2 --test-timeout 1000"
            ;;
        break)
            echo "f1x --files program.c --driver test.sh --tests n1 p1 p2 --test-timeout 1000"
            ;;
        array-subscripts)
            echo "f1x --files program.c --driver test.sh --tests n1 p1 p2 --test-timeout 1000"
            ;;
        cast-expr)
            echo "f1x --files program.c --driver test.sh --tests n1 p1 p2 --test-timeout 1000"
            ;;
        return)
            echo "f1x --files program.c --driver test.sh --tests n1 --test-timeout 1000"
            ;;
        deletion)
            echo "f1x --files program.c:11 --driver test.sh --tests n1 n2 n3 --test-timeout 1000"
            ;;
        loop-condition)
            echo "f1x --files program.c:7 --driver test.sh --tests n1 n2 n3 --test-timeout 1000"
            ;;
        memberexpr)
            echo "f1x --files program.c --driver test.sh --tests n1 p1 p2 --test-timeout 1000"
            ;;
        pointer)
            echo "f1x --files program.c:25 --driver test.sh --tests n1 n2 p1 --test-timeout 1000"
            ;;
        replace-constant)
            echo "f1x --files program.c --driver test.sh --tests n1 n2 n3 n4 p1 p2 p3 --test-timeout 1000"
            ;;
        substitute-bool2)
            echo "f1x --files program.c --driver test.sh --tests n1 n2 p1 --test-timeout 1000"
            ;;
        inside-ifdef)
            echo "f1x --files program.c --driver test.sh --tests n1 p1 p2 --test-timeout 1000"
            ;;
        intersect-ifdef)
            echo "f1x --files program.c --driver test.sh --tests n1 p1 p2 --test-timeout 1000"
            ;;
        non-integer-member)
            echo "f1x --files program.c --driver test.sh --tests n1 p1 p2 --test-timeout 1000"
            ;;
        pointer-arithmetic)
            echo "f1x --files program.c:11 --driver test.sh --tests n1 p1 p2 --test-timeout 1000"
            ;;
        divide-by-zero)
            echo "f1x --files program.c --driver test.sh --tests n1 n2 n3 --test-timeout 1000"
            ;;
        null-dereference)
            echo "f1x --files program.c:15 --driver test.sh --tests n1 p1 p2 --test-timeout 1000"
            ;;
        incomplete-pointee)
            echo "f1x --files program.c --driver test.sh --tests n1 p1 p2 --test-timeout 1000"
            ;;
        refine-condition)
            echo "f1x --files program.c --driver test.sh --tests n1 p1 p2 --test-timeout 1000"
            ;;
        multifile)
            echo "f1x --files program.c lib.c --driver test.sh --tests n1 p1 p2 --test-timeout 1000"
            ;;
        localization)
            echo "f1x --localize 1 --driver test.sh --tests n1 n2 n3 --test-timeout 1000"
            ;;
        array-element-update)
            echo "f1x --files program.c:9 --driver test.sh --tests n1 n2 p1 --test-timeout 1000"
            ;;
        signed-int-overflow)
            echo "f1x --files program.c:9 --driver test.sh --tests n1 --test-timeout 1000 --disable-vteq"
            ;;
        *)
            exit 1
            ;;
    esac
}

cd "$( dirname "${BASH_SOURCE[0]}" )"

if [[ -z "$TESTS" ]]; then
    TESTS=`ls -d */`
fi

for test in $TESTS; do
    test=${test%%/} # removing slash in the end
    echo -n "* testing $test... "

    work_dir=`mktemp -d`
    cp -r "$test"/* "$work_dir"

    repair_cmd=$(get-cmd $test)
    if [[ ($? != 0) ]]; then
        echo "command for test $test is not defined"
        exit 1
    fi

    case "$test" in
        signed-int-overflow)
            # The following is safer since we are using clang for F1X_PROJECT_CC
            repair_cmd="$repair_cmd --enable-llvm-cov"
            (cd $work_dir; F1X_CC_LIBS='-lstdc++' F1X_RUNTIME_CXX='clang -fsanitize=undefined' F1X_PROJECT_CC='clang' $repair_cmd  --output "$work_dir/output.patch" --enable-cleanup &> "$work_dir/log.txt")
            ;;
        *)
            # When F1X_PROJECT_CC is clang, we need to use --enable-llvm-cov
            project_compiler=$(basename $F1X_PROJECT_CC)
            if [[ $project_compiler = *"clang"* ]] ; then
                repair_cmd="$repair_cmd --enable-llvm-cov"
            fi
            (cd $work_dir; $repair_cmd  --output "$work_dir/output.patch" --enable-cleanup &> "$work_dir/log.txt")
            ;;
    esac
    if [[ ($? != 0) || (! -f "$work_dir/output.patch") || (! -s "$work_dir/output.patch") ]]; then
        echo 'FAIL'
        echo "----------------------------------------"
        case "$test" in
            signed-int-overflow)
                echo "cmd: (cd $work_dir; F1X_CC_LIBS='-lstdc++' F1X_RUNTIME_CXX='clang -fsanitize=undefined' F1X_PROJECT_CC='clang' $repair_cmd)"
                ;;
            *)
                echo "cmd: (cd $work_dir; $repair_cmd)"
                ;;
        esac
        echo "log: $work_dir/log.txt"
        echo "----------------------------------------"
        exit 1
    fi

    rm -rf "$work_dir"
    echo 'PASS'
done

echo "----------------------------------------"
echo "f1x passed the tests"
echo "----------------------------------------"
