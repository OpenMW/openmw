#!/bin/sh
################################################################################
# Title         : generateDocumentationAndDeploy.sh
# Date created  : 2016/02/22
# Notes         :
__AUTHOR__="generated"
# Preconditions:
# - Packages doxygen doxygen-doc doxygen-latex doxygen-gui graphviz
#   must be installed.
# - Doxygen configuration file must have the destination directory empty and
#   source code directory with a $(TRAVIS_BUILD_DIR) prefix.
#
# Required global variables:
# - TRAVIS_BUILD_NUMBER    : The number of the current build.
# - TRAVIS_COMMIT          : The commit that the current build is testing.
# - DOXYFILE               : The Doxygen configuration file.
# - OPENMW_GITHUB_IO_TOKEN : Secure token to the github repository.
#
# For information on how to encrypt variables for Travis CI please go to
# https://docs.travis-ci.com/user/environment-variables/#Encrypted-Variables
# or https://gist.github.com/vidavidorra/7ed6166a46c537d3cbd2
#
# This script will generate Doxygen documentation and push the documentation to
# the epository specified by GH_REPO_REF.
#
################################################################################

# Exit with nonzero exit code if anything fails
set -e

GH_REPO_NAME="openmw.github.io"
DOXYFILE="$TRAVIS_BUILD_DIR/build/docs/Doxyfile"
GH_REPO_REF="github.com/openmw/openmw.github.io.git"

# Create a clean working directory for this script.
mkdir -p code_docs
cd code_docs

git clone https://git@$GH_REPO_REF
cd "$GH_REPO_NAME"

##### Configure git.
# Set the push default to simple i.e. push only the current branch.
git config --global push.default simple
# Pretend to be an user called Travis CI.
git config user.name "Travis CI"
git config user.email "travis@travis-ci.org"

# Remove everything currently in the repository.
# GitHub is smart enough to know which files have changed and which files have
# stayed the same and will only update the changed files. So the repository
# can be safely cleaned, and it is sure that everything pushed later is the new
# documentation.
CURRENTCOMMIT=`git rev-parse HEAD`
git reset --hard `git rev-list HEAD | tail -n 1`
git reset --soft $CURRENTCOMMIT

# Need to create a .nojekyll file to allow filenames starting with an underscore
# to be seen on the gh-pages site. Therefore creating an empty .nojekyll file.
# Presumably this is only needed when the SHORT_NAMES option in Doxygen is set
# to NO, which it is by default. So creating the file just in case.
touch .nojekyll

################################################################################
##### Generate the Doxygen code documentation and log the output.          #####
echo 'Generating Doxygen code documentation...'
# Redirect both stderr and stdout to the log file AND the console.
doxygen $DOXYFILE > /dev/null

DOCDIR="$TRAVIS_BUILD_DIR/build/docs/Doxygen"
echo "Checking existence of $DOCDIR/html/index.html"
################################################################################
##### Upload the documentation to the repository.                          #####
# Only upload if Doxygen successfully created the documentation.
# Check this by verifying that the html directory and the file html/index.html
# both exist. This is a good indication that Doxygen did it's work.
if [ -f "$DOCDIR/html/index.html" ]; then

    cp -R "$DOCDIR/html/." .

    echo 'Uploading documentation ...'
    # Add everything in this directory (the Doxygen code documentation).
    # GitHub is smart enough to know which files have changed and which files have
    # stayed the same and will only update the changed files.
    git add --all

    # Commit the added files with a title and description containing the Travis CI
    # build number and the GitHub commit reference that issued this build.
    git commit -q -m "Deploy code docs to GitHub Pages Travis build: ${TRAVIS_BUILD_NUMBER}" -m "Commit: ${TRAVIS_COMMIT}"

    # Force push to the remote repository.
    # The ouput is redirected to /dev/null to hide any sensitive credential data
    # that might otherwise be exposed.
    git push --force "https://${OPENMW_GITHUB_IO_TOKEN}@${GH_REPO_REF}" > /dev/null 2>&1
else
    echo '' >&2
    echo 'Warning: No documentation (html) files have been found!' >&2
    echo 'Warning: Not going to push the documentation to GitHub!' >&2
    exit 1
fi

