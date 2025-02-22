#!/bin/bash -ex

git ls-files -- ':(exclude)extern/' '*.cpp' '*.hpp' '*.h' |
    grep -vP '/[a-z0-9]+\.(cpp|hpp|h)$' &&
    ( echo 'File names do not follow the naming convention, see https://wiki.openmw.org/index.php?title=Naming_Conventions#Files'; exit -1 )
exit 0
