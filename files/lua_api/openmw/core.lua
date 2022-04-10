---
-- `openmw.core` defines functions and types that are available in both local
-- and global scripts.
-- @module core
-- @usage local core = require('openmw.core')



---
-- The revision of OpenMW Lua API. It is an integer that is incremented every time the API is changed.
-- @field [parent=#core] #number API_REVISION

---
-- Terminates the game and quits to the OS. Should be used only for testing purposes.
-- @function [parent=#core] quit

---
-- Send an event to global scripts.
-- @function [parent=#core] sendGlobalEvent
-- @param #string eventName
-- @param eventData

---
-- Simulation time in seconds.
-- The number of simulation seconds passed in the game world since starting a new game.
-- @function [parent=#core] getSimulationTime
-- @return #number

---
-- The scale of simulation time relative to real time.
-- @function [parent=#core] getSimulationTimeScale
-- @return #number

---
-- Game time in seconds.
-- @function [parent=#core] getGameTime
-- @return #number

---
-- The scale of game time relative to simulation time.
-- @function [parent=#core] getGameTimeScale
-- @return #number

---
-- Whether the world is paused (onUpdate doesn't work when the world is paused).
-- @function [parent=#core] isWorldPaused
-- @return #boolean

---
-- Get a GMST setting from content files.
-- @function [parent=#core] getGMST
-- @param #string setting Setting name
-- @return #any

---
-- Return l10n formatting function for the given context.
-- Language files should be stored in VFS as `l10n/<ContextName>/<Locale>.yaml`.
--
-- Locales usually have the form {lang}_{COUNTRY},
-- where {lang} is a lowercase two-letter language code and {COUNTRY} is an uppercase
-- two-letter country code. Capitalization and the separator must have exactly
-- this format for language files to be recognized, but when users request a
-- locale they do not need to match capitalization and can use hyphens instead of
-- underscores.
--
-- Locales may also contain variants and keywords. See https://unicode-org.github.io/icu/userguide/locale/#language-code
-- for full details.
--
-- Messages have the form of ICU MessageFormat strings.
-- See https://unicode-org.github.io/icu/userguide/format_parse/messages/
-- for a guide to MessageFormat, and see 
-- https://unicode-org.github.io/icu-docs/apidoc/released/icu4c/classicu_1_1MessageFormat.html
-- for full details of the MessageFormat syntax.
-- @function [parent=#core] l10n
-- @param #string context l10n context; recommended to use the name of the mod.
-- @param #string fallbackLocale The source locale containing the default messages
--                               If omitted defaults to "en"
-- @return #function
-- @usage
-- # DataFiles/l10n/MyMod/en.yaml
-- good_morning: 'Good morning.'
--
-- you_have_arrows: |- {count, plural,
--     one {You have one arrow.}
--     other {You have {count} arrows.}
--   }
-- @usage
-- # DataFiles/l10n/MyMod/de.yaml
-- good_morning: "Guten Morgen."
-- you_have_arrows: |- {count, plural,
--     one {Du hast ein Pfeil.}
--     other {Du hast {count} Pfeile.}
--   }
-- "Hello {name}!": "Hallo {name}!"
-- @usage
-- # DataFiles/l10n/AdvancedExample/en.yaml
-- # More complicated patterns
-- # select rules can be used to match arbitrary string arguments
-- # The default keyword other must always be provided
-- pc_must_come: {PCGender, select,
--     male {He is}
--     female {She is}
--     other {They are}
--   } coming with us.
-- # Numbers have various formatting options
-- quest_completion: "The quest is {done, number, percent} complete.",
-- # E.g. "You came in 4th place"
-- ordinal: "You came in {num, ordinal} place."
-- # E.g. "There is one thing", "There are one hundred things"
-- spellout: "There {num, plural, one{is {num, spellout} thing} other{are {num, spellout} things}}."
-- numbers: "{int} and {double, number, integer} are integers, but {double} is a double"
-- # Numbers can be formatted with custom patterns
-- # See https://unicode-org.github.io/icu/userguide/format_parse/numbers/skeletons.html#syntax
-- rounding: "{value, number, :: .00}"
-- @usage
-- -- Usage in Lua
-- local myMsg = core.l10n('MyMod', 'en')
-- print( myMsg('good_morning') )
-- print( myMsg('you_have_arrows', {count=5}) )
-- print( myMsg('Hello {name}!', {name='World'}) )


---
-- Any object that exists in the game world and has a specific location.
-- Player, actors, items, and statics are game objects.
-- @type GameObject
-- @field openmw.util#Vector3 position Object position.
-- @field openmw.util#Vector3 rotation Object rotation (ZXY order).
-- @field #Cell cell The cell where the object currently is. During loading a game and for objects in an inventory or a container `cell` is nil.
-- @field #table type Type of the object (one of the tables from the package @{openmw.types#types}).
-- @field #number count Count (makes sense if stored in a container).
-- @field #string recordId Record ID.

---
-- Does the object still exist and is available.
-- Returns true if the object exists and loaded, and false otherwise. If false, then every
-- access to the object will raise an error.
-- @function [parent=#GameObject] isValid
-- @param self
-- @return #boolean

---
-- Send local event to the object.
-- @function [parent=#GameObject] sendEvent
-- @param self
-- @param #string eventName
-- @param eventData

---
-- Activate the object.
-- @function [parent=#GameObject] activateBy
-- @param self
-- @param #GameObject actor The actor who activates the object
-- @usage local self = require('openmw.self')
-- object:activateBy(self)

---
-- Add new local script to the object.
-- Can be called only from a global script. Script should be specified in a content
-- file (omwgame/omwaddon/omwscripts) with a CUSTOM flag. Scripts can not be attached to Statics.
-- @function [parent=#GameObject] addScript
-- @param self
-- @param #string scriptPath Path to the script in OpenMW virtual filesystem.

---
-- Whether a script with given path is attached to this object.
-- Can be called only from a global script.
-- @function [parent=#GameObject] hasScript
-- @param self
-- @param #string scriptPath Path to the script in OpenMW virtual filesystem.
-- @return #boolean

---
-- Removes script that was attached by `addScript`
-- Can be called only from a global script.
-- @function [parent=#GameObject] removeScript
-- @param self
-- @param #string scriptPath Path to the script in OpenMW virtual filesystem.

---
-- Moves object to given cell and position.
-- The effect is not immediate: the position will be updated only in the next
-- frame. Can be called only from a global script.
-- @function [parent=#GameObject] teleport
-- @param self
-- @param #string cellName Name of the cell to teleport into. For exteriors can be empty.
-- @param openmw.util#Vector3 position New position
-- @param openmw.util#Vector3 rotation New rotation. Optional argument. If missed, then the current rotation is used.


---
-- List of GameObjects. Implements [iterables#List](iterables.html#List) of #GameObject
-- @type ObjectList
-- @list <#GameObject>


---
-- A cell of the game world.
-- @type Cell
-- @field #string name Name of the cell (can be empty string).
-- @field #string region Region of the cell.
-- @field #boolean isExterior Is it exterior or interior.
-- @field #number gridX Index of the cell by X (only for exteriors).
-- @field #number gridY Index of the cell by Y (only for exteriors).
-- @field #boolean hasWater True if the cell contains water.

---
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

---
-- Get all objects of given type from the cell.
-- @function [parent=#Cell] getAll
-- @param self
-- @param type (optional) object type (see @{openmw.types#types})
-- @return #ObjectList
-- @usage
-- local type = require('openmw.types')
-- local all = cell:getAll()
-- local weapons = cell:getAll(types.Weapon)


---
-- Inventory of a player/NPC or a content of a container.
-- @type Inventory

---
-- The number of items with given recordId.
-- @function [parent=#Inventory] countOf
-- @param self
-- @param #string recordId
-- @return #number

---
-- Get all items of given type from the inventory.
-- @function [parent=#Inventory] getAll
-- @param self
-- @param type (optional) items type (see @{openmw.types#types})
-- @return #ObjectList
-- @usage
-- local type = require('openmw.types')
-- local all = inventory:getAll()
-- local weapons = inventory:getAll(types.Weapon)


return nil

