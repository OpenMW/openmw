#!/usr/bin/env bash

hdiutil attach ./*.dmg -mountpoint "${TRAVIS_BUILD_DIR}/openmw-package" > /dev/null || echo "hdutil has failed"

EXPECTED_PACKAGE_FILES=('Applications' 'OpenMW-CS.app' 'OpenMW.app')
PACKAGE_FILES=$(ls "${TRAVIS_BUILD_DIR}/openmw-package" | LC_ALL=C sort)

DIFF=$(diff <(printf "%s\n" "${EXPECTED_PACKAGE_FILES[@]}") <(printf "%s\n" "${PACKAGE_FILES[@]}"))
DIFF_STATUS=$?

if [[ $DIFF_STATUS -ne 0 ]]; then
  echo "The package should only contain an Applications symlink and two applications, see the following diff for details." >&2
  echo "$DIFF" >&2
  exit 1
fi
