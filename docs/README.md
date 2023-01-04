# Building OpenMW documentation

## Building in Docker (the recommended way)

### Preparing Docker image

Run the following commands from OpenMW source directory to build a new Docker image `openmw_doc`:
```bash
cd docs
docker build -t openmw_doc .
cd ..
```

(or run script `docs/prepare_docker_image.sh`)

This step needs to be repeated only if any dependencies were changed.

The image is based on `readthedocs/build:latest` that is newer than the image readthedocs uses by default (`readthedocs/build:stable`).
So if after some readthedocs update the documentation will stop building, there is a chance to detect it before the online docs will break.

### Generating HTML

Run the following command from OpenMW source directory to generate the documentation:
```bash
docker run --user "$(id -u)":"$(id -g)" --volume "$PWD":/openmw openmw_doc \
    sphinx-build /openmw/docs/source /openmw/docs/build
```

(or run script `docs/build_docs.sh`)

To view the generated documentation just open `docs/build/index.html` in a browser.

## Building without Docker (an alternative way)

Building documentation without Docker is more complicated as it requires multiple dependencies.

### Installation of required python packages

From OpenMW source directory
```bash
pip3 install -r docs/requirements.txt
```

### Installation of openmwluadocumentor:

**Debian/Ubuntu**

```bash
sudo apt install luarocks
git clone https://gitlab.com/ptmikheev/openmw-luadocumentor.git
cd openmw-luadocumentor/luarocks
luarocks --local pack openmwluadocumentor-0.2.0-1.rockspec
luarocks --local install openmwluadocumentor-0.2.0-1.src.rock
```

**Windows**

- install LuaRocks (heavily recommended to use the standalone package)
  https://github.com/luarocks/luarocks/wiki/Installation-instructions-for-Windows
- `git clone https://gitlab.com/ptmikheev/openmw-luadocumentor.git`
- `cd openmw-luadocumentor/luarocks`
- open "Developer Command Prompt for VS <2017/2019>" in this directory and run:
```bash
luarocks --local pack openmwluadocumentor-0.2.0-1.rockspec
luarocks --local install openmwluadocumentor-0.2.0-1.src.rock
```

### Generating HTML

Run the following command from OpenMW source directory to generate the documentation:
```bash
sphinx-build docs/source docs/build
```
