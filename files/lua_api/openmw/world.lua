-------------------------------------------------------------------------------
-- `openmw.world` is an interface to the game world for global scripts.
-- Can not be used from local scripts.
-- @module world
-- @usage local world = require('openmw.world')



-------------------------------------------------------------------------------
-- List of currently active actors.
-- @field [parent=#world] openmw.core#ObjectList activeActors

-------------------------------------------------------------------------------
-- Evaluates a Query.
-- @function [parent=#world] selectObjects
-- @param openmw.query#Query query
-- @return openmw.core#ObjectList

-------------------------------------------------------------------------------
-- Loads a named cell
-- @function [parent=#world] getCellByName
-- @param #string cellName
-- @return openmw.core#Cell

-------------------------------------------------------------------------------
-- Loads an exterior cell by grid indices
-- @function [parent=#world] getExteriorCell
-- @param #number gridX
-- @param #number gridY
-- @return openmw.core#Cell


return nil

