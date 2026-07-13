# Morrowind integration tests

A set of scripts to provide integration tests based on Morrowind content.

Simple usage using default user configuration:

```bash
"${OPENMW_BINARY_DIR:?}/openmw" \
    --config "${OPENMW_SOURCE_DIR:?}/scripts/data/morrowind_tests" \
    --data "${MORROWIND_DATA_DIR:?}"
```

Or using dedicated configuration based on Morrowind.ini:

```bash
scripts/integration_tests.py \
    --omw "${OPENMW_BINARY_DIR:?}/openmw" \
    --data "${MORROWIND_DIR:?}/Data Files" \
    --ini "${MORROWIND_DIR:?}/Morrowind.ini" \
    scripts/data/morrowind_tests
```
