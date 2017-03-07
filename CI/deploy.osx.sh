#!/bin/sh

cd build

DATE=`date +'%d%m%Y'`
SHORT_COMMIT=`git rev-parse --short ${TRAVIS_COMMIT}`
TARGET_FILENAME="openmw-${DATE}-${SHORT_COMMIT}.dmg"

curl --ssl --ftp-create-dirs -T *.dmg -u $OSX_FTP_USER:$OSX_FTP_PASSWORD "ftp://s3.mydevil.net:21/nightly/${TARGET_FILENAME}"
