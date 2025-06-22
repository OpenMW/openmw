#!/bin/sh -ex

if [[ "${MACOS_AMD64}" ]]; then
    arch -x86_64 ccache -s
else
    ccache -s
fi
