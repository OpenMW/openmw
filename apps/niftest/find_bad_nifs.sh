#!/bin/bash
#Script to test all nif files (both loose, and in BSA archives) in data files directory

#The user input as an absolute path
DATAFILESDIR="`readlink -m "$1"`"
#Program used to test
TEST_PROG="`pwd`/niftest"

#Make sure our files don't bother anyone
NIFTEST_TMP_DIR="/tmp/niftest_$RANDOM/"
mkdir "$NIFTEST_TMP_DIR"
cd "$NIFTEST_TMP_DIR"

find "$DATAFILESDIR" -iname *bsa > nifs.txt
find "$DATAFILESDIR" -iname *nif >> nifs.txt

sed -e 's/.*/\"&\"/' nifs.txt > quoted_nifs.txt

xargs --arg-file=quoted_nifs.txt "$TEST_PROG" 2>&1 | tee nif_out.txt
# xargs --arg-file=quoted_nifs.txt "$TEST_PROG" 2> nif_out.txt >/dev/null

echo "List of bad NIF Files:"
cat nif_out.txt|grep File:|cut -d ' ' -f 2-

rm nifs.txt
rm quoted_nifs.txt
rm nif_out.txt
rmdir "$NIFTEST_TMP_DIR"
