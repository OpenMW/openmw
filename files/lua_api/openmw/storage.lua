---
-- `openmw.storage` contains functions to work with permanent Lua storage.
-- @module storage
-- @usage
-- local storage = require('openmw.storage')
-- local myModData = storage.globalSection('MyModExample')
-- myModData:set("someVariable", 1.0)
-- myModData:set("anotherVariable", { exampleStr='abc', exampleBool=true })
-- local async = require('openmw.async')
-- myModData:subscribe(async:callback(function(section, key)
--     if key then
--         print('Value is changed:', key, '=', myModData:get(key))
--     else
--         print('All values are changed')
--     end
-- end))

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
-- Subscribe to changes in this section.
-- First argument of the callback is the name of the section (so one callback can be used for different sections).
-- The second argument is the changed key (or `nil` if `reset` was used and all values were changed at the same time)
-- @function [parent=#StorageSection] subscribe
-- @param self
-- @param openmw.async#Callback callback

---
-- Copy all values and return them as a table.
-- @function [parent=#StorageSection] asTable
-- @param self
-- @return #table

---
-- Remove all existing values and assign values from given (the arg is optional) table.
-- This function can not be used for a global storage section from a local script.
-- Note: `section:reset()` removes the section.
-- @function [parent=#StorageSection] reset
-- @param self
-- @param #table values (optional) New values

---
-- Make the whole section temporary: will be removed on exit or when load a save.
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

