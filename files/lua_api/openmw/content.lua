---
-- Allows for manipulation of the data loaded from content files while the game is first started.
-- Records can be created and deleted using this package as if a content file had done so.
-- @context load
-- @module content
-- @usage local content = require('openmw.content')


--- @{#GlobalContent}: Global variable manipulation.
-- @field [parent=#content] #GlobalContent globals

---
-- A mutable list of all global mwscript variables.
-- @field [parent=#GlobalContent] #map<#string, #number> records
-- @usage
-- content.globals.records.MyVariable = 42

--- @{#StaticContent}: Static manipulation.
-- @field [parent=#content] #StaticContent statics

---
-- A mutable list of all @{openmw.types#StaticRecord}s.
-- @field [parent=#StaticContent] #list<openmw.types#StaticRecord> records
-- @usage
-- content.statics.records.MyStatic = { model = 'meshes/b/B_N_Wood Elf_M_Head_02.nif' }

return nil
