---
-- `openmw.storage` contains functions to work with permanent Lua storage.
-- @module storage
-- @usage
-- local storage = require('openmw.storage')
-- local myModData = storage.globalSection('MyModExample')
-- myModData:set("someVariable", 1.0)
-- myModData:set("anotherVariable", { exampleStr='abc', exampleBool=true })
-- local function update()
--     if myModCfg:checkChanged() then
--         print('Data was changes by another script:')
--         print('MyModExample.someVariable =', myModData:get('someVariable'))
--         print('MyModExample.anotherVariable.exampleStr =',
--               myModData:get('anotherVariable').exampleStr)
--     end
-- end

---
-- Get a section of the global storage; can be used by any script, but only global scripts can change values.
-- Creates the section if it doesn't exist.
-- @function [parent=#storage] globalSection
-- @param #string sectionName
-- @return #StorageSection

---
-- Get a section of the player storage; can be used by player scripts only.
-- Creates the section if it doesn't exist.
-- @function [parent=#storage] playerSection
-- @param #string sectionName
-- @return #StorageSection

---
-- Get all global sections as a table; can be used by global scripts only.
-- Note that adding/removing items to the returned table doesn't create or remove sections.
-- @function [parent=#storage] allGlobalSections
-- @return #table

---
-- Get all global sections as a table; can be used by player scripts only.
-- Note that adding/removing items to the returned table doesn't create or remove sections.
-- @function [parent=#storage] allPlayerSections
-- @return #table

---
-- A map `key -> value` that represents a storage section.
-- @type StorageSection

---
-- Get value by a string key; if value is a table makes it readonly.
-- @function [parent=#StorageSection] get
-- @param self
-- @param #string key

---
-- Get value by a string key; if value is a table returns a copy.
-- @function [parent=#StorageSection] getCopy
-- @param self
-- @param #string key

---
-- Return `True` if any value in this section was changed by another script since the last `wasChanged`.
-- @function [parent=#StorageSection] wasChanged
-- @param self
-- @return #boolean

---
-- Copy all values and return them as a table.
-- @function [parent=#StorageSection] asTable
-- @param self
-- @return #table

---
-- Remove all existing values and assign values from given (the arg is optional) table.
-- Note: `section:reset()` removes all values, but not the section itself. Use `section:removeOnExit()` to remove the section completely.
-- @function [parent=#StorageSection] reset
-- @param self
-- @param #table values (optional) New values

---
-- Make the whole section temporary: will be removed on exit or when load a save.
-- No section can be removed immediately because other scripts may use it at the moment.
-- Temporary sections have the same interface to get/set values, the only difference is they will not
-- be saved to the permanent storage on exit.
-- This function can not be used for a global storage section from a local script.
-- @function [parent=#StorageSection] removeOnExit
-- @param self

---
-- Set value by a string key; can not be used for global storage from a local script.
-- @function [parent=#StorageSection] set
-- @param self
-- @param #string key
-- @param #any value

return nil

