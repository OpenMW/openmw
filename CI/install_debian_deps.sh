#!/bin/bash

set -euo pipefail

print_help() {
  echo "usage: $0 [group]..."
  echo
  echo "  available groups: "${!GROUPED_DEPS[@]}""
}

declare -rA GROUPED_DEPS=(
  [gcc]="binutils gcc g++ libc-dev"
  [clang]="binutils clang"

  # Common dependencies for building OpenMW.
  [openmw-deps]="
    make cmake ccache git pkg-config

    libboost-filesystem-dev libboost-program-options-dev
    libboost-system-dev libboost-iostreams-dev
    
    libavcodec-dev libavformat-dev libavutil-dev libswscale-dev libswresample-dev
    libsdl2-dev libqt5opengl5-dev libopenal-dev libunshield-dev libtinyxml-dev
    libbullet-dev liblz4-dev libpng-dev libjpeg-dev
    ca-certificates
  "
  # TODO: add librecastnavigation-dev when debian is ready

  # These dependencies can alternatively be built and linked statically.
  [openmw-deps-dynamic]="libmygui-dev libopenscenegraph-dev"
  [coverity]="curl"

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
    make cmake
    ccache curl unzip libcollada-dom-dev libfreetype6-dev libjpeg-dev libpng-dev
    libsdl2-dev libboost-system-dev libboost-filesystem-dev libgl-dev
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
set -x
mkdir -pv "$APT_CACHE_DIR"
apt-get update -yq
apt-get -q -o dir::cache::archives="$APT_CACHE_DIR" install -y --no-install-recommends "${deps[@]}"
