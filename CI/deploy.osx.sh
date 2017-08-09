#!/bin/sh

cd build

DATE=`date +'%d%m%Y'`
SHORT_COMMIT=`git rev-parse --short ${TRAVIS_COMMIT}`
TARGET_FILENAME="OpenMW-${DATE}-${SHORT_COMMIT}.dmg"

curl --ssl --ftp-create-dirs -T *.dmg -u $OSX_FTP_USER:$OSX_FTP_PASSWORD "${OSX_FTP_URL}${TARGET_FILENAME}"
