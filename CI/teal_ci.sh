pushd .

docs/source/install_luadocumentor_in_docker.sh

PATH=$PATH:~/luarocks/bin

echo "Install Teal Cyan"
git clone https://github.com/teal-language/cyan.git --depth 1
cd cyan
luarocks make cyan-dev-1.rockspec

luarocks show openmwluadocumentor
luarocks show cyan

LUAROCKS=~/luarocks/bin
export LUAROCKS
popd
pushd docs
./generate_teal_declarations.sh ../teal_declarations
popd
zip teal_declarations.zip -r teal_declarations
