#!/bin/bash

set -euo pipefail

print_help() {
  echo "usage: $0 [group]..."
  echo
  echo "  available groups: "${!GROUPED_DEPS[@]}""
}

declare -rA GROUPED_DEPS=(
  [gcc]="binutils gcc build-essential cmake ccache curl unzip git pkg-config mold"
  [clang]="binutils clang make cmake ccache curl unzip git pkg-config mold"
  [coverity]="binutils clang-12 make cmake ccache curl unzip git pkg-config"
  [gcc_preprocess]="
    binutils
    build-essential
    clang
    cmake
    curl
    gcc
    git
    libclang-dev
    ninja-build
    python3-clang
    python3-pip
    unzip
  "

  # Common dependencies for building OpenMW.
  [openmw-deps]="
    libboost-program-options-dev
    libboost-system-dev libboost-iostreams-dev

    libavcodec-dev libavformat-dev libavutil-dev libswscale-dev libswresample-dev
    libsdl2-dev libqt6opengl6-dev qt6-tools-dev qt6-tools-dev-tools libopenal-dev
    libunshield-dev libtinyxml-dev libbullet-dev liblz4-dev libpng-dev libjpeg-dev
    libluajit-5.1-dev librecast-dev libsqlite3-dev ca-certificates libicu-dev
    libyaml-cpp-dev libqt6svg6 libqt6svg6-dev
  "

  # These dependencies can alternatively be built and linked statically.
  [openmw-deps-dynamic]="libmygui-dev libopenscenegraph-dev libsqlite3-dev libcollada-dom-dev"
  [clang-tidy]="clang-tidy"

  # Pre-requisites for building MyGUI and OSG for static linking.
  #
  # * MyGUI and OSG: libsdl2-dev liblz4-dev libfreetype6-dev
  # * OSG: libgl-dev
  #
  #   Plugins:
  #   * DAE: libcollada-dom-dev libboost-system-dev libboost-filesystem-dev
  #   * JPEG: libjpeg-dev
  #   * PNG: libpng-dev
  [openmw-deps-static]="
    libcollada-dom-dev libfreetype6-dev libjpeg-dev libpng-dev
    libsdl2-dev libboost-system-dev libboost-filesystem-dev libgl-dev
  "

  [openmw-coverage]="pipx"

  [openmw-integration-tests]="
    ca-certificates
    gdb
    git
    git-lfs
    libavcodec60
    libavformat60
    libavutil58
    libboost-iostreams1.83.0
    libboost-program-options1.83.0
    libboost-system1.83.0
    libbullet3.24
    libcollada-dom2.5-dp0
    libicu74
    libjpeg8
    libluajit-5.1-2
    liblz4-1
    libmyguiengine3debian1v5
    libopenal1
    libopenscenegraph161
    libpng16-16
    libqt6opengl6
    librecast1
    libsdl2-2.0-0
    libsqlite3-0
    libswresample4
    libswscale7
    libtinyxml2.6.2v5
    libyaml-cpp0.8
    python3-pip
    xvfb
  "

  [libasan]="libasan8"

  [android]="binutils build-essential cmake ccache curl unzip git pkg-config"

  [openmw-clang-format]="
    clang-format-14
    git-core
  "

  [openmw-qt-translations]="
    qt6-tools-dev
    qt6-tools-dev-tools
    git-core
  "
)

if [[ $# -eq 0 ]]; then
  >&2 print_help
  exit 1
fi

deps=()
for group in "$@"; do
  if [[ ! -v GROUPED_DEPS[$group] ]]; then
    >&2 echo "error: unknown group ${group}"
    exit 1
  fi
  deps+=(${GROUPED_DEPS[$group]})
done

export APT_CACHE_DIR="${PWD}/apt-cache"
export DEBIAN_FRONTEND=noninteractive
set -x
mkdir -pv "$APT_CACHE_DIR"

while true; do
  apt-get update -yqq && break
done

apt-get -qq -o dir::cache::archives="$APT_CACHE_DIR" install -y --no-install-recommends software-properties-common gnupg >/dev/null

while true; do
  add-apt-repository -y ppa:openmw/openmw && break
done

while true; do
  add-apt-repository -y ppa:openmw/openmw-daily && break
done

while true; do
  add-apt-repository -y ppa:openmw/staging && break
done

apt-get -qq -o dir::cache::archives="$APT_CACHE_DIR" install -y --no-install-recommends "${deps[@]}" >/dev/null
apt list --installed
