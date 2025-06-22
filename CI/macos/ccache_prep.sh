#!/bin/sh -ex

export CCACHE_BASEDIR="$(pwd)"
export CCACHE_DIR="$(pwd)/ccache"
mkdir -pv "${CCACHE_DIR}"

ccache -z -M "${CCACHE_SIZE}"
