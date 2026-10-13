#!/bin/bash -e

shopt -s globstar nullglob dotglob

if [[ ! -f CMakeCache.txt ]]; then
    >&2 echo "This script must be run from the build directory."
    exit 1
fi

index=(.cmake/api/v1/reply/index-*.json)

if [[ ${#index[@]} -ne 1 ]] || jq -e '.reply | has("codemodel-v2") | not' "$index" > /dev/null; then
    echo "Running CMake query..."
    mkdir -p .cmake/api/v1/query
    touch .cmake/api/v1/query/codemodel-v2
    cmake .
    index=(.cmake/api/v1/reply/index-*.json)
    echo "Done."
fi

codemodel=".cmake/api/v1/reply/$(jq -r '.reply."codemodel-v2".jsonFile' "$index")"

targetsjson="$(jq -r '.configurations[].targets[].jsonFile' "$codemodel")"

artifacts=""
for targetjson in $targetsjson; do
    if jq -e '.type == "EXECUTABLE" or .type == "SHARED_LIBRARY" or .type =="MODULE_LIBRARY"' ".cmake/api/v1/reply/$targetjson" > /dev/null; then
        artifacts+="$(jq -r '.artifacts[].path' ".cmake/api/v1/reply/$targetjson")"
        artifacts+=$'\n'
    fi
done

for artifact in $artifacts; do
    if [[ -f $artifact ]]; then
        echo Processing $artifact
        buildid=$(readelf -n "$artifact" | grep -oP "Build ID:\s*\K\w+")
        dest="SymStore/buildid/$buildid"
        mkdir -p "$dest"
        echo Generating $dest/debuginfo
        objcopy --only-keep-debug --compress-debug-sections "$artifact" "$dest/debuginfo"
    fi
done
