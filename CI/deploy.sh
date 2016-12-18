#!/bin/sh
#adapted from https://oncletom.io/2016/travis-ssh-deploy/
# and https://docs.travis-ci.com/user/encrypting-files/

echo $DEPLOY_PASS | gpg --passphrase-fd 0 $TRAVIS_BUILD_DIR/CI/deploy_rsa.gpg
eval "$(ssh-agent -s)"
chmod 600 $TRAVIS_BUILD_DIR/CI/deploy_rsa
ssh-add $TRAVIS_BUILD_DIR/CI/deploy_rsa

git push git+ssh://${BZR_LOGIN}@git.launchpad.net/openmw -f
