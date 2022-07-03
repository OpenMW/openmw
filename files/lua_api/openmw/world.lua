---
-- `openmw.world` is an interface to the game world for global scripts.
-- Can not be used from local scripts.
-- @module world
-- @usage local world = require('openmw.world')



---
-- List of currently active actors.
-- @field [parent=#world] openmw.core#ObjectList activeActors

---
-- Loads a named cell
-- @function [parent=#world] getCellByName
-- @param #string cellName
-- @return openmw.core#Cell

---
-- Loads an exterior cell by grid indices
-- @function [parent=#world] getExteriorCell
-- @param #number gridX
-- @param #number gridY
-- @return openmw.core#Cell

---
-- Simulation time in seconds.
-- The number of simulation seconds passed in the game world since starting a new game.
-- @function [parent=#world] getSimulationTime
-- @return #number

---
-- The scale of simulation time relative to real time.
-- @function [parent=#world] getSimulationTimeScale
-- @return #number

---
-- Set the simulation time scale.
-- @function [parent=#world] setSimulationTimeScale
-- @param #number scale

---
-- Game time in seconds.
-- @function [parent=#world] getGameTime
-- @return #number

---
-- The scale of game time relative to simulation time.
-- @function [parent=#world] getGameTimeScale
-- @return #number

---
-- Set the ratio of game time speed to simulation time speed.
-- @function [parent=#world] setGameTimeScale
-- @param #number ratio

---
-- Whether the world is paused (onUpdate doesn't work when the world is paused).
-- @function [parent=#world] isWorldPaused
-- @return #boolean

return nil

