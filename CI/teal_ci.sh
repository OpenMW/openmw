#!/bin/bash -e

docs/source/install_luadocumentor_in_docker.sh
PATH=$PATH:~/luarocks/bin

pushd .
echo "Install Teal Cyan"
git clone https://github.com/teal-language/cyan.git
cd cyan
git checkout v0.4.0
luarocks make cyan-0.4.0-1.rockspec
popd

cyan version
scripts/generate_teal_declarations.sh ./teal_declarations
