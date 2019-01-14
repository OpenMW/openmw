#!/bin/sh

echo "$OSX_DEPLOY_KEY" > ~/.ssh/openmw_deploy

cd build

DATE=`date +'%d%m%Y'`
SHORT_COMMIT=`git rev-parse --short ${TRAVIS_COMMIT}`
TARGET_FILENAME="OpenMW-${DATE}-${SHORT_COMMIT}.dmg"

if ! ssh -p "$OSX_DEPLOY_PORT" -i ~/.ssh/openmw_deploy "$OSX_DEPLOY_HOST" sh -c "ls ~/nightly" | grep $SHORT_COMMIT > /dev/null; then
    scp -P "$OSX_DEPLOY_PORT" -i ~/.ssh/openmw_deploy *.dmg "$OSX_DEPLOY_HOST:~/nightly"
fi
