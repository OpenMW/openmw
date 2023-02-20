if [ -z "$1" ]; then
  echo "Takes a path to the output directory as the argument"
  exit
fi

SCRIPTS_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
OPENMW_DIR=$(realpath $SCRIPTS_DIR/..)
DOCS_DIR=$(realpath $OPENMW_DIR/docs)
FILES_DIR=$(realpath $OPENMW_DIR/files)
OUTPUT_DIR=$(realpath "$1")

PATH=$PATH:~/lua-5.1.5/src
eval "$(luarocks path)"

rm -rf $OUTPUT_DIR
mkdir $OUTPUT_DIR
cp "$DOCS_DIR/tlconfig.lua" "$OUTPUT_DIR/tlconfig.lua"

build_path() {
  for file in $1
  do
    mkdir -p $OUTPUT_DIR/$(dirname $file)
    openmwluadocumentor -f teal -d "$OUTPUT_DIR" $file
  done
}

cd $FILES_DIR
build_path "lua_api/openmw/*lua"

data_paths=$($DOCS_DIR/source/luadoc_data_paths.sh)
for path in $data_paths
do
  build_path "data/$path"
done

cd $OUTPUT_DIR

mv lua_api/openmw ./
rm -r lua_api

mv data/* ./
rm -r data

cyan check **/*.d.tl
