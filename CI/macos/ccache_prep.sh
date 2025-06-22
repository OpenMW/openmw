#!/bin/sh -ex

export CCACHE_BASEDIR="$(pwd)"
export CCACHE_DIR="$(pwd)/ccache"
mkdir -pv "${CCACHE_DIR}"
ccache -z -M "${CCACHE_SIZE}"

if [[ "${MACOS_AMD64}" ]]; then
    arch -x86_64 ccache -z -M "${CCACHE_SIZE}"
else
    ccache -z -M "${CCACHE_SIZE}"
fi
