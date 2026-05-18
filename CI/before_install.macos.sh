#!/bin/sh -e

DEPS_DIR="./deps"

while [ $# -gt 0 ]; do
    ARGSTR=$1
    shift

    if [ ${ARGSTR:0:1} != "-" ]; then
        echo "Unknown argument $ARGSTR"
        echo "Try '$0 -h'"
        exit 1
    fi

    for (( i=1; i<${#ARGSTR}; i++ )); do
        ARG=${ARGSTR:$i:1}
        case $ARG in
            d )
                if [ $i -lt $((${#ARGSTR} - 1)) ]; then
                    DEPS_DIR="${ARGSTR:$((i+1))}"
                    break
                else
                    DEPS_DIR=$1
                    shift
                fi
                ;;

            h )
                cat <<EOF
Usage: $0 [-dh]
Options:
    -d <dir>
        Use this folder as the directory to download deps into.
    -h
        Show this message
EOF
                exit 0
                ;;

            * )
                echo "Unknown argument $ARG."
                echo "Try '$0 -h'"
                exit 1
                ;;
        esac
    done
done

mkdir -p $DEPS_DIR

source ./CI/macos/deps_versions.sh

brew tap --repair
brew update --quiet

brew install curl p7zip

if [[ "${MACOS_AMD64}" ]]; then
    VCPKG_FILE="vcpkg-x64-osx-dynamic"
    command -v /usr/local/bin/brew || arch -x86_64 bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

    arch -x86_64 bash -c "command -v qmake >/dev/null 2>&1 && qmake -v | grep -F 'Using Qt version 6.' >/dev/null || /usr/local/bin/brew install qt@6"
else
    VCPKG_FILE="vcpkg-arm64-osx-dynamic"

    command -v qmake >/dev/null 2>&1 && qmake -v | grep -F "Using Qt version 6." >/dev/null || brew install qt@6
fi

curl "https://gitlab.com/OpenMW/openmw-deps/-/raw/main/macos/${VCPKG_FILE}-${VCPKG_TAG}-manifest.txt" -o $DEPS_DIR/openmw-manifest.txt

{ read -r URL && read -r HASH FILE; } < $DEPS_DIR/openmw-manifest.txt

curl -fSL -R -J $URL -o $DEPS_DIR/$FILE
echo "${HASH:?}  ${FILE:?}" | sha512sum
7z x -y -o$DEPS_DIR/openmw-deps-pre $DEPS_DIR/$FILE && \
    mv $DEPS_DIR/openmw-deps-pre/*/ $DEPS_DIR/openmw-deps/ && \
    rmdir $DEPS_DIR/openmw-deps-pre

command -v cmake >/dev/null 2>&1 || brew install cmake
