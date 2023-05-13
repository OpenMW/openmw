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
-- Real time in seconds; starting point is not fixed (can be time since last reboot), use only for measuring intervals. For Unix time use `os.time()`.
-- @function [parent=#core] getRealTime
-- @return #number

---
-- Get a GMST setting from content files.
-- @function [parent=#core] getGMST
-- @param #string setting Setting name
-- @return #any

---
-- Return l10n formatting function for the given context.
-- Localisation files (containing the message names and translations) should be stored in
-- VFS as files of the form `l10n/<ContextName>/<Locale>.yaml`.
--
-- See [Localisation](../modding/localisation.html) for details of the localisation file structure.
--
-- When calling the l10n formatting function, if no localisation can be found for any of the requested locales then
-- the message key will be returned instead (and formatted, if possible).
-- This makes it possible to use the source strings as message identifiers.
--
-- If you do not use the source string as a message identifier you should instead make certain to include
-- a fallback locale with a complete set of messages.
--
-- @function [parent=#core] l10n
-- @param #string context l10n context; recommended to use the name of the mod.
--                This must match the <ContextName> directory in the VFS which stores the localisation files.
-- @param #string fallbackLocale The source locale containing the default messages
--                               If omitted defaults to "en".
-- @return #function
-- @usage
-- # DataFiles/l10n/MyMod/en.yaml
-- good_morning: 'Good morning.'
-- you_have_arrows: |-
--   {count, plural,
--     one {You have one arrow.}
--     other {You have {count} arrows.}
--   }
-- @usage
-- # DataFiles/l10n/MyMod/de.yaml
-- good_morning: "Guten Morgen."
-- you_have_arrows: |-
--   {count, plural,
--     one {Du hast ein Pfeil.}
--     other {Du hast {count} Pfeile.}
--   }
-- "Hello {name}!": "Hallo {name}!"
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
-- @extends #userdata
-- @field #string id A unique id of this object (not record id), can be used as a key in a table.
-- @field #boolean enabled Whether the object is enabled or disabled. Global scripts can set the value. Items in containers or inventories can't be disabled.
-- @field openmw.util#Vector3 position Object position.
-- @field openmw.util#Vector3 rotation Object rotation (ZXY order).
-- @field #string ownerRecordId NPC who owns the object (nil if missing). Global and self scripts can set the value.
-- @field #string ownerFactionId Faction who owns the object (nil if missing). Global and self scripts can set the value.
-- @field #number ownerFactionRank Rank required to be allowed to pick up the object. Global and self scripts can set the value.
-- @field #Cell cell The cell where the object currently is. During loading a game and for objects in an inventory or a container `cell` is nil.
-- @field #any type Type of the object (one of the tables from the package @{openmw.types#types}).
-- @field #number count Count (>1 means a stack of objects).
-- @field #string recordId Returns record ID of the object in lowercase.

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
-- @param #table initData (optional) Initialization data to be passed to onInit. If missed then Lua initialization data from content files will be used (if exists for this script).

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
-- Can be called only from a global script.
-- The effect is not immediate: the position will be updated only in the next
-- frame. Can be called only from a global script. Enables object if it was disabled.
-- Can be used to move objects from an inventory or a container to the world.
-- @function [parent=#GameObject] teleport
-- @param self
-- @param #any cellOrName A cell to define the destination worldspace; can be either #Cell, or cell name, or an empty string (empty string means the default exterior worldspace).
-- If the worldspace has multiple cells (i.e. an exterior), the destination cell is calculated using `position`.
-- @param openmw.util#Vector3 position New position.
-- @param #TeleportOptions options (optional) Either table @{#TeleportOptions} or @{openmw.util#Vector3} rotation.

---
-- Either table with options or @{openmw.util#Vector3} rotation.
-- @type TeleportOptions
-- @field openmw.util#Vector3 rotation New rotation; if missing, then the current rotation is used.
-- @field #boolean onGround If true, adjust destination position to the ground.

---
-- Moves object into a container or an inventory. Enables if was disabled.
-- Can be called only from a global script.
-- @function [parent=#GameObject] moveInto
-- @param self
-- @param #Inventory dest
-- @usage item:moveInto(types.Actor.inventory(actor))

---
-- Removes an object or reduces a stack of objects.
-- Can be called only from a global script.
-- @function [parent=#GameObject] remove
-- @param self
-- @param #number count (optional) the number of items to remove (if not specified then the whole stack)

---
-- Splits a stack of items. Original stack is reduced by `count`. Returns a new stack with `count` items.
-- Can be called only from a global script.
-- @function [parent=#GameObject] split
-- @param self
-- @param #number count The number of items to return.
-- @usage -- take 50 coins from `money` and put to the container `cont`
-- money:split(50):moveInto(types.Container.content(cont))


---
-- List of GameObjects. Implements [iterables#List](iterables.html#List) of #GameObject
-- @type ObjectList
-- @list <#GameObject>


---
-- A cell of the game world.
-- @type Cell
-- @field #string name Name of the cell (can be empty string).
-- @field #string region Region of the cell.
-- @field #boolean isExterior Whether the cell is an exterior cell. "Exterior" means grid of cells where the player can seamless walk from one cell to another without teleports. QuasiExterior (interior with sky) is not an exterior.
-- @field #boolean isQuasiExterior (DEPRECATED, use `hasTag("QuasiExterior")`) Whether the cell is a quasi exterior (like interior but with the sky and the wheather).
-- @field #number gridX Index of the cell by X (only for exteriors).
-- @field #number gridY Index of the cell by Y (only for exteriors).
-- @field #boolean hasWater True if the cell contains water.
-- @field #boolean hasSky True if in this cell sky should be rendered.

---
-- Returns true if the cell has given tag.
-- @function [parent=#Cell] hasTag
-- @param self
-- @param #string tag One of "QuasiExterior", "NoSleep".
-- @return #boolean

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
-- local types = require('openmw.types')
-- local self = require('openmw.self')
-- local playerInventory = types.Actor.inventory(self.object)
-- local all = playerInventory:getAll()
-- local weapons = playerInventory:getAll(types.Weapon)

---
-- Get first item with given recordId from the inventory. Returns nil if not found.
-- @function [parent=#Inventory] find
-- @param self
-- @param #string recordId
-- @return #GameObject
-- @usage inventory:find('gold_001')

---
-- Get all items with given recordId from the inventory.
-- @function [parent=#Inventory] findAll
-- @param self
-- @param #string recordId
-- @return #ObjectList
-- @usage for _, item in ipairs(inventory:findAll('common_shirt_01')) do ... end


--- Possible @{#ATTRIBUTE} values
-- @field [parent=#core] #ATTRIBUTE ATTRIBUTE

--- `core.ATTRIBUTE`
-- @type ATTRIBUTE
-- @field #string Strength "strength"
-- @field #string Intelligence "intelligence"
-- @field #string Willpower "willpower"
-- @field #string Agility "agility"
-- @field #string Speed "speed"
-- @field #string Endurance "endurance"
-- @field #string Personality "personality"
-- @field #string Luck "luck"


--- Possible @{#SKILL} values
-- @field [parent=#core] #SKILL SKILL

--- `core.SKILL`
-- @type SKILL
-- @field #string Acrobatics "acrobatics"
-- @field #string Alchemy "alchemy"
-- @field #string Alteration "alteration"
-- @field #string Armorer "armorer"
-- @field #string Athletics "athletics"
-- @field #string Axe "axe"
-- @field #string Block "block"
-- @field #string BluntWeapon "bluntweapon"
-- @field #string Conjuration "conjuration"
-- @field #string Destruction "destruction"
-- @field #string Enchant "enchant"
-- @field #string HandToHand "handtohand"
-- @field #string HeavyArmor "heavyarmor"
-- @field #string Illusion "illusion"
-- @field #string LightArmor "lightarmor"
-- @field #string LongBlade "longblade"
-- @field #string Marksman "marksman"
-- @field #string MediumArmor "mediumarmor"
-- @field #string Mercantile "mercantile"
-- @field #string Mysticism "mysticism"
-- @field #string Restoration "restoration"
-- @field #string Security "security"
-- @field #string ShortBlade "shortblade"
-- @field #string Sneak "sneak"
-- @field #string Spear "spear"
-- @field #string Speechcraft "speechcraft"
-- @field #string Unarmored "unarmored"


--- @{#Magic}: spells and spell effects
-- @field [parent=#core] #Magic magic


--- Possible @{#SpellRange} values
-- @field [parent=#Magic] #SpellRange RANGE

--- `core.magic.RANGE`
-- @type SpellRange
-- @field #number Self Applied on self
-- @field #number Touch On touch
-- @field #number Target Ranged spell


--- Possible @{#MagicSchool} values
-- @field [parent=#Magic] #MagicSchool SCHOOL

--- `core.magic.SCHOOL`
-- @type MagicSchool
-- @field #number Alteration Alteration
-- @field #number Conjuration Conjuration
-- @field #number Destruction Destruction
-- @field #number Illusion Illusion
-- @field #number Mysticism Mysticism
-- @field #number Restoration Restoration


--- Possible @{#SpellType} values
-- @field [parent=#Magic] #SpellType SPELL_TYPE

--- `core.magic.SPELL_TYPE`
-- @type SpellType
-- @field #number Spell Normal spell, must be cast and costs mana
-- @field #number Ability Inert ability, always in effect
-- @field #number Blight Blight disease
-- @field #number Disease Common disease
-- @field #number Curse Curse
-- @field #number Power Power, can be used once a day


--- List of all @{#Spell}s.
-- @field [parent=#Magic] #list<#Spell> spells
-- @usage local spell = core.magic.spells['thunder fist']  -- get by id
-- @usage local spell = core.magic.spells[1]  -- get by index
-- @usage -- Print all powers
-- for _, spell in pairs(core.magic.spells) do
--     if spell.types == core.magic.SPELL_TYPE.Power then
--         print(spell.name)
--     end
-- end

--- Map from effectId to @{#SpellEffect}
-- @field [parent=#Magic] #map<#number, #MagicEffect> effects
-- @usage -- Print all harmful effects
-- for _, effect in pairs(core.magic.effects) do
--     if effect.harmful then
--         print(effect.name)
--     end
-- end

---
-- @type Spell
-- @field #string id Spell id
-- @field #string name Spell name
-- @field #number type @{#SpellType}
-- @field #number cost
-- @field #list<#MagicEffectWithParams> effects The effects (@{#MagicEffectWithParams}) of the spell

---
-- @type MagicEffect
-- @field #number id
-- @field #string name
-- @field #number school @{#MagicSchool}
-- @field #number baseCost
-- @field openmw.util#Color color
-- @field #boolean harmful

---
-- @type MagicEffectWithParams
-- @field #MagicEffect effect @{#MagicEffect}
-- @field #any affectedSkill @{#SKILL} or nil
-- @field #any affectedAttribute @{#ATTRIBUTE} or nil
-- @field #number range
-- @field #number area
-- @field #number magnitudeMin
-- @field #number magnitudeMax


return nil
