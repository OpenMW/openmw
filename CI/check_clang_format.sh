#!/bin/bash -ex

set -o pipefail

CLANG_FORMAT="${CLANG_FORMAT:-clang-format}"
git ls-files -- ':(exclude)extern/' '*.cpp' '*.hpp' '*.h' |
    xargs -I '{}' -P $(nproc) bash -ec "\"${CLANG_FORMAT:?}\" --dry-run -Werror \"\${0:?}\" &> /dev/null || \"${CLANG_FORMAT:?}\" \"\${0:?}\" | git diff --color=always --no-index \"\${0:?}\" -" '{}' ||
    ( echo "clang-format differences detected"; exit -1 )
