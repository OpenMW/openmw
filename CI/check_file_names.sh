#!/bin/bash -ex

git ls-files -- ':(exclude)extern/' '*.cpp' '*.hpp' '*.h' |
    grep -vP '/[a-z0-9]+\.(cpp|hpp|h)$' &&
    ( echo 'File names do not follow the naming convention, see https://gitlab.com/OpenMW/openmw/-/blob/master/CODESTYLE.md#files-and-layout'; exit -1 )
exit 0
