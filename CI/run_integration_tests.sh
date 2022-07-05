#!/bin/bash -ex

git clone --depth=1 https://gitlab.com/OpenMW/example-suite.git

xvfb-run --auto-servernum --server-args='-screen 0 640x480x24x60' \
    scripts/integration_tests.py --omw build/install/bin/openmw --workdir integration_tests_output example-suite/

ls integration_tests_output/*.osg_stats.log | while read v; do
    scripts/osg_stats.py --stats '.*' --regexp_match < "${v}"
done
