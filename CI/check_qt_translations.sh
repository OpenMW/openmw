#!/bin/bash -ex

set -o pipefail

LUPDATE="${LUPDATE:-lupdate}"

${LUPDATE:?} -locations none apps/wizard -ts files/lang/wizard_*.ts
${LUPDATE:?} -locations none apps/launcher -ts files/lang/launcher_*.ts
${LUPDATE:?} -locations none components/contentselector components/process -ts files/lang/components_*.ts

! (git diff --name-only | grep -q "^") || (echo -e "\033[0;31mBuild a 'translations' CMake target to update Qt localization for these files:\033[0;0m"; git diff --name-only | xargs -i echo -e "\033[0;31m{}\033[0;0m"; exit -1)
