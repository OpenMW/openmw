#!/bin/sh -ex

if [[ "${MACOS_AMD64}" ]]; then
    arch -x86_64 ccache -z -M "${CCACHE_SIZE}"
else
    ccache -z -M "${CCACHE_SIZE}"
fi
