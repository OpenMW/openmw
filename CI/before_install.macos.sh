#!/bin/sh -ex

if [[ "${MACOS_AMD64}" ]]; then
    ./CI/macos/before_install.amd64.sh
else
    ./CI/macos/before_install.arm64.sh
fi
