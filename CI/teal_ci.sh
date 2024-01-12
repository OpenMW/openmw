#!/bin/bash -e

docs/source/install_luadocumentor_in_docker.sh
PATH=$PATH:~/luarocks/bin

pushd .
echo "Install Teal Cyan"
git clone https://github.com/teal-language/cyan.git --depth 1
cd cyan
luarocks make cyan-dev-1.rockspec
popd

scripts/generate_teal_declarations.sh ./teal_declarations
zip teal_declarations.zip -r teal_declarations
