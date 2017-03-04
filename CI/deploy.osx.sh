#!/bin/sh

cd build

DATE=`date +'%d%m%Y'`
TARGET_FILENAME="openmw-${DATE}-${TRAVIS_COMMIT}.dmg"

curl --ftp-create-dirs -T *.dmg -u $OSX_FTP_USER:$OSX_FTP_PASSWORD "ftp://s3.mydevil.net:21/nightly/${TARGET_FILENAME}"
