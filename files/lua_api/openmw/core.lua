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
-- Simulation time in seconds.
-- The number of simulation seconds passed in the game world since starting a new game.
-- @function [parent=#core] getSimulationTime
-- @return #number

-------------------------------------------------------------------------------
-- The scale of simulation time relative to real time.
-- @function [parent=#core] getSimulationTimeScale
-- @return #number

-------------------------------------------------------------------------------
-- Game time in seconds.
-- @function [parent=#core] getGameTime
-- @return #number

-------------------------------------------------------------------------------
-- The scale of game time relative to simulation time.
-- @function [parent=#core] getGameTimeScale
-- @return #number

-------------------------------------------------------------------------------
-- Whether the world is paused (onUpdate doesn't work when the world is paused).
-- @function [parent=#core] isWorldPaused
-- @return #boolean

-------------------------------------------------------------------------------
-- Get a GMST setting from content files.
-- @function [parent=#core] getGMST
-- @param #string setting Setting name
-- @return #any

-------------------------------------------------------------------------------
-- Return i18n formatting function for the given context.
-- It is based on `i18n.lua` library.
-- Language files should be stored in VFS as `i18n/<ContextName>/<Lang>.lua`.
-- See https://github.com/kikito/i18n.lua for format details.
-- @function [parent=#core] i18n
-- @param #string context I18n context; recommended to use the name of the mod.
-- @return #function
-- @usage
-- -- DataFiles/i18n/MyMod/en.lua
-- return {
--     good_morning = 'Good morning.',
--     you_have_arrows = {
--       one = 'You have one arrow.',
--       other = 'You have %{count} arrows.',
--     },
-- }
-- @usage
-- -- DataFiles/i18n/MyMod/de.lua
-- return {
--     good_morning = "Guten Morgen.",
--     you_have_arrows = {
--       one = "Du hast ein Pfeil.",
--       other = "Du hast %{count} Pfeile.",
--     },
--     ["Hello %{name}!"] = "Hallo %{name}!",
-- }
-- @usage
-- local myMsg = core.i18n('MyMod')
-- print( myMsg('good_morning') )
-- print( myMsg('you_have_arrows', {count=5}) )
-- print( myMsg('Hello %{name}!', {name='World'}) )


-------------------------------------------------------------------------------
-- @type OBJECT_TYPE
-- @field #string Activator "Activator"
-- @field #string Armor "Armor"
-- @field #string Book "Book"
-- @field #string Clothing "Clothing"
-- @field #string Container "Container"
-- @field #string Creature "Creature"
-- @field #string Door "Door"
-- @field #string Ingredient "Ingredient"
-- @field #string Light "Light"
-- @field #string Miscellaneous "Miscellaneous"
-- @field #string NPC "NPC"
-- @field #string Player "Player"
-- @field #string Potion "Potion"
-- @field #string Static "Static"
-- @field #string Apparatus "Apparatus"
-- @field #string Lockpick "Lockpick"
-- @field #string Probe "Probe"
-- @field #string Repair "Repair"

-------------------------------------------------------------------------------
-- Possible `object.type` values.
-- @field [parent=#core] #OBJECT_TYPE OBJECT_TYPE


-------------------------------------------------------------------------------
-- @type EQUIPMENT_SLOT
-- @field #number Helmet
-- @field #number Cuirass
-- @field #number Greaves
-- @field #number LeftPauldron
-- @field #number RightPauldron
-- @field #number LeftGauntlet
-- @field #number RightGauntlet
-- @field #number Boots
-- @field #number Shirt
-- @field #number Pants
-- @field #number Skirt
-- @field #number Robe
-- @field #number LeftRing
-- @field #number RightRing
-- @field #number Amulet
-- @field #number Belt
-- @field #number CarriedRight
-- @field #number CarriedLeft
-- @field #number Ammunition

-------------------------------------------------------------------------------
-- Available equipment slots. Used in `object:getEquipment` and `object:setEquipment`.
-- @field [parent=#core] #EQUIPMENT_SLOT EQUIPMENT_SLOT


-------------------------------------------------------------------------------
-- Any object that exists in the game world and has a specific location.
-- Player, actors, items, and statics are game objects.
-- @type GameObject
-- @field openmw.util#Vector3 position Object position.
-- @field openmw.util#Vector3 rotation Object rotation (ZXY order).
-- @field #Cell cell The cell where the object currently is. During loading a game and for objects in an inventory or a container `cell` is nil.
-- @field #string type Type of the object (see @{openmw.core#OBJECT_TYPE}).
-- @field #number count Count (makes sense if holded in a container).
-- @field #string recordId Record ID.
-- @field #Inventory inventory Inventory of an Player/NPC or content of an container.
-- @field #boolean isTeleport `True` if it is a teleport door (only if `object.type` == "Door").
-- @field openmw.util#Vector3 destPosition Destination (only if a teleport door).
-- @field openmw.util#Vector3 destRotation Destination rotation (only if a teleport door).
-- @field #string destCell Destination cell (only if a teleport door).

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
-- Activate the object.
-- @function [parent=#GameObject] activateBy
-- @param self
-- @param #GameObject actor The actor who activates the object
-- @usage local self = require('openmw.self')
-- object:activateBy(self)

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
-- Filter list with a Query.
-- @function [parent=#ObjectList] select
-- @param self
-- @param openmw.query#Query query
-- @return #ObjectList


-------------------------------------------------------------------------------
-- A cell of the game world.
-- @type Cell
-- @field #string name Name of the cell (can be empty string).
-- @field #string region Region of the cell.
-- @field #boolean isExterior Is it exterior or interior.
-- @field #number gridX Index of the cell by X (only for exteriors).
-- @field #number gridY Index of the cell by Y (only for exteriors).
-- @field #boolean hasWater True if the cell contains water.

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
-- Get all items of given type from the inventory.
-- @function [parent=#Inventory] getAll
-- @param self
-- @param type (optional) items type (see @{openmw.core#OBJECT_TYPE})
-- @return #ObjectList
-- @usage local all = inventory:getAll()
-- local weapons = inventory:getAll(core.OBJECT_TYPE.Weapon)


return nil

