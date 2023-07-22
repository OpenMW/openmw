# Morrowind integration tests

A set of scripts to provide integration tests based on Morrowind content.

Simple usage:
```bash
"${OPENMW_BINARY_DIR:?}/openmw" \
    --config "${OPENMW_SOURCE_DIR:?}/scripts/data/morrowind_tests" \
    --data "${MORROWIND_DATA_DIR:?}"
```
