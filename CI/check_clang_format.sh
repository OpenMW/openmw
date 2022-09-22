#!/bin/bash

CLANG_FORMAT="${CLANG_FORMAT:-clang-format}"
HAS_DIFFS=0

check_format_file() {
    local item=$1
    "$CLANG_FORMAT" --dry-run -Werror "$item" &>/dev/null 
    if [[ $? = 1 ]]; then
        "${CLANG_FORMAT}" "${item}" | git diff --color=always --no-index "${item}" -
        HAS_DIFFS=1
    fi
}

check_format() {
    local path=$1
    for item in $(find "$path" -type f -name "*");
    do
        if [[ "$item" =~ .*\.(cpp|hpp|h) ]]; then
            check_format_file "$item" &
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