-------------------------------------------------------------------------------
-- `openmw.core` defines functions and types that are available in both local
-- and global scripts.
-- @module core
-- @usage local core = require('openmw.core')



-------------------------------------------------------------------------------
-- The revision of OpenMW Lua API. It is an integer that is incremented every time the API is changed.
-- @field [parent=#core] #number API_REVISION

-------------------------------------------------------------------------------
-- Terminates the game and quits to the OS. Should be used only for testing purposes.
-- @function [parent=#core] quit

-------------------------------------------------------------------------------
-- Send an event to global scripts.
-- @function [parent=#core] sendGlobalEvent
-- @param #string eventName
-- @param eventData

-------------------------------------------------------------------------------
-- Game time in seconds.
-- The number of seconds in the game world, passed from starting a new game.
-- @function [parent=#core] getGameTimeInSeconds
-- @return #number

-------------------------------------------------------------------------------
-- Current time of the game world in hours.
-- Note that the number of game seconds in a game hour is not guaranteed to be fixed.
-- @function [parent=#core] getGameTimeInHours
-- @return #number


-------------------------------------------------------------------------------
-- @type OBJECT_TYPE
-- @field [parent=#OBJECT_TYPE] #string Activator "Activator"
-- @field [parent=#OBJECT_TYPE] #string Armor "Armor"
-- @field [parent=#OBJECT_TYPE] #string Book "Book"
-- @field [parent=#OBJECT_TYPE] #string Clothing "Clothing"
-- @field [parent=#OBJECT_TYPE] #string Container "Container"
-- @field [parent=#OBJECT_TYPE] #string Creature "Creature"
-- @field [parent=#OBJECT_TYPE] #string Door "Door"
-- @field [parent=#OBJECT_TYPE] #string Ingredient "Ingredient"
-- @field [parent=#OBJECT_TYPE] #string Light "Light"
-- @field [parent=#OBJECT_TYPE] #string Miscellaneous "Miscellaneous"
-- @field [parent=#OBJECT_TYPE] #string NPC "NPC"
-- @field [parent=#OBJECT_TYPE] #string Player "Player"
-- @field [parent=#OBJECT_TYPE] #string Potion "Potion"
-- @field [parent=#OBJECT_TYPE] #string Static "Static"
-- @field [parent=#OBJECT_TYPE] #string Weapon "Weapon"

-------------------------------------------------------------------------------
-- Possible `object.type` values.
-- @field [parent=#core] #OBJECT_TYPE OBJECT_TYPE


-------------------------------------------------------------------------------
-- @type EQUIPMENT_SLOT
-- @field [parent=#EQUIPMENT_SLOT] #number Helmet
-- @field [parent=#EQUIPMENT_SLOT] #number Cuirass
-- @field [parent=#EQUIPMENT_SLOT] #number Greaves
-- @field [parent=#EQUIPMENT_SLOT] #number LeftPauldron
-- @field [parent=#EQUIPMENT_SLOT] #number RightPauldron
-- @field [parent=#EQUIPMENT_SLOT] #number LeftGauntlet
-- @field [parent=#EQUIPMENT_SLOT] #number RightGauntlet
-- @field [parent=#EQUIPMENT_SLOT] #number Boots
-- @field [parent=#EQUIPMENT_SLOT] #number Shirt
-- @field [parent=#EQUIPMENT_SLOT] #number Pants
-- @field [parent=#EQUIPMENT_SLOT] #number Skirt
-- @field [parent=#EQUIPMENT_SLOT] #number Robe
-- @field [parent=#EQUIPMENT_SLOT] #number LeftRing
-- @field [parent=#EQUIPMENT_SLOT] #number RightRing
-- @field [parent=#EQUIPMENT_SLOT] #number Amulet
-- @field [parent=#EQUIPMENT_SLOT] #number Belt
-- @field [parent=#EQUIPMENT_SLOT] #number CarriedRight
-- @field [parent=#EQUIPMENT_SLOT] #number CarriedLeft
-- @field [parent=#EQUIPMENT_SLOT] #number Ammunition

-------------------------------------------------------------------------------
-- Available equipment slots. Used in `object:getEquipment` and `object:setEquipment`.
-- @field [parent=#core] #EQUIPMENT_SLOT EQUIPMENT_SLOT


-------------------------------------------------------------------------------
-- Any object that exists in the game world and has a specific location.
-- Player, actors, items, and statics are game objects.
-- @type GameObject
-- @field [parent=#GameObject] openmw.util#Vector3 position Object position.
-- @field [parent=#GameObject] openmw.util#Vector3 rotation Object rotation (ZXY order).
-- @field [parent=#GameObject] #Cell cell The cell where the object currently is. During loading a game and for objects in an inventory or a container `cell` is nil.
-- @field [parent=#GameObject] #string type Type of the object (see @{openmw.core#OBJECT_TYPE}).
-- @field [parent=#GameObject] #number count Count (makes sense if holded in a container).
-- @field [parent=#GameObject] #string recordId Record ID.
-- @field [parent=#GameObject] #Inventory inventory Inventory of an Player/NPC or content of an container.
-- @field [parent=#GameObject] #boolean isTeleport `True` if it is a teleport door (only if `object.type` == "Door").
-- @field [parent=#GameObject] openmw.util#Vector3 destPosition Destination (only if a teleport door).
-- @field [parent=#GameObject] openmw.util#Vector3 destRotation Destination rotation (only if a teleport door).
-- @field [parent=#GameObject] #string destCell Destination cell (only if a teleport door).

-------------------------------------------------------------------------------
-- Is the object still exists/available.
-- Returns true if the object exists and loaded, and false otherwise. If false, then every
-- access to the object will raise an error.
-- @function [parent=#GameObject] isValid
-- @param self
-- @return #boolean

-------------------------------------------------------------------------------
-- Returns true if the object is an actor and is able to move. For dead, paralized,
-- or knocked down actors in returns false.
-- access to the object will raise an error.
-- @function [parent=#GameObject] canMove
-- @param self
-- @return #boolean

-------------------------------------------------------------------------------
-- Speed of running. Returns 0 if not an actor, but for dead actors it still returns a positive value.
-- @function [parent=#GameObject] getRunSpeed
-- @param self
-- @return #number

-------------------------------------------------------------------------------
-- Speed of walking. Returns 0 if not an actor, but for dead actors it still returns a positive value.
-- @function [parent=#GameObject] getWalkSpeed
-- @param self
-- @return #number

-------------------------------------------------------------------------------
-- Current speed. Can be called only from a local script.
-- @function [parent=#GameObject] getCurrentSpeed
-- @param self
-- @return #number

-------------------------------------------------------------------------------
-- Is the actor standing on ground. Can be called only from a local script.
-- @function [parent=#GameObject] isOnGround
-- @param self
-- @return #boolean

-------------------------------------------------------------------------------
-- Is the actor in water. Can be called only from a local script.
-- @function [parent=#GameObject] isSwimming
-- @param self
-- @return #boolean

-------------------------------------------------------------------------------
-- Is the actor in weapon stance. Can be called only from a local script.
-- @function [parent=#GameObject] isInWeaponStance
-- @param self
-- @return #boolean

-------------------------------------------------------------------------------
-- Is the actor in magic stance. Can be called only from a local script.
-- @function [parent=#GameObject] isInMagicStance
-- @param self
-- @return #boolean

-------------------------------------------------------------------------------
-- Send local event to the object.
-- @function [parent=#GameObject] sendEvent
-- @param self
-- @param #string eventName
-- @param eventData

-------------------------------------------------------------------------------
-- Returns `true` if the item is equipped on the object.
-- @function [parent=#GameObject] isEquipped
-- @param self
-- @param #GameObject item
-- @return #boolean

-------------------------------------------------------------------------------
-- Get equipment.
-- Returns a table `slot` -> `GameObject` of currently equipped items.
-- See @{openmw.core#EQUIPMENT_SLOT}. Returns empty table if the object doesn't have
-- equipment slots.
-- @function [parent=#GameObject] getEquipment
-- @param self
-- @return #map<#number,#GameObject>

-------------------------------------------------------------------------------
-- Set equipment.
-- Keys in the table are equipment slots (see @{openmw.core#EQUIPMENT_SLOT}). Each
-- value can be either a `GameObject` or recordId. Raises an error if
-- the object doesn't have equipment slots and table is not empty. Can be
-- called only on self or from a global script.
-- @function [parent=#GameObject] setEquipment
-- @param self
-- @param equipment

-------------------------------------------------------------------------------
-- Add new local script to the object.
-- Can be called only from a global script. Script should be specified in a content
-- file (omwgame/omwaddon/omwscripts) with a CUSTOM flag.
-- @function [parent=#GameObject] addScript
-- @param self
-- @param #string scriptPath Path to the script in OpenMW virtual filesystem.

-------------------------------------------------------------------------------
-- Whether a script with given path is attached to this object.
-- Can be called only from a global script.
-- @function [parent=#GameObject] hasScript
-- @param self
-- @param #string scriptPath Path to the script in OpenMW virtual filesystem.
-- @return #boolean

-------------------------------------------------------------------------------
-- Removes script that was attached by `addScript`
-- Can be called only from a global script.
-- @function [parent=#GameObject] removeScript
-- @param self
-- @param #string scriptPath Path to the script in OpenMW virtual filesystem.

-------------------------------------------------------------------------------
-- Moves object to given cell and position.
-- The effect is not immediate: the position will be updated only in the next
-- frame. Can be called only from a global script.
-- @function [parent=#GameObject] teleport
-- @param self
-- @param #string cellName Name of the cell to teleport into. For exteriors can be empty.
-- @param openmw.util#Vector3 position New position
-- @param openmw.util#Vector3 rotation New rotation. Optional argument. If missed, then the current rotation is used.


-------------------------------------------------------------------------------
-- List of GameObjects.
-- @type ObjectList
-- @extends #list<#GameObject>

-------------------------------------------------------------------------------
-- Create iterator.
-- @function [parent=#ObjectList] ipairs
-- @param self

-------------------------------------------------------------------------------
-- Filter list with a Query.
-- @function [parent=#ObjectList] select
-- @param self
-- @param openmw.query#Query query
-- @return #ObjectList


-------------------------------------------------------------------------------
-- A cell of the game world.
-- @type Cell
-- @field [parent=#Cell] #string name Name of the cell (can be empty string).
-- @field [parent=#Cell] #string region Region of the cell.
-- @field [parent=#Cell] #boolean isExterior Is it exterior or interior.
-- @field [parent=#Cell] #number gridX Index of the cell by X (only for exteriors).
-- @field [parent=#Cell] #number gridY Index of the cell by Y (only for exteriors).
-- @field [parent=#Cell] #boolean hasWater True if the cell contains water.

-------------------------------------------------------------------------------
-- Returns true either if the cell contains the object or if the cell is an exterior and the object is also in an exterior.
-- @function [parent=#Cell] isInSameSpace
-- @param self
-- @param #GameObject object
-- @return #boolean
-- @usage
-- if obj1.cell:isInSameSpace(obj2) then
--     dist = (obj1.position - obj2.position):length()
-- else
--     -- the distance can't be calculated because the coordinates are in different spaces
-- end

-------------------------------------------------------------------------------
-- Select objects from the cell with a Query (only in global scripts).
-- Returns an empty list if the cell is not loaded.
-- @function [parent=#Cell] selectObjects
-- @param self
-- @param openmw.query#Query query
-- @return #ObjectList


-------------------------------------------------------------------------------
-- Inventory of a player/NPC or a content of a container.
-- @type Inventory

-------------------------------------------------------------------------------
-- The number of items with given recordId.
-- @function [parent=#Inventory] countOf
-- @param self
-- @param #string recordId
-- @return #number

-------------------------------------------------------------------------------
-- Get all content of the inventory.
-- @function [parent=#Inventory] getAll
-- @param self
-- @return #ObjectList

-------------------------------------------------------------------------------
-- Get all potions.
-- @function [parent=#Inventory] getPotions
-- @param self
-- @return #ObjectList

-------------------------------------------------------------------------------
-- Get all apparatuses.
-- @function [parent=#Inventory] getApparatuses
-- @param self
-- @return #ObjectList

-------------------------------------------------------------------------------
-- Get all armor.
-- @function [parent=#Inventory] getArmor
-- @param self
-- @return #ObjectList

-------------------------------------------------------------------------------
-- Get all books.
-- @function [parent=#Inventory] getBooks
-- @param self
-- @return #ObjectList

-------------------------------------------------------------------------------
-- Get all clothing.
-- @function [parent=#Inventory] getClothing
-- @param self
-- @return #ObjectList

-------------------------------------------------------------------------------
-- Get all ingredients.
-- @function [parent=#Inventory] getIngredients
-- @param self
-- @return #ObjectList

-------------------------------------------------------------------------------
-- Get all lights.
-- @function [parent=#Inventory] getLights
-- @param self
-- @return #ObjectList

-------------------------------------------------------------------------------
-- Get all lockpicks.
-- @function [parent=#Inventory] getLockpicks
-- @param self
-- @return #ObjectList

-------------------------------------------------------------------------------
-- Get all miscellaneous items.
-- @function [parent=#Inventory] getMiscellaneous
-- @param self
-- @return #ObjectList

-------------------------------------------------------------------------------
-- Get all probes.
-- @function [parent=#Inventory] getProbes
-- @param self
-- @return #ObjectList

-------------------------------------------------------------------------------
-- Get all repair kits.
-- @function [parent=#Inventory] getRepairKits
-- @param self
-- @return #ObjectList

-------------------------------------------------------------------------------
-- Get all weapon.
-- @function [parent=#Inventory] getWeapons
-- @param self
-- @return #ObjectList


return nil

