#!/bin/sh
#adapted from https://oncletom.io/2016/travis-ssh-deploy/
# and https://docs.travis-ci.com/user/encrypting-files/

echo $DEPLOY_PASS | gpg --passphrase-fd 0 $TRAVIS_BUILD_DIR/CI/deploy_rsa.gpg
eval "$(ssh-agent -s)"
chmod 600 $TRAVIS_BUILD_DIR/CI/deploy_rsa
ssh-add $TRAVIS_BUILD_DIR/CI/deploy_rsa

bzr launchpad-login $BZR_LOGIN
bzr whoami "Travis Build <openmw@travis.org>"

bzr init
echo ".git" > .bzrignore
bzr add

HASH=$(git log --pretty=format:'%h' -n 1)
#bzr stat -S|grep ^?|sed 's/^? //'| while read LINE; do bzr add "$LINE"; done
bzr ci -m "Cron update. Git hash: $HASH"
bzr push --overwrite lp:openmw
