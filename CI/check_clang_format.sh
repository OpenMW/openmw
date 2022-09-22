#!/bin/bash

CLANG_FORMAT="clang-format-14"
HAS_DIFFS=0

check_format() {
    local path=$1
    for item in $(find $path -type f -name "*");
    do
        if [[ "$item" =~ .*\.(cpp|hpp|h) ]]; then
            echo "Checking code formatting on $item"
            $CLANG_FORMAT --dry-run -Werror "$item"
            if [[ $? = 1 ]]; then
                local tempfile=$(mktemp)
                # Avoid having different modes in the diff.
                chmod --reference="$item" "$tempfile"
                # Generate diff
                $CLANG_FORMAT "$item" > "$tempfile"
                git diff --color=always --no-index $item $tempfile
                rm -f "$tempfile"
                HAS_DIFFS=1
            fi
        fi;
    done;
}

check_format "./apps"
check_format "./components"

if [[ $HAS_DIFFS -eq 1 ]]; then
    echo "clang-format differences detected" 
    exit 1
fi;

exit 0