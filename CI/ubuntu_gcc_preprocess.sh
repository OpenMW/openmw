#!/bin/bash

set -xeo pipefail

SRC="${PWD:?}"
VERSION=$(git rev-parse HEAD)

mkdir -p build
cd build

cmake \
    -G Ninja \
    -D CMAKE_C_COMPILER=gcc \
    -D CMAKE_CXX_COMPILER=g++ \
    -D USE_SYSTEM_TINYXML=ON \
    -D OPENMW_USE_SYSTEM_RECASTNAVIGATION=ON \
    -D CMAKE_BUILD_TYPE=Release \
    -D CMAKE_C_FLAGS_RELEASE='-DNDEBUG -E -w' \
    -D CMAKE_CXX_FLAGS_RELEASE='-DNDEBUG -E -w' \
    -D CMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -D BUILD_BENCHMARKS=ON \
    -D BUILD_BSATOOL=ON \
    -D BUILD_BULLETOBJECTTOOL=ON \
    -D BUILD_ESMTOOL=ON \
    -D BUILD_ESSIMPORTER=ON \
    -D BUILD_LAUNCHER=ON \
    -D BUILD_LAUNCHER_TESTS=ON \
    -D BUILD_MWINIIMPORTER=ON \
    -D BUILD_NAVMESHTOOL=ON \
    -D BUILD_NIFTEST=ON \
    -D BUILD_OPENCS=ON \
    -D BUILD_OPENCS_TESTS=ON \
    -D BUILD_OPENMW=ON \
    -D BUILD_OPENMW_TESTS=ON \
    -D BUILD_COMPONENTS_TESTS=ON \
    -D BUILD_WIZARD=ON \
    "${SRC}"
cmake --build . --parallel

cd ..

scripts/preprocessed_file_size_stats.py --remove_prefix "${SRC}/" build > "${VERSION:?}.json"
ls -al "${VERSION:?}.json"

if [[ "${GENERATE_ONLY}" ]]; then
    exit 0
fi

git remote add target "${CI_MERGE_REQUEST_PROJECT_URL:-https://gitlab.com/OpenMW/openmw.git}"

TARGET_BRANCH="${CI_MERGE_REQUEST_TARGET_BRANCH_NAME:-master}"

git fetch target "${TARGET_BRANCH:?}"

if [[ "${CI_MERGE_REQUEST_SOURCE_BRANCH_NAME}" ]]; then
    git remote add source "${CI_MERGE_REQUEST_SOURCE_PROJECT_URL}"
    git fetch --unshallow source "${CI_MERGE_REQUEST_SOURCE_BRANCH_NAME}"
elif [[ "${CI_COMMIT_BRANCH}" ]]; then
    git fetch origin "${CI_COMMIT_BRANCH:?}"
else
    git fetch origin
fi

BASE_VERSION=$(git merge-base "target/${TARGET_BRANCH:?}" "${VERSION:?}")

# Save and use scripts from this commit because they may be absent or different in the base version
cp scripts/preprocessed_file_size_stats.py scripts/preprocessed_file_size_stats.bak.py
cp CI/ubuntu_gcc_preprocess.sh CI/ubuntu_gcc_preprocess.bak.sh
git checkout "${BASE_VERSION:?}"
mv scripts/preprocessed_file_size_stats.bak.py scripts/preprocessed_file_size_stats.py
mv CI/ubuntu_gcc_preprocess.bak.sh CI/ubuntu_gcc_preprocess.sh
env GENERATE_ONLY=1 CI/ubuntu_gcc_preprocess.sh
git checkout --force "${VERSION:?}"

scripts/preprocessed_file_size_stats_diff.py "${BASE_VERSION:?}.json" "${VERSION:?}.json"
