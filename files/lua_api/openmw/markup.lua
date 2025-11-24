---
-- Allows to work with markup languages.
-- @context global|menu|local|player
-- @module markup
-- @usage local markup = require('openmw.markup')



---
-- Convert YAML data to a Lua object
-- @function [parent=#markup] decodeYaml
-- @param #string inputData Data to decode. It has such limitations:
--
--   1. YAML format of [version 1.2](https://yaml.org/spec/1.2.2) is used.
--   2. Map keys should be scalar values (strings, booleans, numbers).
--   3. YAML tag system is not supported.
--   4. If a scalar is quoted, it is treated like a string.
-- Otherwise, type deduction works according to YAML 1.2 [Core Schema](https://yaml.org/spec/1.2.2/#103-core-schema).
--   5. Circular dependencies between YAML nodes are not allowed.
--   6. Lua 5.1 does not have integer numbers - all numeric scalars use a #number type (which use a floating point).
--   7. Integer scalars numbers values are limited by the "int" range. Use floating point notation for larger number in YAML files.
-- @return #any Lua object (can be table or scalar value).
-- @usage local result = markup.decodeYaml('{ "x": 1 }');
-- -- prints 1
-- print(result["x"])

---
-- Load a YAML file from the VFS to Lua object. Conventions are the same as in @{#markup.decodeYaml}.
-- @function [parent=#markup] loadYaml
-- @param #string fileName YAML file path in the VFS.
-- @return #any Lua object (can be table or scalar value).
-- @usage -- file contains '{ "x": 1 }' data
-- local result = markup.loadYaml('test.yaml');
-- -- prints 1
-- print(result["x"])


return nil
