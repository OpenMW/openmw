---
-- Allows for manipulation of the data loaded from content files while the game is first started.
-- Records can be created and deleted using this package as if a content file had done so.
-- @context load
-- @module content
-- @usage local content = require('openmw.content')


--- @{openmw.core#SpellRange}: Magic ranges
-- @field [parent=#content] openmw.core#SpellRange RANGE


--- @{#ActivatorContent}: Activator manipulation.
-- @field [parent=#content] #ActivatorContent activators

---
-- A mutable list of all @{openmw.types#ActivatorRecord}s.
-- @field [parent=#ActivatorContent] #list<openmw.types#ActivatorRecord> records
-- @usage
-- content.activators.records.MyActivator = { mwscript = 'float', model = 'meshes/w/w_chitin_arrow.nif', name = 'Quest marker' }

--- @{#DoorContent}: Door manipulation.
-- @field [parent=#content] #DoorContent doors

---
-- A mutable list of all @{openmw.types#DoorRecord}s.
-- @field [parent=#DoorContent] #list<openmw.types#DoorRecord> records
-- @usage
-- content.doors.records.MyDoor = { template = content.doors.records['door_dwrv_double00'], mwscript = 'blockedDoor', name = 'Overly Heavy Dwemer Door' }

--- @{#EnchantmentContent}: Enchantment manipulation.
-- @field [parent=#content] #EnchantmentContent enchantments

--- @{openmw.core#EnchantmentType}: Enchantment types
-- @field [parent=#EnchantmentContent] openmw.core#EnchantmentType TYPE

---
-- A mutable list of all @{openmw.core#Enchantment}s.
-- @field [parent=#EnchantmentContent] #list<openmw.core#Enchantment> records
-- @usage
-- content.enchantments.records.MyEnchantment = { type = content.enchantments.TYPE.CastOnUse, charge = 1, cost = 1, effects = { { id = 'FortifySkill', affectedSkill = 'enchant', duration = 5, magnitudeMin = 50, magnitudeMax = 100 } } }

--- @{#GMSTContent}: GMST manipulation.
-- @field [parent=#content] #GMSTContent gameSettings

---
-- Returns a table containing all fallback values defined in `openmw.cfg`.
-- @function [parent=#GMSTContent] getFallbacks
-- @return #table

---
-- A mutable list of all game settings.
-- @field [parent=#GMSTContent] #map<#string, #any> records
-- @usage
-- content.gameSettings.records.fJumpAcrobaticsBase = 1024

--- @{#GlobalContent}: Global variable manipulation.
-- @field [parent=#content] #GlobalContent globals

---
-- A mutable list of all global mwscript variables.
-- @field [parent=#GlobalContent] #map<#string, #number> records
-- @usage
-- content.globals.records.MyVariable = 42

--- @{#MiscContent}: Misc manipulation.
-- @field [parent=#content] #MiscContent miscs

---
-- A mutable list of all @{openmw.types#MiscellaneousRecord}s.
-- @field [parent=#MiscContent] #list<openmw.types#MiscellaneousRecord> records
-- @usage
-- content.miscs.records.MyMisc = { template = content.miscs.records['gold_001'], mwscript = 'BILL_MarksSpiritSummon', weight = 5 }

--- @{#PotionContent}: Potion manipulation.
-- @field [parent=#content] #PotionContent potions

---
-- A mutable list of all @{openmw.types#PotionRecord}s.
-- @field [parent=#PotionContent] #list<openmw.types#PotionRecord> records
-- @usage
-- content.potions.records.MyPotion = { template = content.potions.records['p_dispel_s'], name = 'Too Strong', effects = { { id = 'FireDamage', duration = 10, range = content.RANGE.Self, magnitudeMin = 100 } } }

--- @{#SpellContent}: Spell manipulation.
-- @field [parent=#content] #SpellContent spells

--- @{openmw.core#SpellType}: Spell types
-- @field [parent=#SpellContent] openmw.core#SpellType TYPE

---
-- A mutable list of all @{openmw.core#Spell}s.
-- @field [parent=#SpellContent] #list<openmw.core#Spell> records
-- @usage
-- content.spells.records.MySpell = { name = 'Enchantment?', type = content.spells.TYPE.Spell, cost = 1000, starterSpellFlag = true, isAutocalc = true, effects = { { id = 'FortifyAttribute', affectedAttribute = 'intelligence', duration = 5, magnitudeMin = 5, magnitudeMax = 10 } } }

--- @{#StaticContent}: Static manipulation.
-- @field [parent=#content] #StaticContent statics

---
-- A mutable list of all @{openmw.types#StaticRecord}s.
-- @field [parent=#StaticContent] #list<openmw.types#StaticRecord> records
-- @usage
-- content.statics.records.MyStatic = { model = 'meshes/b/B_N_Wood Elf_M_Head_02.nif' }

--- @{#SoundContent}: Sound manipulation.
-- @field [parent=#content] #SoundContent sounds

---
-- A mutable list of all @{openmw.core#SoundRecord}s.
-- @field [parent=#SoundContent] #list<openmw.core#SoundRecord> records
-- @usage
-- content.sounds.records.MySound = { template = content.sounds.records['MournDayAmb'], fileName = 'sound/fx/funny.wav' }

return nil
