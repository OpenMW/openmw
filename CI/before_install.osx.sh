#!/bin/sh -ex

export HOMEBREW_NO_EMOJI=1
export HOMEBREW_NO_INSTALL_CLEANUP=1
export HOMEBREW_AUTOREMOVE=1

if [[ "${MACOS_X86_64}" ]]; then
    ./CI/macos/before_install.x86_64.sh
else
    ./CI/macos/before_install.arm.sh
fi
