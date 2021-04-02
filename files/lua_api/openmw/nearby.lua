-------------------------------------------------------------------------------
-- `openmw.nearby` provides read-only access to the nearest area of the game world.
-- Can be used only from local scripts.
-- @module nearby
-- @usage local nearby = require('openmw.nearby')



-------------------------------------------------------------------------------
-- List of nearby activators.
-- @field [parent=#nearby] openmw.core#ObjectList activators

-------------------------------------------------------------------------------
-- List of nearby actors.
-- @field [parent=#nearby] openmw.core#ObjectList actors

-------------------------------------------------------------------------------
-- List of nearby containers.
-- @field [parent=#nearby] openmw.core#ObjectList containers

-------------------------------------------------------------------------------
-- List of nearby doors.
-- @field [parent=#nearby] openmw.core#ObjectList doors

-------------------------------------------------------------------------------
-- Everything that can be picked up in the nearby.
-- @field [parent=#nearby] openmw.core#ObjectList items

-------------------------------------------------------------------------------
-- Evaluates a Query.
-- @function [parent=#nearby] selectObjects
-- @param openmw.query#Query query
-- @return openmw.core#ObjectList

return nil

