#!/bin/sh -ex

export HOMEBREW_NO_EMOJI=1
export HOMEBREW_NO_INSTALL_CLEANUP=1
export HOMEBREW_AUTOREMOVE=1

if [[ "${MACOS_AMD64}" ]]; then
    ./CI/macos/before_install.amd64.sh
else
    ./CI/macos/before_install.arm64.sh
fi
