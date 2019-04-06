#!/bin/sh

# This script expect the following environment variables to be set:
# - OSX_DEPLOY_KEY: private SSH key, must be encoded like this before adding it to Travis secrets: https://github.com/travis-ci/travis-ci/issues/7715#issuecomment-433301692
# - OSX_DEPLOY_HOST: string specifying SSH of the following format: ssh-user@ssh-host
# - OSX_DEPLOY_PORT: SSH port, it can't be a part of the host string because scp doesn't accept hosts with ports
# - OSX_DEPLOY_HOST_FINGERPRINT: fingerprint of the host, can be obtained by using ssh-keygen -F [host]:port & putting it in double quotes when adding to Travis secrets

SSH_KEY_PATH="$HOME/.ssh/openmw_deploy"
REMOTE_PATH="\$HOME/nightly"

echo "$OSX_DEPLOY_KEY" > "$SSH_KEY_PATH"
chmod 600 "$SSH_KEY_PATH"
echo "$OSX_DEPLOY_HOST_FINGERPRINT" >> "$HOME/.ssh/known_hosts"

cd build || exit 1

DATE=$(date +'%d%m%Y')
SHORT_COMMIT=$(git rev-parse --short "${TRAVIS_COMMIT}")
TARGET_FILENAME="OpenMW-${DATE}-${SHORT_COMMIT}.dmg"

if ! ssh -p "$OSX_DEPLOY_PORT" -i "$SSH_KEY_PATH" "$OSX_DEPLOY_HOST" "ls \"$REMOTE_PATH\"" | grep "$SHORT_COMMIT" > /dev/null; then
    scp -P "$OSX_DEPLOY_PORT" -i "$SSH_KEY_PATH" ./*.dmg "$OSX_DEPLOY_HOST:$REMOTE_PATH/$TARGET_FILENAME"
else
    echo "An existing nightly build for commit ${SHORT_COMMIT} has been found, skipping upload."
fi
