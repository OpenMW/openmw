#!/bin/bash -e

docs/source/install_luadocumentor_in_docker.sh
PATH=$PATH:~/luarocks/bin

pushd .
echo "Install Teal Cyan"
git clone https://github.com/teal-language/cyan.git
cd cyan
git checkout 71eaea271bff489d82a9fb575b823b161b996162
luarocks make cyan-dev-1.rockspec
popd

scripts/generate_teal_declarations.sh ./teal_declarations
