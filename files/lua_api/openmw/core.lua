---
-- `openmw.core` defines functions and types that are available in both local
-- and global scripts.
-- @module core
-- @usage local core = require('openmw.core')



---
-- The revision of OpenMW Lua API. It is an integer that is incremented every time the API is changed. See the actual value at the top of the page.
-- @field [parent=#core] #number API_REVISION

---
-- A read-only list of all @{#FactionRecord}s in the world database.
-- @field [parent=#core] #list<#FactionRecord> factions

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
-- Frame duration in seconds
-- @function [parent=#core] getRealFrameDuration
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
-- @{#ContentFiles}: functions working with the list of currently loaded content files.
-- @field [parent=#core] #ContentFiles contentFiles

---
-- Functions working with the list of currently loaded content files.
-- @type ContentFiles
-- @field #list<#string> list The current load order (list of content file names).

---
-- Return the index of a specific content file in the load order (or `nil` if there is no such content file).
-- @function [parent=#ContentFiles] indexOf
-- @param #string contentFile
-- @return #number

---
-- Check if the content file with given name present in the load order.
-- @function [parent=#ContentFiles] has
-- @param #string contentFile
-- @return #boolean

---
-- Construct FormId string from content file name and the index in the file.
-- In ESM3 games (e.g. Morrowind) FormIds are used to reference game objects.
-- In ESM4 games (e.g. Skyrim) FormIds are used both for game objects and as record ids.
-- @function [parent=#core] getFormId
-- @param #string contentFile
-- @param #number index
-- @return #string
-- @usage if obj.recordId == core.getFormId('Skyrim.esm', 0x4d7da) then ... end
-- @usage -- In ESM3 content files (e.g. Morrowind) ids are human-readable strings
-- obj.ownerFactionId = 'blades'
-- -- In ESM4 (e.g. Skyrim) ids should be constructed using `core.getFormId`:
-- obj.ownerFactionId = core.getFormId('Skyrim.esm', 0x72834)
-- @usage -- local scripts
-- local obj = nearby.getObjectByFormId(core.getFormId('Morrowind.esm', 128964))
-- @usage -- global scripts
-- local obj = world.getObjectByFormId(core.getFormId('Morrowind.esm', 128964))


---
-- Any object that exists in the game world and has a specific location.
-- Player, actors, items, and statics are game objects.
-- @type GameObject
-- @extends #userdata
-- @field #string id A unique id of this object (not record id), can be used as a key in a table.
-- @field #string contentFile Lower cased file name of the content file that defines this object; nil for dynamically created objects.
-- @field #boolean enabled Whether the object is enabled or disabled. Global scripts can set the value. Items in containers or inventories can't be disabled.
-- @field openmw.util#Vector3 position Object position.
-- @field #number scale Object scale.
-- @field openmw.util#Transform rotation Object rotation.
-- @field openmw.util#Vector3 startingPosition The object original position
-- @field openmw.util#Transform startingRotation The object original rotation
-- @field #string ownerRecordId NPC who owns the object (nil if missing). Global and self scripts can set the value.
-- @field #string ownerFactionId Faction who owns the object (nil if missing). Global and self scripts can set the value.
-- @field #number ownerFactionRank Rank required to be allowed to pick up the object (`nil` if any rank is allowed). Global and self scripts can set the value.
-- @field #Cell cell The cell where the object currently is. During loading a game and for objects in an inventory or a container `cell` is nil.
-- @field #GameObject parentContainer Container or actor that contains (or has in inventory) this object. It is nil if the object is in a cell.
-- @field #any type Type of the object (one of the tables from the package @{openmw.types#types}).
-- @field #number count Count (>1 means a stack of objects).
-- @field #string recordId Returns record ID of the object in lowercase.
-- @field #string globalVariable Global Variable associated with this object(read only).

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
-- Sets the object's scale.
-- Can be called only from a global script.
-- @function [parent=#GameObject] setScale
-- @param self
-- @param #number scale Scale desired in game.

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
-- @param #TeleportOptions options (optional) Either table @{#TeleportOptions} or @{openmw.util#Transform} rotation.

---
-- Either table with options or @{openmw.util#Vector3} rotation.
-- @type TeleportOptions
-- @field openmw.util#Transform rotation New rotation; if missing, then the current rotation is used.
-- @field #boolean onGround If true, adjust destination position to the ground.

---
-- Moves object into a container or an inventory. Enables if was disabled.
-- Can be called only from a global script.
-- @function [parent=#GameObject] moveInto
-- @param self
-- @param #any dest @{#Inventory} or @{#GameObject}
-- @usage item:moveInto(types.Actor.inventory(actor))
-- @usage item:moveInto(types.Container.content(container))
-- @usage item:moveInto(container)

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
-- @return #GameObject
-- @usage -- take 50 coins from `money` and put to the container `cont`
-- money:split(50):moveInto(types.Container.content(cont))

---
-- The axis aligned bounding box in local coordinates.
-- @function [parent=#GameObject] getBoundingBox
-- @param self
-- @return openmw.util#Box

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
-- @field #string worldSpaceId Id of the world space.
-- @field #boolean hasWater True if the cell contains water.
-- @field #number waterLevel The water level of the cell. (nil if cell has no water).
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
-- Get all objects of given type from the cell; Only available from global scripts.
-- @function [parent=#Cell] getAll
-- @param self
-- @param type (optional) object type (see @{openmw.types#types})
-- @return #ObjectList
-- @usage
-- local type = require('openmw.types')
-- local all = cell:getAll()
-- local weapons = cell:getAll(types.Weapon)

---
-- @type ActiveSpell
-- @field #string name The spell or item display name
-- @field #string id Record id of the spell or item used to cast the spell
-- @field #GameObject item The enchanted item used to cast the spell, or nil if the spell was not cast from an enchanted item. Note that if the spell was cast for a single-use enchantment such as a scroll, this will be nil.
-- @field #GameObject caster The caster object, or nil if the spell has no defined caster
-- @field #list<#ActiveSpellEffect> effects The active effects (@{#ActiveSpellEffect}) of this spell.

---
-- @type ActiveSpellEffect
-- @field #string affectedSkill Optional skill ID
-- @field #string affectedAttribute Optional attribute ID
-- @field #string id Magic effect id
-- @field #string name Localized name of the effect
-- @field #number magnitudeThisFrame The magnitude of the effect in the current frame. This will be a new random number between minMagnitude and maxMagnitude every frame. Or nil if the effect has no magnitude.
-- @field #number minMagnitude The minimum magnitude of this effect, or nil if the effect has no magnitude.
-- @field #number maxMagnitude The maximum magnitude of this effect, or nil if the effect has no magnitude.
-- @field #number duration Total duration in seconds of this spell effect, should not be confused with remaining duration. Or nil if the effect is not temporary.
-- @field #number durationLeft Remaining duration in seconds of this spell effect, or nil if the effect is not temporary.


--- Possible @{#EnchantmentType} values
-- @field [parent=#Magic] #EnchantmentType ENCHANTMENT_TYPE

--- `core.magic.ENCHANTMENT_TYPE`
-- @type EnchantmentType
-- @field #number CastOnce Enchantment can be cast once, destroying the enchanted item.
-- @field #number CastOnStrike Enchantment is cast on strike, if there is enough charge.
-- @field #number CastOnUse Enchantment is cast when used, if there is enough charge.
-- @field #number ConstantEffect Enchantment is always active when equipped.


---
-- @type Enchantment
-- @field #string id Enchantment id
-- @field #number type @{#EnchantmentType}
-- @field #number autocalcFlag If set, the casting cost should be computer rather than reading the cost field
-- @field #number cost
-- @field #number charge Charge capacity. Should not be confused with current charge.
-- @field #list<#MagicEffectWithParams> effects The effects (@{#MagicEffectWithParams}) of the enchantment
-- @usage -- Getting the enchantment of an arbitrary item, if it has one
-- local function getRecord(item)
--     if item.type and item.type.record then
--         return item.type.record(item)
--     end
--     return nil
-- end
-- local function getEnchantment(item)
--     local record = getRecord(item)
--     if record and record.enchant then
--         return core.magic.enchantments[record.enchant]
--     end
--     return nil
-- end


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
-- Will resolve the inventory, filling it with levelled items if applicable, making its contents permanent. Must be used in a global script.
-- @function [parent=#Inventory] resolve
-- @param self
-- @usage inventory:resolve()

---
-- Checks if the inventory has a resolved item list.
-- @function [parent=#Inventory] isResolved
-- @param self
-- @return #boolean
-- @usage inventory:isResolved()

---
-- Get all items with given recordId from the inventory.
-- @function [parent=#Inventory] findAll
-- @param self
-- @param #string recordId
-- @return #ObjectList
-- @usage for _, item in ipairs(inventory:findAll('common_shirt_01')) do ... end


--- @{#Magic}: spells and spell effects
-- @field [parent=#core] #Magic magic


--- Possible @{#SpellRange} values
-- @field [parent=#Magic] #SpellRange RANGE

--- `core.magic.RANGE`
-- @type SpellRange
-- @field #number Self Applied on self
-- @field #number Touch On touch
-- @field #number Target Ranged spell


--- Possible @{#MagicEffectId} values
-- @field [parent=#Magic] #MagicEffectId EFFECT_TYPE

--- `core.magic.EFFECT_TYPE`
-- @type MagicEffectId
-- @field #number WaterBreathing "waterbreathing"
-- @field #number SwiftSwim "swiftswim"
-- @field #number WaterWalking "waterwalking"
-- @field #number Shield "shield"
-- @field #number FireShield "fireshield"
-- @field #number LightningShield "lightningshield"
-- @field #number FrostShield "frostshield"
-- @field #number Burden "burden"
-- @field #number Feather "feather"
-- @field #number Jump "jump"
-- @field #number Levitate "levitate"
-- @field #number SlowFall "slowfall"
-- @field #number Lock "lock"
-- @field #number Open "open"
-- @field #number FireDamage "firedamage"
-- @field #number ShockDamage "shockdamage"
-- @field #number FrostDamage "frostdamage"
-- @field #number DrainAttribute "drainattribute"
-- @field #number DrainHealth "drainhealth"
-- @field #number DrainMagicka "drainmagicka"
-- @field #number DrainFatigue "drainfatigue"
-- @field #number DrainSkill "drainskill"
-- @field #number DamageAttribute "damageattribute"
-- @field #number DamageHealth "damagehealth"
-- @field #number DamageMagicka "damagemagicka"
-- @field #number DamageFatigue "damagefatigue"
-- @field #number DamageSkill "damageskill"
-- @field #number Poison "poison"
-- @field #number WeaknessToFire "weaknesstofire"
-- @field #number WeaknessToFrost "weaknesstofrost"
-- @field #number WeaknessToShock "weaknesstoshock"
-- @field #number WeaknessToMagicka "weaknesstomagicka"
-- @field #number WeaknessToCommonDisease "weaknesstocommondisease"
-- @field #number WeaknessToBlightDisease "weaknesstoblightdisease"
-- @field #number WeaknessToCorprusDisease "weaknesstocorprusdisease"
-- @field #number WeaknessToPoison "weaknesstopoison"
-- @field #number WeaknessToNormalWeapons "weaknesstonormalweapons"
-- @field #number DisintegrateWeapon "disintegrateweapon"
-- @field #number DisintegrateArmor "disintegratearmor"
-- @field #number Invisibility "invisibility"
-- @field #number Chameleon "chameleon"
-- @field #number Light "light"
-- @field #number Sanctuary "sanctuary"
-- @field #number NightEye "nighteye"
-- @field #number Charm "charm"
-- @field #number Paralyze "paralyze"
-- @field #number Silence "silence"
-- @field #number Blind "blind"
-- @field #number Sound "sound"
-- @field #number CalmHumanoid "calmhumanoid"
-- @field #number CalmCreature "calmcreature"
-- @field #number FrenzyHumanoid "frenzyhumanoid"
-- @field #number FrenzyCreature "frenzycreature"
-- @field #number DemoralizeHumanoid "demoralizehumanoid"
-- @field #number DemoralizeCreature "demoralizecreature"
-- @field #number RallyHumanoid "rallyhumanoid"
-- @field #number RallyCreature "rallycreature"
-- @field #number Dispel "dispel"
-- @field #number Soultrap "soultrap"
-- @field #number Telekinesis "telekinesis"
-- @field #number Mark "mark"
-- @field #number Recall "recall"
-- @field #number DivineIntervention "divineintervention"
-- @field #number AlmsiviIntervention "almsiviintervention"
-- @field #number DetectAnimal "detectanimal"
-- @field #number DetectEnchantment "detectenchantment"
-- @field #number DetectKey "detectkey"
-- @field #number SpellAbsorption "spellabsorption"
-- @field #number Reflect "reflect"
-- @field #number CureCommonDisease "curecommondisease"
-- @field #number CureBlightDisease "cureblightdisease"
-- @field #number CureCorprusDisease "curecorprusdisease"
-- @field #number CurePoison "curepoison"
-- @field #number CureParalyzation "cureparalyzation"
-- @field #number RestoreAttribute "restoreattribute"
-- @field #number RestoreHealth "restorehealth"
-- @field #number RestoreMagicka "restoremagicka"
-- @field #number RestoreFatigue "restorefatigue"
-- @field #number RestoreSkill "restoreskill"
-- @field #number FortifyAttribute "fortifyattribute"
-- @field #number FortifyHealth "fortifyhealth"
-- @field #number FortifyMagicka "fortifymagicka"
-- @field #number FortifyFatigue "fortifyfatigue"
-- @field #number FortifySkill "fortifyskill"
-- @field #number FortifyMaximumMagicka "fortifymaximummagicka"
-- @field #number AbsorbAttribute "absorbattribute"
-- @field #number AbsorbHealth "absorbhealth"
-- @field #number AbsorbMagicka "absorbmagicka"
-- @field #number AbsorbFatigue "absorbfatigue"
-- @field #number AbsorbSkill "absorbskill"
-- @field #number ResistFire "resistfire"
-- @field #number ResistFrost "resistfrost"
-- @field #number ResistShock "resistshock"
-- @field #number ResistMagicka "resistmagicka"
-- @field #number ResistCommonDisease "resistcommondisease"
-- @field #number ResistBlightDisease "resistblightdisease"
-- @field #number ResistCorprusDisease "resistcorprusdisease"
-- @field #number ResistPoison "resistpoison"
-- @field #number ResistNormalWeapons "resistnormalweapons"
-- @field #number ResistParalysis "resistparalysis"
-- @field #number RemoveCurse "removecurse"
-- @field #number TurnUndead "turnundead"
-- @field #number SummonScamp "summonscamp"
-- @field #number SummonClannfear "summonclannfear"
-- @field #number SummonDaedroth "summondaedroth"
-- @field #number SummonDremora "summondremora"
-- @field #number SummonAncestralGhost "summonancestralghost"
-- @field #number SummonSkeletalMinion "summonskeletalminion"
-- @field #number SummonBonewalker "summonbonewalker"
-- @field #number SummonGreaterBonewalker "summongreaterbonewalker"
-- @field #number SummonBonelord "summonbonelord"
-- @field #number SummonWingedTwilight "summonwingedtwilight"
-- @field #number SummonHunger "summonhunger"
-- @field #number SummonGoldenSaint "summongoldensaint"
-- @field #number SummonFlameAtronach "summonflameatronach"
-- @field #number SummonFrostAtronach "summonfrostatronach"
-- @field #number SummonStormAtronach "summonstormatronach"
-- @field #number FortifyAttack "fortifyattack"
-- @field #number CommandCreature "commandcreature"
-- @field #number CommandHumanoid "commandhumanoid"
-- @field #number BoundDagger "bounddagger"
-- @field #number BoundLongsword "boundlongsword"
-- @field #number BoundMace "boundmace"
-- @field #number BoundBattleAxe "boundbattleaxe"
-- @field #number BoundSpear "boundspear"
-- @field #number BoundLongbow "boundlongbow"
-- @field #number ExtraSpell "extraspell"
-- @field #number BoundCuirass "boundcuirass"
-- @field #number BoundHelm "boundhelm"
-- @field #number BoundBoots "boundboots"
-- @field #number BoundShield "boundshield"
-- @field #number BoundGloves "boundgloves"
-- @field #number Corprus "corprus"
-- @field #number Vampirism "vampirism"
-- @field #number SummonCenturionSphere "summoncenturionsphere"
-- @field #number SunDamage "sundamage"
-- @field #number StuntedMagicka "stuntedmagicka"
-- @field #number SummonFabricant "summonfabricant"
-- @field #number SummonWolf "summonwolf"
-- @field #number SummonBear "summonbear"
-- @field #number SummonBonewolf "summonbonewolf"
-- @field #number SummonCreature04 "summoncreature04"
-- @field #number SummonCreature05 "summoncreature05"

--- Possible @{#SpellType} values
-- @field [parent=#Magic] #SpellType SPELL_TYPE

--- `core.magic.SPELL_TYPE`
-- @type SpellType
-- @field #number Spell Normal spell, must be cast and costs mana
-- @field #number Ability Innate ability, always in effect
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

--- Map from @{#MagicEffectId} to @{#MagicEffect}
-- @field [parent=#Magic] #map<#number, #MagicEffect> effects
-- @usage -- Print all harmful effects
-- for _, effect in pairs(core.magic.effects) do
--     if effect.harmful then
--         print(effect.name)
--     end
-- end
-- @usage -- Look up the record of a specific effect and print its icon
-- local mgef = core.magic.effects[core.magic.EFFECT_TYPE.Reflect]
-- print('Reflect Icon: '..tostring(mgef.icon))

--- List of all @{#Enchantment}s.
-- @field [parent=#Magic] #list<#Enchantment> enchantments
-- @usage local enchantment = core.magic.enchantments['marara's boon']  -- get by id
-- @usage local enchantment = core.magic.enchantments[1]  -- get by index
-- @usage -- Print all enchantments with constant effect
-- for _, ench in pairs(core.magic.enchantments) do
--     if ench.type == core.magic.ENCHANTMENT_TYPE.ConstantEffect then
--         print(ench.id)
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
-- @field #string id Effect ID
-- @field #string icon Effect Icon Path
-- @field #string name Localized name of the effect
-- @field #string school Skill ID that is this effect's school
-- @field #number baseCost
-- @field openmw.util#Color color
-- @field #boolean harmful
-- @field #boolean continuousVfx Whether the magic effect's vfx should loop or not
-- @field #string particle Identifier of the particle texture
-- @field #string castingStatic Identifier of the vfx static used for casting
-- @field #string hitStatic Identifier of the vfx static used on hit
-- @field #string areaStatic Identifier of the vfx static used for AOE spells

---
-- @type MagicEffectWithParams
-- @field #MagicEffect effect @{#MagicEffect}
-- @field #string affectedSkill Optional skill ID
-- @field #string affectedAttribute Optional attribute ID
-- @field #number range
-- @field #number area
-- @field #number magnitudeMin
-- @field #number magnitudeMax
-- @field #number duration

---
-- @type ActiveEffect
-- Magic effect that is currently active on an actor.
-- @field #string affectedSkill Optional skill ID
-- @field #string affectedAttribute Optional attribute ID
-- @field #string id Effect id string
-- @field #string name Localized name of the effect
-- @field #number magnitude current magnitude of the effect. Will be set to 0 when effect is removed or expires.
-- @field #number magnitudeBase
-- @field #number magnitudeModifier

--- @{#Sound}: Sounds and Speech
-- @field [parent=#core] #Sound sound

---
-- Checks if sound system is enabled (any functions to play sounds are no-ops when it is disabled).
-- It can not be enabled or disabled during runtime.
-- @function [parent=#Sound] isEnabled
-- @return #boolean
-- @usage local enabled = core.sound.isEnabled();

---
-- Play a 3D sound, attached to object
-- @function [parent=#Sound] playSound3d
-- @param #string soundId ID of Sound record to play
-- @param #GameObject object Object to which we attach the sound
-- @param #table options An optional table with additional optional arguments. Can contain:
--
--   * `timeOffset` - a floating point number >= 0, to some time (in second) from beginning of sound file (default: 0);
--   * `volume` - a floating point number >= 0, to set a sound volume (default: 1);
--   * `pitch` - a floating point number >= 0, to set a sound pitch (default: 1);
--   * `loop` - a boolean, to set if sound should be repeated when it ends (default: false);
-- @usage local params = {
--    timeOffset=0.1
--    volume=0.3,
--    loop=false,
--    pitch=1.0
-- };
-- core.sound.playSound3d("shock bolt", object, params)

---
-- Play a 3D sound file, attached to object
-- @function [parent=#Sound] playSoundFile3d
-- @param #string fileName Path to sound file in VFS
-- @param #GameObject object Object to which we attach the sound
-- @param #table options An optional table with additional optional arguments. Can contain:
--
--   * `timeOffset` - a floating point number >= 0, to some time (in second) from beginning of sound file (default: 0);
--   * `volume` - a floating point number >= 0, to set a sound volume (default: 1);
--   * `pitch` - a floating point number >= 0, to set a sound pitch (default: 1);
--   * `loop` - a boolean, to set if sound should be repeated when it ends (default: false);
-- @usage local params = {
--    timeOffset=0.1
--    volume=0.3,
--    loop=false,
--    pitch=1.0
-- };
-- core.sound.playSoundFile3d("Sound\\test.mp3", object, params)

---
-- Stop a 3D sound, attached to object
-- @function [parent=#Sound] stopSound3d
-- @param #string soundId ID of Sound record to stop
-- @param #GameObject object Object on which we want to stop sound
-- @usage core.sound.stopSound("shock bolt", object);

---
-- Stop a 3D sound file, attached to object
-- @function [parent=#Sound] stopSoundFile3d
-- @param #string fileName Path to sound file in VFS
-- @param #GameObject object Object on which we want to stop sound
-- @usage core.sound.stopSoundFile("Sound\\test.mp3", object);

---
-- Check if sound is playing on given object
-- @function [parent=#Sound] isSoundPlaying
-- @param #string soundId ID of Sound record to check
-- @param #GameObject object Object on which we want to check sound
-- @return #boolean
-- @usage local isPlaying = core.sound.isSoundPlaying("shock bolt", object);

---
-- Check if sound file is playing on given object
-- @function [parent=#Sound] isSoundFilePlaying
-- @param #string fileName Path to sound file in VFS
-- @param #GameObject object Object on which we want to check sound
-- @return #boolean
-- @usage local isPlaying = core.sound.isSoundFilePlaying("Sound\\test.mp3", object);

---
-- Play an animated voiceover. Has two overloads:
--
--   * With an "object" argument: play sound for given object, with speaking animation if possible
--   * Without an "object" argument: play sound globally, without object
-- @function [parent=#Sound] say
-- @param #string fileName Path to sound file in VFS
-- @param #GameObject object Object on which we want to play an animated voiceover (optional)
-- @param #string text Subtitle text (optional)
-- @usage -- play voiceover for object and print messagebox
-- core.sound.say("Sound\\Vo\\Misc\\voice.mp3", object, "Subtitle text")
-- @usage -- play voiceover globally and print messagebox
-- core.sound.say("Sound\\Vo\\Misc\\voice.mp3", "Subtitle text")
-- @usage -- play voiceover for object without messagebox
-- core.sound.say("Sound\\Vo\\Misc\\voice.mp3", object)
-- @usage -- play voiceover globally without messagebox
-- core.sound.say("Sound\\Vo\\Misc\\voice.mp3")

---
-- Stop animated voiceover
-- @function [parent=#Sound] stopSay
-- @param #string fileName Path to sound file in VFS
-- @param #GameObject object Object on which we want to stop an animated voiceover (optional)
-- @usage -- stop voice for given object
-- core.sound.stopSay(object);
-- @usage -- stop global voice
-- core.sound.stopSay();

---
-- Check if animated voiceover is playing
-- @function [parent=#Sound] isSayActive
-- @param #GameObject object Object on which we want to check an animated voiceover (optional)
-- @return #boolean
-- @usage -- check voice for given object
-- local isActive = isSayActive(object);
-- @usage -- check global voice
-- local isActive = isSayActive();

---
-- @type SoundRecord
-- @field #string id Sound id
-- @field #string fileName Normalized path to sound file in VFS
-- @field #number volume Raw sound volume, from 0 to 255
-- @field #number minRange Raw minimal range value, from 0 to 255
-- @field #number maxRange Raw maximal range value, from 0 to 255

--- List of all @{#SoundRecord}s.
-- @field [parent=#Sound] #list<#SoundRecord> sounds
-- @usage local sound = core.sound.sounds['Ashstorm']  -- get by id
-- @usage local sound = core.sound.sounds[1]  -- get by index
-- @usage -- Print all sound files paths
-- for _, sound in pairs(core.sound.sounds) do
--     print(sound.fileName)
-- end

--- @{#Stats}: stats
-- @field [parent=#core] #Stats stats


--- @{#Attribute} functions
-- @field [parent=#Stats] #Attribute Attribute

--- `core.stats.Attribute`
-- @type Attribute
-- @field #list<#AttributeRecord> records A read-only list of all @{#AttributeRecord}s in the world database.

---
-- Returns a read-only @{#AttributeRecord}
-- @function [parent=#Attribute] record
-- @param #string recordId
-- @return #AttributeRecord

--- @{#Skill} functions
-- @field [parent=#Stats] #Skill Skill

--- `core.stats.Skill`
-- @type Skill
-- @field #list<#SkillRecord> records A read-only list of all @{#SkillRecord}s in the world database.

---
-- Returns a read-only @{#SkillRecord}
-- @function [parent=#Skill] record
-- @param #string recordId
-- @return #SkillRecord

---
-- @type AttributeRecord
-- @field #string id Record id
-- @field #string name Human-readable name
-- @field #string description Human-readable description
-- @field #string icon VFS path to the icon

---
-- @type SkillRecord
-- @field #string id Record id
-- @field #string name Human-readable name
-- @field #string description Human-readable description
-- @field #string icon VFS path to the icon
-- @field #string specialization Skill specialization. Either combat, magic, or stealth.
-- @field #MagicSchoolData school Optional magic school
-- @field #string attribute The id of the skill's governing attribute

---
-- @type MagicSchoolData
-- @field #string name Human-readable name
-- @field #string areaSound VFS path to the area sound
-- @field #string boltSound VFS path to the bolt sound
-- @field #string castSound VFS path to the cast sound
-- @field #string failureSound VFS path to the failure sound
-- @field #string hitSound VFS path to the hit sound

---
-- Faction data record
-- @type FactionRecord
-- @field #string id Faction id
-- @field #string name Faction name
-- @field #list<#FactionRank> ranks A read-only list containing data for all ranks in the faction, in order.
-- @field #map<#string, #number> reactions A read-only map containing reactions of other factions to this faction.
-- @field #list<#string> attributes A read-only list containing IDs of attributes to advance ranks in the faction.
-- @field #list<#string> skills A read-only list containing IDs of skills to advance ranks in the faction.

---
-- Faction rank data record
-- @type FactionRank
-- @field #string name Faction name Rank display name
-- @field #list<#number> attributeValues Attributes values required to get this rank.
-- @field #number primarySkillValue Primary skill value required to get this rank.
-- @field #number favouredSkillValue Secondary skill value required to get this rank.
-- @field #number factionReaction Reaction of faction members if player is in this faction.

--- @{#VFX}: Visual effects
-- @field [parent=#core] #VFX vfx

---
-- Spawn a VFX at the given location in the world
-- @function [parent=#VFX] spawn
-- @param #any static openmw.core#StaticRecord or #string ID
-- @param openmw.util#Vector3 location
-- @param #table options optional table of parameters. Can contain:
--
--   * `mwMagicVfx` - Boolean that if true causes the textureOverride parameter to only affect nodes with the Nif::RC_NiTexturingProperty property set. (default: true).
--   * `particleTextureOverride` - Name of a particle texture that should override this effect's default texture. (default: "")
--   * `scale` - A number that scales the size of the vfx (Default: 1)
--
-- @usage -- Spawn a sanctuary effect near the player
-- local effect = core.magic.effects[core.magic.EFFECT_TYPE.Sanctuary]
-- pos = self.position + util.vector3(0, 100, 0)
-- core.vfx.spawn(effect.castingStatic, pos)
--

return nil
