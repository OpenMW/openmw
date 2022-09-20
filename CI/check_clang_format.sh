#!/bin/bash

CLANG_FORMAT="clang-format-14"
HAS_DIFFS=0

check_format() {
    local path=$1
    local tempfile=$(mktemp)
    for item in $(find $path -type f -name "*");
    do
        if [[ "$item" =~ .*\.(cpp|hpp|h) ]]; then
            echo "Checking code formatting on $item"
            $CLANG_FORMAT $item > $tempfile
            git diff --color=always --no-index $item $tempfile
            if [[ $? = 1 ]]; then
                HAS_DIFFS=1
            fi
        fi;
    done;
    rm -f $tempfile
}

check_format "./apps"
check_format "./components"

if [[ $HAS_DIFFS -eq 1 ]]; then
    echo "clang-format differences detected" 
    exit 1
fi;

exit 0