# Source files for OpenMW documentation

## Generating Lua scripting API reference

### Building Docker image

Run following command from OpenMW source directory to build a new Docker image `openmw.luadoc`:
```bash
docker build -f docs/source/Dockerfile.luadoc -t openmw.luadoc docs/source
```

### Generating HTML

Run following command from OpenMW source directory to generate HTML pages:
```bash
docker run --rm --tty --interactive --user "$(id -u)":"$(id -g)" \
    --volume "${PWD}":/openmw openmw.luadoc /openmw/docs/source/generate_luadoc_in_docker.sh
```
