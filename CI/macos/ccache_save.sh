#!/bin/sh -ex

if [[ "${MACOS_AMD64}" ]]; then
    arch -x86_64 ccache -svv
else
    ccache -svv
fi
