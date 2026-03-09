---
-- Allows for manipulation of the data loaded from content files while the game is first started.
-- Records can be created and deleted using this package as if a content file had done so.
-- @context load
-- @module content
-- @usage local content = require('openmw.content')


--- @{#ActivatorContent}: Activator manipulation.
-- @field [parent=#content] #ActivatorContent activators

---
-- A mutable list of all @{openmw.types#ActivatorRecord}s.
-- @field [parent=#ActivatorContent] #list<openmw.types#ActivatorRecord> records
-- @usage
-- content.activators.records.MyActivator = { mwscript = 'float', model = 'meshes/w/w_chitin_arrow.nif', name = 'Quest marker' }

--- @{#DoorContent}: Door manipulation.
-- @field [parent=#content] #DoorContent doors

---
-- A mutable list of all @{openmw.types#DoorRecord}s.
-- @field [parent=#DoorContent] #list<openmw.types#DoorRecord> records
-- @usage
-- content.doors.records.MyDoor = { template = content.doors.records['door_dwrv_double00'], mwscript = 'blockedDoor', name = 'Overly Heavy Dwemer Door' }


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
