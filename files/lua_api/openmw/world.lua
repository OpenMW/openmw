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
-- @param #any cellOrName (optional) other cell or cell name in the same exterior world space
-- @return openmw.core#Cell

---
-- List of all cells
-- @field [parent=#world] #list<openmw.core#Cell> cells
-- @usage for i, cell in ipairs(world.cells) do print(cell) end

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

---
-- Create a new instance of the given record.
-- After creation the object is in the disabled state. Use :teleport to place to the world or :moveInto to put it into a container or an inventory.
-- @function [parent=#world] createObject
-- @param #string recordId Record ID in lowercase
-- @param #number count (optional, 1 by default) The number of objects in stack
-- @return openmw.core#GameObject
-- @usage  -- put 100 gold on the ground at the position of `actor`
-- money = world.createObject('gold_001', 100)
-- money:teleport(actor.cell.name, actor.position)
-- @usage -- put 50 gold into the actor's inventory
-- money = world.createObject('gold_001', 50)
-- money:moveInto(types.Actor.inventory(actor))

---
-- Creates a custom record in the world database.
-- Eventually meant to support all records, but the current
-- set of supported types is limited to:
-- * @{openmw.types#PotionRecord},
-- * @{openmw.types#ArmorRecord},
-- * @{openmw.types#BookRecord},
-- * @{openmw.types#MiscellaneousRecord},
-- * @{openmw.types#ActivatorRecord}
-- @function [parent=#world] createRecord
-- @param #any record A record to be registered in the database. Must be one of the supported types.
-- @return #any A new record added to the database. The type is the same as the input's.

return nil
