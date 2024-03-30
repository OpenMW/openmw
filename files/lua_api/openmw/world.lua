---
-- `openmw.world` is an interface to the game world for global scripts.
-- Can not be used from local scripts.
-- @module world
-- @usage local world = require('openmw.world')



---
-- List of currently active actors.
-- @field [parent=#world] openmw.core#ObjectList activeActors

---
-- List of players. Currently (since multiplayer is not yet implemented) always has one element.
-- @field [parent=#world] openmw.core#ObjectList players

---
-- Functions related to MWScript (see @{#MWScriptFunctions}).
-- @field [parent=#world] #MWScriptFunctions mwscript

---
-- Functions related to MWScript.
-- @type MWScriptFunctions

---
-- @type MWScriptVariables
-- @map <#string, #number>

---
-- Returns local mwscript on ``object``. Returns `nil` if the script doesn't exist or is not started.
-- @function [parent=#MWScriptFunctions] getLocalScript
-- @param openmw.core#GameObject object
-- @param openmw.core#GameObject player (optional) Will be used in multiplayer mode to get the script if there is a separate instance for each player. Currently has no effect.
-- @return #MWScript, #nil

---
-- Returns mutable global variables. In multiplayer, these may be specific to the provided player.
-- @function [parent=#MWScriptFunctions] getGlobalVariables
-- @param openmw.core#GameObject player (optional) Will be used in multiplayer mode to get the globals if there is a separate instance for each player. Currently has no effect.
-- @return #MWScriptVariables

---
-- Returns global mwscript with given recordId. Returns `nil` if the script doesn't exist or is not started.
-- Currently there can be only one instance of each mwscript, but in multiplayer it will be possible to have a separate instance per player.
-- @function [parent=#MWScriptFunctions] getGlobalScript
-- @param #string recordId
-- @param openmw.core#GameObject player (optional) Will be used in multiplayer mode to get the script if there is a separate instance for each player. Currently has no effect.
-- @return #MWScript, #nil

---
-- @type MWScript
-- @field #string recordId Id of the script
-- @field openmw.core#GameObject object The object the script is attached to.
-- @field openmw.core#GameObject player The player the script refers to.
-- @field #MWScriptVariables variables Local variables of the script (mutable)
-- @usage
-- for _, script in ipairs(world.mwscript.getLocalScripts(object)) do
--   -- print the value of local variable 'something' (0 if there is no such variable)
--   print(script.variables.something)
--   -- set the variable 'something' (raises an error if there is no such variable)
--   script.variables.something = 5
-- end

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
-- Frame duration in seconds
-- @function [parent=#world] getRealFrameDuration
-- @return #number

---
-- Whether the world is paused (onUpdate doesn't work when the world is paused).
-- @function [parent=#world] isWorldPaused
-- @return #boolean

---
-- Pause the game starting from the next frame.
-- @function [parent=#world] pause
-- @param #string tag (optional, empty string by default) The game will be paused until `unpause` is called with the same tag.

---
-- Remove given tag from the list of pause tags. Resume the game starting from the next frame if the list became empty.
-- @function [parent=#world] unpause
-- @param #string tag (optional, empty string by default) Needed to undo `pause` called with this tag.

---
-- The tags that are currently pausing the game.
-- @function [parent=#world] getPausedTags
-- @return #table

---
-- Return an object by RefNum/FormId.
-- Note: the function always returns @{openmw.core#GameObject} and doesn't validate that
-- the object exists in the game world. If it doesn't exist or not yet loaded to memory),
-- then `obj:isValid()` will be `false`.
-- @function [parent=#world] getObjectByFormId
-- @param #string formId String returned by `core.getFormId`
-- @return openmw.core#GameObject
-- @usage local obj = world.getObjectByFormId(core.getFormId('Morrowind.esm', 128964))

---
-- Create a new instance of the given record.
-- After creation the object is in the disabled state. Use :teleport to place to the world or :moveInto to put it into a container or an inventory.
-- Note that dynamically created creatures, NPCs, and container inventories will not respawn.
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
--
-- * @{openmw.types#PotionRecord},
-- * @{openmw.types#ArmorRecord},
-- * @{openmw.types#BookRecord},
-- * @{openmw.types#MiscellaneousRecord},
-- * @{openmw.types#ClothingRecord},
-- * @{openmw.types#WeaponRecord},
-- * @{openmw.types#ActivatorRecord}
-- @function [parent=#world] createRecord
-- @param #any record A record to be registered in the database. Must be one of the supported types.
-- @return #any A new record added to the database. The type is the same as the input's.

return nil
