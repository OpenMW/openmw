---
-- `openmw.types` defines functions for specific types of game objects.
-- @module types
-- @usage local types = require('openmw.types')

--- Common @{#Actor} functions for Creature, NPC, and Player.
-- @field [parent=#types] #Actor Actor

--- Common functions for Creature, NPC, and Player.
-- @type Actor

---
-- Agent bounds to be used for pathfinding functions.
-- @function [parent=#Actor] getPathfindingAgentBounds
-- @param openmw.core#GameObject actor
-- @return #table with `shapeType` and `halfExtents`

---
-- Whether the object is an actor.
-- @function [parent=#Actor] objectIsInstance
-- @param openmw.core#GameObject object
-- @return #boolean

---
-- Actor inventory.
-- @function [parent=#Actor] inventory
-- @param openmw.core#GameObject actor
-- @return openmw.core#Inventory

---
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

---
-- Available @{#EQUIPMENT_SLOT} values. Used in `Actor.equipment(obj)` and `Actor.setEquipment(obj, eqp)`.
-- @field [parent=#Actor] #EQUIPMENT_SLOT EQUIPMENT_SLOT

---
-- @type STANCE
-- @field #number Nothing Default stance
-- @field #number Weapon Weapon stance
-- @field #number Spell Magic stance

--- @{#STANCE}
-- @field [parent=#Actor] #STANCE STANCE

---
-- Returns true if the object is an actor and is able to move. For dead, paralyzed,
-- or knocked down actors it returns false.
-- @function [parent=#Actor] canMove
-- @param openmw.core#GameObject object
-- @return #boolean

---
-- Speed of running. For dead actors it still returns a positive value.
-- @function [parent=#Actor] getRunSpeed
-- @param openmw.core#GameObject actor
-- @return #number

---
-- Speed of walking. For dead actors it still returns a positive value.
-- @function [parent=#Actor] getWalkSpeed
-- @param openmw.core#GameObject actor
-- @return #number

---
-- Current speed.
-- @function [parent=#Actor] getCurrentSpeed
-- @param openmw.core#GameObject actor
-- @return #number

---
-- Is the actor standing on ground. Can be called only from a local script.
-- @function [parent=#Actor] isOnGround
-- @param openmw.core#GameObject actor
-- @return #boolean

---
-- Is the actor in water. Can be called only from a local script.
-- @function [parent=#Actor] isSwimming
-- @param openmw.core#GameObject actor
-- @return #boolean

---
-- Returns the current stance (whether a weapon/spell is readied), see the list of @{#STANCE} values.
-- @function [parent=#Actor] getStance
-- @param openmw.core#GameObject actor
-- @return #number

---
-- Sets the current stance (whether a weapon/spell is readied), see the list of @{#STANCE} values.
-- Can be used only in local scripts on self.
-- @function [parent=#Actor] setStance
-- @param openmw.core#GameObject actor
-- @param #number stance

---
-- Returns `true` if the item is equipped on the actor.
-- @function [parent=#Actor] hasEquipped
-- @param openmw.core#GameObject actor
-- @param openmw.core#GameObject item
-- @return #boolean

---
-- Map from values of @{#EQUIPMENT_SLOT} to items @{openmw.core#GameObject}s
-- @type EquipmentTable
-- @map <#number, openmw.core#GameObject>

---
-- Get equipment.
-- Has two overloads:
-- 1) With a single argument: returns a table `slot` -> @{openmw.core#GameObject} of currently equipped items.
-- See @{#EQUIPMENT_SLOT}. Returns empty table if the actor doesn't have
-- equipment slots.
-- 2) With two arguments: returns an item equipped to the given slot.
-- @function [parent=#Actor] getEquipment
-- @param openmw.core#GameObject actor
-- @param #number slot Optional number of the equipment slot
-- @return #EquipmentTable, openmw.core#GameObject

---
-- Set equipment.
-- Keys in the table are equipment slots (see @{#EQUIPMENT_SLOT}). Each
-- value can be either a `GameObject` or recordId. Raises an error if
-- the actor doesn't have equipment slots and table is not empty. Can be
-- used only in local scripts and only on self.
-- @function [parent=#Actor] setEquipment
-- @param openmw.core#GameObject actor
-- @param #EquipmentTable equipment
-- @usage local self = require('openmw.self')
-- local Actor = require('openmw.types').Actor
-- Actor.setEquipment(self, {}) -- unequip all

---
-- Get currently selected spell
-- @function [parent=#Actor] getSelectedSpell
-- @param openmw.core#GameObject actor
-- @return openmw.core#Spell, nil

---
-- Set selected spell
-- @function [parent=#Actor] setSelectedSpell
-- @param openmw.core#GameObject actor
-- @param openmw.core#Spell spell Spell (can be nil)

---
-- Get currently selected enchanted item
-- @function [parent=#Actor] getSelectedEnchantedItem
-- @param openmw.core#GameObject actor
-- @return openmw.core#GameObject, nil enchanted item or nil

---
-- Set currently selected enchanted item, equipping it if applicable
-- @function [parent=#Actor] setSelectedEnchantedItem
-- @param openmw.core#GameObject actor
-- @param openmw.core#GameObject item enchanted item

---
-- Return the active magic effects (@{#ActorActiveEffects}) currently affecting the given actor.
-- @function [parent=#Actor] activeEffects
-- @param openmw.core#GameObject actor
-- @return #ActorActiveEffects

--- Read-only list of effects currently affecting the actor.
-- @type ActorActiveEffects
-- @usage -- print active effects
-- for _, effect in pairs(Actor.activeEffects(self)) do
--     print('Active Effect: '..effect.id..', attribute='..tostring(effect.affectedAttribute)..', skill='..tostring(effect.affectedSkill)..', magnitude='..tostring(effect.magnitude))
-- end
-- @usage -- Check for a specific effect
-- local effect = Actor.activeEffects(self):getEffect(core.magic.EFFECT_TYPE.Telekinesis)
-- if effect then
--     print(effect.id..', attribute='..tostring(effect.affectedAttribute)..', skill='..tostring(effect.affectedSkill)..', magnitude='..tostring(effect.magnitude))
-- else
--     print('No Telekinesis effect')
-- end
-- @usage -- Check for a specific effect targeting a specific attribute.
-- local effect = Actor.activeEffects(self):getEffect(core.magic.EFFECT_TYPE.FortifyAttribute, core.ATTRIBUTE.Luck)
-- if effect then
--     print(effect.id..', attribute='..tostring(effect.affectedAttribute)..', skill='..tostring(effect.affectedSkill)..', magnitude='..tostring(effect.magnitude))
-- else
--     print('No Fortify Luck effect')
-- end

---
-- Get a specific active effect on the actor.
-- @function [parent=#ActorActiveEffects] getEffect
-- @param self
-- @param #string effectId effect ID
-- @param #string extraParam Optional skill or attribute ID
-- @return openmw.core#ActiveEffect if such an effect is active, nil otherwise

---
-- Completely removes the active effect from the actor.
-- This removes both the effects incurred by active spells and effect added by console, mwscript, or luascript.
-- @function [parent=#ActorActiveEffects] remove
-- @param self
-- @param #string effectId effect ID
-- @param #string extraParam Optional skill or attribute ID

---
-- Permanently modifies the magnitude of an active effect to be exactly equal to the provided value. This adds the effect to the list of active effects if not already active.
-- Note that although the modification is permanent, the magnitude will not stay equal to the value if any active spells with this effects are added/removed.
-- @function [parent=#ActorActiveEffects] set
-- @param self
-- @param #number value
-- @param #string effectId effect ID
-- @param #string extraParam Optional skill or attribute ID

---
-- Permanently modifies the magnitude of an active effect by increasing it by the provided value. This adds the effect to the list of active effects if not already active.
-- @function [parent=#ActorActiveEffects] modify
-- @param self
-- @param #number value
-- @param #string effectId effect ID
-- @param #string extraParam Optional skill or attribute ID

---
-- Return the active spells (@{#ActorActiveSpells}) currently affecting the given actor.
-- @function [parent=#Actor] activeSpells
-- @param openmw.core#GameObject actor
-- @return #ActorActiveSpells

--- Read-only list of spells currently affecting the actor.
-- @type ActorActiveSpells
-- @usage -- print active spells
-- for _, spell in pairs(Actor.activeSpells(self)) do
--     print('Active Spell: '..tostring(spell))
-- end
-- @usage -- Check for a specific spell
-- if Actor.activeSpells(self):isSpellActive('bound longbow') then
--     print('Player has bound longbow')
-- else
--     print('Player does not have bound longbow')
-- end

---
-- Get whether a specific spell is active on the actor.
-- @function [parent=#ActorActiveSpells] isSpellActive
-- @param self
-- @param #any spellOrId @{openmw.core#Spell} or string spell id
-- @return true if spell is active, false otherwise

---
-- Remove the given spell and all its effects from the given actor's active spells.
-- @function [parent=#ActorActiveSpells] remove
-- @param self
-- @param #any spellOrId @{openmw.core#Spell} or string spell id

---
-- Return the spells (@{#ActorSpells}) of the given actor.
-- @function [parent=#Actor] spells
-- @param openmw.core#GameObject actor
-- @return #ActorSpells

--- List of spells with additional functions add/remove/clear (modification are allowed only in global scripts or on self).
-- @type ActorSpells
-- @usage -- print available spells
-- local mySpells = types.Actor.spells(self)
-- for _, spell in pairs(mySpells) do print(spell.id) end
-- @usage -- print available spells (equivalent)
-- local mySpells = types.Actor.spells(self)
-- for i = 1, #mySpells do print(mySpells[i].id) end
-- @usage -- add ALL spells that exist in the world
-- local mySpells = types.Actor.spells(self)
-- for _, spell in pairs(core.magic.spells) do
--     if spell.type == core.magic.SPELL_TYPE.Spell then
--         mySpells:add(spell)
--     end
-- end
-- @usage -- add specific spell
-- types.Actor.spells(self):add('thunder fist')
-- @usage -- check specific spell
-- local mySpells = types.Actor.spells(self)
-- if mySpells['thunder fist'] then print('I have thunder fist') end

---
-- Add spell (only in global scripts or on self).
-- @function [parent=#ActorSpells] add
-- @param self
-- @param #any spellOrId @{openmw.core#Spell} or string spell id

---
-- Remove spell (only in global scripts or on self).
-- @function [parent=#ActorSpells] remove
-- @param self
-- @param #any spellOrId @{openmw.core#Spell} or string spell id

---
-- Remove all spells (only in global scripts or on self).
-- @function [parent=#ActorSpells] clear
-- @param self

---
-- @type LevelStat
-- @field #number current The actor's current level.
-- @field #number progress The NPC's level progress (read-only.)

---
-- @type DynamicStat
-- @field #number base
-- @field #number current
-- @field #number modifier

---
-- @type AttributeStat
-- @field #number base The actor's base attribute value.
-- @field #number damage The amount the attribute has been damaged.
-- @field #number modified The actor's current attribute value (read-only.)
-- @field #number modifier The attribute's modifier.

---
-- @type SkillStat
-- @field #number base The NPC's base skill value.
-- @field #number damage The amount the skill has been damaged.
-- @field #number modified The NPC's current skill value (read-only.)
-- @field #number modifier The skill's modifier.
-- @field #number progress [0-1] The NPC's skill progress.

---
-- @type DynamicStats

---
-- Health (returns @{#DynamicStat})
-- @function [parent=#DynamicStats] health
-- @param openmw.core#GameObject actor
-- @return #DynamicStat

---
-- Magicka (returns @{#DynamicStat})
-- @function [parent=#DynamicStats] magicka
-- @param openmw.core#GameObject actor
-- @return #DynamicStat

---
-- Fatigue (returns @{#DynamicStat})
-- @function [parent=#DynamicStats] fatigue
-- @param openmw.core#GameObject actor
-- @return #DynamicStat

---
-- @type AttributeStats

---
-- Strength (returns @{#AttributeStat})
-- @function [parent=#AttributeStats] strength
-- @param openmw.core#GameObject actor
-- @return #AttributeStat

---
-- Intelligence (returns @{#AttributeStat})
-- @function [parent=#AttributeStats] intelligence
-- @param openmw.core#GameObject actor
-- @return #AttributeStat

---
-- Willpower (returns @{#AttributeStat})
-- @function [parent=#AttributeStats] willpower
-- @param openmw.core#GameObject actor
-- @return #AttributeStat

---
-- Agility (returns @{#AttributeStat})
-- @function [parent=#AttributeStats] agility
-- @param openmw.core#GameObject actor
-- @return #AttributeStat

---
-- Speed (returns @{#AttributeStat})
-- @function [parent=#AttributeStats] speed
-- @param openmw.core#GameObject actor
-- @return #AttributeStat

---
-- Endurance (returns @{#AttributeStat})
-- @function [parent=#AttributeStats] endurance
-- @param openmw.core#GameObject actor
-- @return #AttributeStat

---
-- Personality (returns @{#AttributeStat})
-- @function [parent=#AttributeStats] personality
-- @param openmw.core#GameObject actor
-- @return #AttributeStat

---
-- Luck (returns @{#AttributeStat})
-- @function [parent=#AttributeStats] luck
-- @param openmw.core#GameObject actor
-- @return #AttributeStat

---
-- @type SkillStats

---
-- Block (returns @{#SkillStat})
-- @function [parent=#SkillStats] block
-- @param openmw.core#GameObject actor
-- @return #SkillStat

---
-- Armorer (returns @{#SkillStat})
-- @function [parent=#SkillStats] armorer
-- @param openmw.core#GameObject actor
-- @return #SkillStat

---
-- Medium Armor (returns @{#SkillStat})
-- @function [parent=#SkillStats] mediumarmor
-- @param openmw.core#GameObject actor
-- @return #SkillStat

---
-- Heavy Armor (returns @{#SkillStat})
-- @function [parent=#SkillStats] heavyarmor
-- @param openmw.core#GameObject actor
-- @return #SkillStat

---
-- Blunt Weapon (returns @{#SkillStat})
-- @function [parent=#SkillStats] bluntweapon
-- @param openmw.core#GameObject actor
-- @return #SkillStat

---
-- Long Blade (returns @{#SkillStat})
-- @function [parent=#SkillStats] longblade
-- @param openmw.core#GameObject actor
-- @return #SkillStat

---
-- Axe (returns @{#SkillStat})
-- @function [parent=#SkillStats] axe
-- @param openmw.core#GameObject actor
-- @return #SkillStat

---
-- Spear (returns @{#SkillStat})
-- @function [parent=#SkillStats] spear
-- @param openmw.core#GameObject actor
-- @return #SkillStat

---
-- Athletics (returns @{#SkillStat})
-- @function [parent=#SkillStats] athletics
-- @param openmw.core#GameObject actor
-- @return #SkillStat

---
-- Enchant (returns @{#SkillStat})
-- @function [parent=#SkillStats] enchant
-- @param openmw.core#GameObject actor
-- @return #SkillStat

---
-- Destruction (returns @{#SkillStat})
-- @function [parent=#SkillStats] destruction
-- @param openmw.core#GameObject actor
-- @return #SkillStat

---
-- Alteration (returns @{#SkillStat})
-- @function [parent=#SkillStats] alteration
-- @param openmw.core#GameObject actor
-- @return #SkillStat

---
-- Illusion (returns @{#SkillStat})
-- @function [parent=#SkillStats] illusion
-- @param openmw.core#GameObject actor
-- @return #SkillStat

---
-- Conjuration (returns @{#SkillStat})
-- @function [parent=#SkillStats] conjuration
-- @param openmw.core#GameObject actor
-- @return #SkillStat

---
-- Mysticism (returns @{#SkillStat})
-- @function [parent=#SkillStats] mysticism
-- @param openmw.core#GameObject actor
-- @return #SkillStat

---
-- Restoration (returns @{#SkillStat})
-- @function [parent=#SkillStats] restoration
-- @param openmw.core#GameObject actor
-- @return #SkillStat

---
-- Alchemy (returns @{#SkillStat})
-- @function [parent=#SkillStats] alchemy
-- @param openmw.core#GameObject actor
-- @return #SkillStat

---
-- Unarmored (returns @{#SkillStat})
-- @function [parent=#SkillStats] unarmored
-- @param openmw.core#GameObject actor
-- @return #SkillStat

---
-- Security (returns @{#SkillStat})
-- @function [parent=#SkillStats] security
-- @param openmw.core#GameObject actor
-- @return #SkillStat

---
-- Sneak (returns @{#SkillStat})
-- @function [parent=#SkillStats] sneak
-- @param openmw.core#GameObject actor
-- @return #SkillStat

---
-- Acrobatics (returns @{#SkillStat})
-- @function [parent=#SkillStats] acrobatics
-- @param openmw.core#GameObject actor
-- @return #SkillStat

---
-- Light Armor (returns @{#SkillStat})
-- @function [parent=#SkillStats] lightarmor
-- @param openmw.core#GameObject actor
-- @return #SkillStat

---
-- Short Blade (returns @{#SkillStat})
-- @function [parent=#SkillStats] shortblade
-- @param openmw.core#GameObject actor
-- @return #SkillStat

---
-- Marksman (returns @{#SkillStat})
-- @function [parent=#SkillStats] marksman
-- @param openmw.core#GameObject actor
-- @return #SkillStat

---
-- Mercantile (returns @{#SkillStat})
-- @function [parent=#SkillStats] mercantile
-- @param openmw.core#GameObject actor
-- @return #SkillStat

---
-- Speechcraft (returns @{#SkillStat})
-- @function [parent=#SkillStats] speechcraft
-- @param openmw.core#GameObject actor
-- @return #SkillStat

---
-- Hand To Hand (returns @{#SkillStat})
-- @function [parent=#SkillStats] handtohand
-- @param openmw.core#GameObject actor
-- @return #SkillStat

---
-- @type ActorStats
-- @field #DynamicStats dynamic
-- @field #AttributeStats attributes

---
-- Level (returns @{#LevelStat})
-- @function [parent=#ActorStats] level
-- @param openmw.core#GameObject actor
-- @return #LevelStat

--- The actor's stats.
-- @field [parent=#Actor] #ActorStats stats

---
-- @type NpcStats
-- @extends ActorStats
-- @field #SkillStats skills


--------------------------------------------------------------------------------
-- @{#Item} functions (all items that can be placed to an inventory or container)
-- @field [parent=#types] #Item Item

--- Functions for items that can be placed to an inventory or container
-- @type Item

---
-- Whether the object is an item.
-- @function [parent=#Item] objectIsInstance
-- @param openmw.core#GameObject object
-- @return #boolean

---
-- Get this item's current enchantment charge.
-- @function [parent=#Item] getEnchantmentCharge
-- @param openmw.core#GameObject item
-- @return #number The charge remaining. -1 if the enchantment has never been used, implying the charge is full. Unenchanted items will always return a value of -1.

---
-- Set this item's enchantment charge.
-- @function [parent=#Item] setEnchantmentCharge
-- @param openmw.core#GameObject item
-- @param #number charge

---
-- Whether the object is supposed to be carriable. It is true for all items except
-- lights without the Carry flag. Non-carriable lights can still be put into
-- an inventory with an explicit `object:moveInto` call.
-- @function [parent=#Item] isCarriable
-- @param openmw.core#GameObject object
-- @return #boolean


--------------------------------------------------------------------------------
-- @{#Creature} functions
-- @field [parent=#types] #Creature Creature

---
-- @type Creature
-- @extends #Actor
-- @field #Actor baseType @{#Actor}
-- @field #list<#CreatureRecord> records A read-only list of all @{#CreatureRecord}s in the world database.

---
-- Whether the object is a creature.
-- @function [parent=#Creature] objectIsInstance
-- @param openmw.core#GameObject object
-- @return #boolean

--- Creature.TYPE
-- @type CreatureTYPE
-- @field #number Creatures
-- @field #number Daedra
-- @field #number Undead
-- @field #number Humanoid

--- @{#CreatureTYPE}
-- @field [parent=#Creature] #CreatureTYPE TYPE

---
-- Returns the read-only @{#CreatureRecord} of a creature
-- @function [parent=#Creature] record
-- @param #any objectOrRecordId
-- @return #CreatureRecord

---
-- @type CreatureRecord
-- @field #string id The record ID of the creature
-- @field #string name
-- @field #string baseCreature Record id of a base creature, which was modified to create this one
-- @field #string model VFS path to the creature's model
-- @field #string mwscript
-- @field #number soulValue The soul value of the creature record
-- @field #number type The @{#Creature.TYPE} of the creature


--- @{#NPC} functions
-- @field [parent=#types] #NPC NPC

---
-- @type NPC
-- @extends #Actor
-- @field #Actor baseType @{#Actor}
-- @field [parent=#NPC] #NpcStats stats
-- @field #list<#NpcRecord> records A read-only list of all @{#NpcRecord}s in the world database.

---
-- Whether the object is an NPC or a Player.
-- @function [parent=#NPC] objectIsInstance
-- @param openmw.core#GameObject object
-- @return #boolean

---
-- Whether the NPC or player is in the werewolf form at the moment.
-- @function [parent=#NPC] isWerewolf
-- @param openmw.core#GameObject actor
-- @return #boolean

---
-- Returns the read-only @{#NpcRecord} of an NPC
-- @function [parent=#NPC] record
-- @param #any objectOrRecordId
-- @return #NpcRecord

---
-- @type NpcRecord
-- @field #string id The record ID of the NPC
-- @field #string name
-- @field #string race
-- @field #string class Name of the NPC's class (e. g. Acrobat)
-- @field #string mwscript MWScript that is attached to this NPC
-- @field #string hair Path to the hair body part model
-- @field #string head Path to the head body part model
-- @field #bool isMale The gender setting of the NPC


--------------------------------------------------------------------------------
-- @{#Player} functions
-- @field [parent=#types] #Player Player

---
-- @type Player
-- @extends #NPC
-- @field #NPC baseType @{#NPC}

---
-- Whether the object is a player.
-- @function [parent=#Player] objectIsInstance
-- @param openmw.core#GameObject object
-- @return #boolean

---
-- Returns the bounty or crime level of the player
-- @function [parent=#Player] getCrimeLevel
-- @param openmw.core#GameObject player
-- @return #number

---
-- Returns a list containing quests @{#PlayerQuest} for the specified player, indexed by quest ID.
-- @function [parent=#Player] quests
-- @param openmw.core#GameObject player
-- @return #list<#PlayerQuest>
-- @usage -- Get stage of a specific quest
-- stage = types.Player.quests(player)["ms_fargothring"].stage
-- @usage -- Get names of all started quests
-- for x, quest in pairs(types.Player.quests(player)) do print (quest.name) end
-- @usage -- Start a new quest, add it to the player's quest list but don't add any journal entries
-- types.Player.quests(player)["ms_fargothring"].stage = 0

---
-- @type PlayerQuest
-- @field #string id The quest id.
-- @field #number stage The quest stage (global and player scripts can change it). Changing the stage starts the quest if it wasn't started.
-- @field #bool started Whether the quest is started.
-- @field #bool finished Whether the quest is finished (global and player scripts can change it).

---
-- Sets the quest stage for the given quest, on the given player, and adds the entry to the journal, if there is an entry at the specified stage. Can only be used in global or player scripts.
-- @function [parent=#PlayerQuest] addJournalEntry
-- @param self
-- @param #number stage Quest stage
-- @param openmw.core#GameObject actor (optional) The actor who is the source of the journal entry, it may be used in journal entries with variables such as `%name(The speaker's name)` or `%race(The speaker's race)`.


--------------------------------------------------------------------------------
-- @{#Armor} functions
-- @field [parent=#types] #Armor Armor

---
-- @type Armor
-- @extends #Item
-- @field #Item baseType @{#Item}
-- @field #list<#ArmorRecord> records A read-only list of all @{#ArmorRecord}s in the world database.

---
-- Whether the object is an Armor.
-- @function [parent=#Armor] objectIsInstance
-- @param openmw.core#GameObject object
-- @return #boolean

--- Armor.TYPE
-- @type ArmorTYPE
-- @field #number Helmet
-- @field #number Cuirass
-- @field #number LPauldron
-- @field #number RPauldron
-- @field #number Greaves
-- @field #number Boots
-- @field #number LGauntlet
-- @field #number RGauntlet
-- @field #number Shield
-- @field #number LBracer
-- @field #number RBracer

--- @{#ArmorTYPE}
-- @field [parent=#Armor] #ArmorTYPE TYPE

---
-- Returns the read-only @{#ArmorRecord} of an Armor
-- @function [parent=#Armor] record
-- @param #any objectOrRecordId
-- @return #ArmorRecord

---
-- @type ArmorRecord
-- @field #string id Record id
-- @field #string name Human-readable name
-- @field #string model VFS path to the model
-- @field #string mwscript MWScript on this armor (can be empty)
-- @field #string icon VFS path to the icon
-- @field #string enchant The enchantment ID of this armor (can be empty)
-- @field #number weight
-- @field #number value
-- @field #number type See @{#Armor.TYPE}
-- @field #number health
-- @field #number baseArmor The base armor rating of this armor
-- @field #number enchantCapacity

---
-- Creates a @{#ArmorRecord} without adding it to the world database.
-- Use @{openmw_world#(world).createRecord} to add the record to the world.
-- @function [parent=#Armor] createRecordDraft
-- @param #ArmorRecord armor A Lua table with the fields of a ArmorRecord.
-- @return #ArmorRecord A strongly typed Armor record.


--- @{#Book} functions
-- @field [parent=#types] #Book Book

---
-- @type Book
-- @extends #Item
-- @field #Item baseType @{#Item}
-- @field #list<#BookRecord> records A read-only list of all @{#BookRecord}s in the world database.

---
-- Whether the object is a Book.
-- @function [parent=#Book] objectIsInstance
-- @param openmw.core#GameObject object
-- @return #boolean

--- Book.SKILL
-- @type BookSKILL
-- @field #string acrobatics "acrobatics"
-- @field #string alchemy "alchemy"
-- @field #string alteration "alteration"
-- @field #string armorer "armorer"
-- @field #string athletics "athletics"
-- @field #string axe "axe"
-- @field #string block "block"
-- @field #string bluntWeapon "bluntweapon"
-- @field #string conjuration "conjuration"
-- @field #string destruction "destruction"
-- @field #string enchant "enchant"
-- @field #string handToHand "handtohand"
-- @field #string heavyArmor "heavyarmor"
-- @field #string illusion "illusion"
-- @field #string lightArmor "lightarmor"
-- @field #string longBlade "longblade"
-- @field #string marksman "marksman"
-- @field #string mediumArmor "mediumarmor"
-- @field #string mercantile "mercantile"
-- @field #string mysticism "mysticism"
-- @field #string restoration "restoration"
-- @field #string security "security"
-- @field #string shortBlade "shortblade"
-- @field #string sneak "sneak"
-- @field #string spear "spear"
-- @field #string speechcraft "speechcraft"
-- @field #string unarmored "unarmored"

--- DEPRECATED, use @{openmw.core#SKILL}
-- @field [parent=#Book] #BookSKILL SKILL

---
-- Returns the read-only @{#BookRecord} of a book
-- @function [parent=#Book] record
-- @param #any objectOrRecordId
-- @return #BookRecord

---
-- @type BookRecord
-- @field #string id The record ID of the book
-- @field #string name Name of the book
-- @field #string model VFS path to the model
-- @field #string mwscript MWScript on this book (can be empty)
-- @field #string icon VFS path to the icon
-- @field #string enchant The enchantment ID of this book (can be empty)
-- @field #string text The text content of the book
-- @field #number weight
-- @field #number value
-- @field #string skill The skill that this book teaches. See @{openmw.core#SKILL}
-- @field #boolean isScroll
-- @field #number enchantCapacity

---
-- Creates a @{#BookRecord} without adding it to the world database.
-- Use @{openmw_world#(world).createRecord} to add the record to the world.
-- @function [parent=#Book] createRecordDraft
-- @param #BookRecord book A Lua table with the fields of a BookRecord.
-- @return #BookRecord A strongly typed Book record.

--- @{#Clothing} functions
-- @field [parent=#types] #Clothing Clothing

---
-- @type Clothing
-- @extends #Item
-- @field #Item baseType @{#Item}
-- @field #list<#ClothingRecord> records A read-only list of all @{#ClothingRecord}s in the world database.

---
-- Whether the object is a Clothing.
-- @function [parent=#Clothing] objectIsInstance
-- @param openmw.core#GameObject object
-- @return #boolean

--- Clothing.TYPE
-- @type ClothingTYPE
-- @field #number Amulet
-- @field #number Belt
-- @field #number LGlove
-- @field #number Pants
-- @field #number RGlove
-- @field #number Ring
-- @field #number Robe
-- @field #number Shirt
-- @field #number Shoes
-- @field #number Skirt

--- @{#ClothingTYPE}
-- @field [parent=#Clothing] #ClothingTYPE TYPE

---
-- Returns the read-only @{#ClothingRecord} of a Clothing
-- @function [parent=#Clothing] record
-- @param #any objectOrRecordId
-- @return #ClothingRecord

---
-- Creates a @{#ClothingRecord} without adding it to the world database.
-- Use @{openmw_world#(world).createRecord} to add the record to the world.
-- @function [parent=#Clothing] createRecordDraft
-- @param #ClothingRecord clothing A Lua table with the fields of a ClothingRecord.
-- @return #ClothingRecord A strongly typed clothing record.

---
-- @type ClothingRecord
-- @field #string id Record id
-- @field #string name Name of the clothing
-- @field #string model VFS path to the model
-- @field #string mwscript MWScript on this clothing (can be empty)
-- @field #string icon VFS path to the icon
-- @field #string enchant The enchantment ID of this clothing (can be empty)
-- @field #number weight
-- @field #number value
-- @field #number type See @{#Clothing.TYPE}
-- @field #number enchantCapacity




--- @{#Ingredient} functions
-- @field [parent=#types] #Ingredient Ingredient

---
-- @type Ingredient
-- @extends #Item
-- @field #Item baseType @{#Item}
-- @field #list<#IngredientRecord> records A read-only list of all @{#IngredientRecord}s in the world database.

---
-- Whether the object is an Ingredient.
-- @function [parent=#Ingredient] objectIsInstance
-- @param openmw.core#GameObject object
-- @return #boolean

---
-- Returns the read-only @{#IngredientRecord} of a Ingredient
-- @function [parent=#Ingredient] record
-- @param #any objectOrRecordId
-- @return #IngredientRecord

---
-- @type IngredientRecord
-- @field #string id Record id
-- @field #string name Human-readable name
-- @field #string model VFS path to the model
-- @field #string mwscript MWScript on this potion (can be empty)
-- @field #string icon VFS path to the icon
-- @field #number weight
-- @field #number value
-- @field #list<openmw.core#MagicEffectWithParams> effects The effects (@{#list<openmw.core#MagicEffectWithParams>}) of the ingredient


--- @{#Lockable} functions
-- @field [parent=#types] #Lockable Lockable

---
-- Returns the key record of a lockable object(door, container)
-- @function [parent=#Lockable] getKeyRecord
-- @param openmw.core#GameObject object
-- @return #MiscellaneousRecord

---
-- Sets the key of a lockable object(door, container); removes it if empty string is provided. Must be used in a global script.
-- @function [parent=#Lockable] setKeyRecord
-- @param openmw.core#GameObject object
-- @param #any miscOrId @{#MiscellaneousRecord} or string misc item id Record ID of the key to use.

---
-- Returns the trap spell of a lockable object(door, container)
-- @function [parent=#Lockable] getTrapSpell
-- @param openmw.core#GameObject object
-- @return openmw.core#Spell

---
-- Sets the trap spell of a lockable object(door, container); removes it if empty string is provided. Must be used in a global script.
-- @function [parent=#Lockable] setTrapSpell
-- @param openmw.core#GameObject object
-- @param #any spellOrId @{openmw.core#Spell} or string spell id Record ID for the trap to use

---
-- Returns the lock level of a lockable object(door, container). Does not determine if an object is locked or not, if an object is locked while this is set above 0, this value will be used if no other value is specified.
-- @function [parent=#Lockable] getLockLevel
-- @param openmw.core#GameObject object
-- @return #number


---
-- Returns true if the lockable object is locked, and false if it is not.
-- @function [parent=#Lockable] isLocked
-- @param openmw.core#GameObject object
-- @return #boolean


---
-- Sets the lock level level of a lockable object(door, container);Locks if not already locked; Must be used in a global script.
-- @function [parent=#Lockable] lock
-- @param openmw.core#GameObject object
-- @param #number lockLevel Level to lock the object at. Optional, if not specified, then 1 will be used, or the previous level if it was locked before.

---
-- Unlocks the lockable object. Does not change the lock level, it can be kept for future use.
-- @function [parent=#Lockable] unlock
-- @param openmw.core#GameObject object



--- @{#Light} functions
-- @field [parent=#types] #Light Light

---
-- @type Light
-- @extends #Item
-- @field #Item baseType @{#Item}
-- @field #list<#LightRecord> records A read-only list of all @{#LightRecord}s in the world database.

---
-- Whether the object is a Light.
-- @function [parent=#Light] objectIsInstance
-- @param openmw.core#GameObject object
-- @return #boolean

---
-- Returns the read-only @{#LightRecord} of a Light
-- @function [parent=#Light] record
-- @param #any objectOrRecordId
-- @return #LightRecord

---
-- @type LightRecord
-- @field #string id Record id
-- @field #string name Human-readable name
-- @field #string model VFS path to the model
-- @field #string mwscript MWScript on this light (can be empty)
-- @field #string icon VFS path to the icon
-- @field #string sound VFS path to the sound
-- @field #number weight
-- @field #number value
-- @field #number duration
-- @field #number radius
-- @field #number color
-- @field #boolean isCarriable



--- Functions for @{#Miscellaneous} objects
-- @field [parent=#types] #Miscellaneous Miscellaneous

---
-- @type Miscellaneous
-- @extends #Item
-- @field #Item baseType @{#Item}
-- @field #list<#MiscellaneousRecord> records A read-only list of all @{#MiscellaneousRecord}s in the world database.

---
-- Whether the object is a Miscellaneous.
-- @function [parent=#Miscellaneous] objectIsInstance
-- @param openmw.core#GameObject object
-- @return #boolean

---
-- Returns the read-only @{#MiscellaneousRecord} of a miscellaneous item
-- @function [parent=#Miscellaneous] record
-- @param #any objectOrRecordId
-- @return #MiscellaneousRecord

---
-- Returns the read-only soul of a miscellaneous item
-- @function [parent=#Miscellaneous] getSoul
-- @param openmw.core#GameObject object
-- @return #string

---
-- Creates a @{#MiscellaneousRecord} without adding it to the world database.
-- Use @{openmw_world#(world).createRecord} to add the record to the world.
-- @function [parent=#Miscellaneous] createRecordDraft
-- @param #MiscellaneousRecord miscellaneous A Lua table with the fields of a MiscellaneousRecord.
-- @return #MiscellaneousRecord A strongly typed Miscellaneous record.

---
-- Sets the soul of a miscellaneous item, intended for soul gem objects; Must be used in a global script.
-- @function [parent=#Miscellaneous] setSoul
-- @param openmw.core#GameObject object
-- @param #string soulId Record ID for the soul of the creature to use

---
-- @type MiscellaneousRecord
-- @field #string id The record ID of the miscellaneous item
-- @field #string name The name of the miscellaneous item
-- @field #string model VFS path to the model
-- @field #string mwscript MWScript on this miscellaneous item (can be empty)
-- @field #string icon VFS path to the icon
-- @field #number weight
-- @field #number value
-- @field #boolean isKey

--- @{#Potion} functions
-- @field [parent=#types] #Potion Potion

---
-- @type Potion
-- @extends #Item
-- @field #Item baseType @{#Item}
-- @field #list<#PotionRecord> records A read-only list of all @{#PotionRecord}s in the world database.

---
-- Whether the object is a Potion.
-- @function [parent=#Potion] objectIsInstance
-- @param openmw.core#GameObject object
-- @return #boolean

---
-- Returns the read-only @{#PotionRecord} of a potion
-- @function [parent=#Potion] record
-- @param #any objectOrRecordId
-- @return #PotionRecord

---
-- Creates a @{#PotionRecord} without adding it to the world database.
-- Use @{openmw_world#(world).createRecord} to add the record to the world.
-- @function [parent=#Potion] createRecordDraft
-- @param #PotionRecord potion A Lua table with the fields of a PotionRecord.
-- @return #PotionRecord A strongly typed Potion record.

---
-- @type PotionRecord
-- @field #string id Record id
-- @field #string name Human-readable name
-- @field #string model VFS path to the model
-- @field #string mwscript MWScript on this potion (can be empty)
-- @field #string icon VFS path to the icon
-- @field #number weight
-- @field #number value
-- @field #list<openmw.core#MagicEffectWithParams> effects The effects (@{#list<openmw.core#MagicEffectWithParams>}) of the potion



--- @{#Weapon} functions
-- @field [parent=#types] #Weapon Weapon

---
-- @type Weapon
-- @extends #Item
-- @field #Item baseType @{#Item}
-- @field #list<#WeaponRecord> records A read-only list of all @{#WeaponRecord}s in the world database.

---
-- Whether the object is a Weapon.
-- @function [parent=#Weapon] objectIsInstance
-- @param openmw.core#GameObject object
-- @return #boolean

--- Weapon.TYPE
-- @type WeaponTYPE
-- @field #number ShortBladeOneHand
-- @field #number LongBladeOneHand
-- @field #number LongBladeTwoHand
-- @field #number BluntOneHand
-- @field #number BluntTwoClose
-- @field #number BluntTwoWide
-- @field #number SpearTwoWide
-- @field #number AxeOneHand
-- @field #number AxeTwoHand
-- @field #number MarksmanBow
-- @field #number MarksmanCrossbow
-- @field #number MarksmanThrown
-- @field #number Arrow
-- @field #number Bolt

--- @{#WeaponTYPE}
-- @field [parent=#Weapon] #WeaponTYPE TYPE

---
-- Returns the read-only @{#WeaponRecord} of a weapon
-- @function [parent=#Weapon] record
-- @param #any objectOrRecordId
-- @return #WeaponRecord

---
-- @type WeaponRecord
-- @field #string id Record id
-- @field #string name Human-readable name
-- @field #string model VFS path to the model
-- @field #string mwscript MWScript on this weapon (can be empty)
-- @field #string icon VFS path to the icon
-- @field #string enchant
-- @field #boolean isMagical
-- @field #boolean isSilver
-- @field #number weight
-- @field #number value
-- @field #number type See @{#Weapon.TYPE}
-- @field #number health
-- @field #number speed
-- @field #number reach
-- @field #number enchantCapacity
-- @field #number chopMinDamage
-- @field #number chopMaxDamage
-- @field #number slashMinDamage
-- @field #number slashMaxDamage
-- @field #number thrustMinDamage
-- @field #number thrustMaxDamage

---
-- Creates a @{#WeaponRecord} without adding it to the world database.
-- Use @{openmw_world#(world).createRecord} to add the record to the world.
-- @function [parent=#Weapon] createRecordDraft
-- @param #WeaponRecord weapon A Lua table with the fields of a WeaponRecord.
-- @return #WeaponRecord A strongly typed Weapon record.

--- @{#Apparatus} functions
-- @field [parent=#types] #Apparatus Apparatus

---
-- @type Apparatus
-- @extends #Item
-- @field #Item baseType @{#Item}
-- @field #list<#ApparatusRecord> records A read-only list of all @{#ApparatusRecord}s in the world database.

---
-- Whether the object is an Apparatus.
-- @function [parent=#Apparatus] objectIsInstance
-- @param openmw.core#GameObject object
-- @return #boolean

--- Apparatus.TYPE
-- @type ApparatusTYPE
-- @field #number MortarPestle
-- @field #number Alembic
-- @field #number Calcinator
-- @field #number Retort

--- @{#ApparatusTYPE}
-- @field [parent=#Apparatus] #ApparatusTYPE TYPE

---
-- Returns the read-only @{#ApparatusRecord} of an apparatus
-- @function [parent=#Apparatus] record
-- @param #any objectOrRecordId
-- @return #ApparatusRecord

---
-- @type ApparatusRecord
-- @field #string id The record ID of the apparatus
-- @field #string name The name of the apparatus
-- @field #string model VFS path to the model
-- @field #string mwscript MWScript on this apparatus (can be empty)
-- @field #string icon VFS path to the icon
-- @field #number type The type of apparatus. See @{#Apparatus.TYPE}
-- @field #number weight
-- @field #number value
-- @field #number quality The quality of the apparatus

--- @{#Lockpick} functions
-- @field [parent=#types] #Lockpick Lockpick

---
-- @type Lockpick
-- @extends #Item
-- @field #Item baseType @{#Item}
-- @field #list<#LockpickRecord> records A read-only list of all @{#LockpickRecord}s in the world database.

---
-- Whether the object is a Lockpick.
-- @function [parent=#Lockpick] objectIsInstance
-- @param openmw.core#GameObject object
-- @return #boolean

---
-- Returns the read-only @{#LockpickRecord} of a lockpick
-- @function [parent=#Lockpick] record
-- @param #any objectOrRecordId
-- @return #LockpickRecord

---
-- @type LockpickRecord
-- @field #string id The record ID of the lockpick
-- @field #string name The name of the lockpick
-- @field #string model VFS path to the model
-- @field #string mwscript MWScript on this lockpick (can be empty)
-- @field #string icon VFS path to the icon
-- @field #number maxCondition The maximum number of uses of this lockpick
-- @field #number weight
-- @field #number value
-- @field #number quality The quality of the lockpick

--- @{#Probe} functions
-- @field [parent=#types] #Probe Probe

---
-- @type Probe
-- @extends #Item
-- @field #Item baseType @{#Item}
-- @field #list<#ProbeRecord> records A read-only list of all @{#ProbeRecord}s in the world database.

---
-- Whether the object is a Probe.
-- @function [parent=#Probe] objectIsInstance
-- @param openmw.core#GameObject object
-- @return #boolean

---
-- Returns the read-only @{#ProbeRecord} of a probe
-- @function [parent=#Probe] record
-- @param #any objectOrRecordId
-- @return #ProbeRecord

---
-- @type ProbeRecord
-- @field #string id The record ID of the probe
-- @field #string name The name of the probe
-- @field #string model VFS path to the model
-- @field #string mwscript MWScript on this probe (can be empty)
-- @field #string icon VFS path to the icon
-- @field #number maxCondition The maximum number of uses of this probe
-- @field #number weight
-- @field #number value
-- @field #number quality The quality of the probe

--- @{#Repair} functions
-- @field [parent=#types] #Repair Repair

---
-- @type Repair
-- @extends #Item
-- @field #Item baseType @{#Item}
-- @field #list<#RepairRecord> records A read-only list of all @{#RepairRecord}s in the world database.

---
-- Whether the object is a Repair.
-- @function [parent=#Repair] objectIsInstance
-- @param openmw.core#GameObject object
-- @return #boolean

---
-- Returns the read-only @{#RepairRecord} of a repair tool
-- @function [parent=#Repair] record
-- @param #any objectOrRecordId
-- @return #RepairRecord

---
-- @type RepairRecord
-- @field #string id The record ID of the repair tool
-- @field #string name The name of the repair tool
-- @field #string model VFS path to the model
-- @field #string mwscript MWScript on this repair tool (can be empty)
-- @field #string icon VFS path to the icon
-- @field #number maxCondition The maximum number of uses of this repair tool
-- @field #number weight
-- @field #number value
-- @field #number quality The quality of the repair tool

--- @{#Activator} functions
-- @field [parent=#types] #Activator Activator

---
-- @type Activator
-- @field #list<#ActivatorRecord> records A read-only list of all @{#ActivatorRecord}s in the world database.

---
-- Whether the object is an Activator.
-- @function [parent=#Activator] objectIsInstance
-- @param openmw.core#GameObject object
-- @return #boolean

---
-- Returns the read-only @{#ActivatorRecord} of an activator
-- @function [parent=#Activator] record
-- @param #any objectOrRecordId
-- @return #ActivatorRecord

---
-- @type ActivatorRecord
-- @field #string id Record id
-- @field #string name Human-readable name
-- @field #string model VFS path to the model
-- @field #string mwscript MWScript on this activator (can be empty)

---
-- Creates a @{#ActivatorRecord} without adding it to the world database.
-- Use @{openmw_world#(world).createRecord} to add the record to the world.
-- @function [parent=#Activator] createRecordDraft
-- @param #ActivatorRecord activator A Lua table with the fields of a ActivatorRecord.
-- @return #ActivatorRecord A strongly typed Activator record.

--- @{#Container} functions
-- @field [parent=#types] #Container Container

---
-- @type Container
-- @extends #Lockable
-- @field #Lockable baseType @{#Lockable}
-- @field #list<#ContainerRecord> records A read-only list of all @{#ContainerRecord}s in the world database.

---
-- Container content.
-- @function [parent=#Container] content
-- @param openmw.core#GameObject object
-- @return openmw.core#Inventory

---
-- Whether the object is a Container.
-- @function [parent=#Container] objectIsInstance
-- @param openmw.core#GameObject object
-- @return #boolean

---
-- Returns the total weight of everything in a container
-- @function [parent=#Container] encumbrance
-- @param openmw.core#GameObject object
-- @return #number

---
-- Returns the capacity of a container
-- @function [parent=#Container] capacity
-- @param openmw.core#GameObject object
-- @return #number

---
-- Returns the read-only @{#ContainerRecord} of a container
-- @function [parent=#Container] record
-- @param #any objectOrRecordId
-- @return #ContainerRecord

---
-- @type ContainerRecord
-- @field #string id Record id
-- @field #string name Human-readable name
-- @field #string model VFS path to the model
-- @field #string mwscript MWScript on this container (can be empty)
-- @field #number weight capacity of this container

--- @{#Door} functions
-- @field [parent=#types] #Door Door

---
-- @type Door
-- @extends #Lockable
-- @field #Lockable baseType @{#Lockable}
-- @field #list<#DoorRecord> records A read-only list of all @{#DoorRecord}s in the world database.

---
-- Whether the object is a Door.
-- @function [parent=#Door] objectIsInstance
-- @param openmw.core#GameObject object
-- @return #boolean

---
-- Whether the door is a teleport.
-- @function [parent=#Door] isTeleport
-- @param openmw.core#GameObject object
-- @return #boolean

---
-- Destination (only if a teleport door).
-- @function [parent=#Door] destPosition
-- @param openmw.core#GameObject object
-- @return openmw.util#Vector3

---
-- Destination rotation (only if a teleport door).
-- @function [parent=#Door] destRotation
-- @param openmw.core#GameObject object
-- @return openmw.util#Transform

---
-- Destination cell (only if a teleport door).
-- @function [parent=#Door] destCell
-- @param openmw.core#GameObject object
-- @return openmw.core#Cell

---
-- Returns the read-only @{#DoorRecord} of a door
-- @function [parent=#Door] record
-- @param #any objectOrRecordId
-- @return #DoorRecord

---
-- @type DoorRecord
-- @field #string id Record id
-- @field #string name Human-readable name
-- @field #string model VFS path to the model
-- @field #string mwscript MWScript on this door (can be empty)
-- @field #string openSound VFS path to the sound of opening
-- @field #string closeSound VFS path to the sound of closing



--- Functions for @{#Static} objects
-- @field [parent=#types] #Static Static

---
-- @type Static
-- @field #list<#StaticRecord> records A read-only list of all @{#StaticRecord}s in the world database.

---
-- Whether the object is a Static.
-- @function [parent=#Static] objectIsInstance
-- @param openmw.core#GameObject object
-- @return #boolean

---
-- Returns the read-only @{#StaticRecord} of a Static
-- @function [parent=#Static] record
-- @param #any objectOrRecordId
-- @return #StaticRecord

---
-- @type StaticRecord
-- @field #string id Record id
-- @field #string model VFS path to the model


--- @{#CreatureLevelledList} functions
-- @field [parent=#types] #CreatureLevelledList CreatureLevelledList

---
-- @type CreatureLevelledList
-- @field #list<#CreatureLevelledListRecord> records A read-only list of all @{#CreatureLevelledListRecord}s in the world database.

---
-- Whether the object is a CreatureLevelledList.
-- @function [parent=#CreatureLevelledList] objectIsInstance
-- @param openmw.core#GameObject object
-- @return #boolean

---
-- Returns the read-only @{#CreatureLevelledListRecord} of a levelled creature
-- @function [parent=#CreatureLevelledList] record
-- @param #any objectOrRecordId
-- @return #CreatureLevelledListRecord

---
-- @type CreatureLevelledListRecord
-- @field #string id Record id
-- @field #number chanceNone Chance this list won't spawn anything [0-1]
-- @field #boolean calculateFromAllLevels Calculate from all levels <= player level, not just the closest below player
-- @field #list<#LevelledListItem> creatures

---
-- Picks a random id from the levelled list.
-- @function [parent=#CreatureLevelledListRecord] getRandomId
-- @param openmw.core#CreatureLevelledListRecord The list
-- @param #number The maximum level to select entries for
-- @return #string An id

---
-- @type LevelledListItem
-- @field #string id Item id
-- @field #number level The minimum player level at which this item can occur


--- Functions for @{#ESM4Activator} objects
-- @field [parent=#types] #ESM4Activator ESM4Activator

--- Functions for @{#ESM4Ammunition} objects
-- @field [parent=#types] #ESM4Ammunition ESM4Ammunition

--- Functions for @{#ESM4Armor} objects
-- @field [parent=#types] #ESM4Armor ESM4Armor

--- Functions for @{#ESM4Book} objects
-- @field [parent=#types] #ESM4Book ESM4Book

--- Functions for @{#ESM4Clothing} objects
-- @field [parent=#types] #ESM4Clothing ESM4Clothing

--- Functions for @{#ESM4Door} objects
-- @field [parent=#types] #ESM4Door ESM4Door

--- Functions for @{#ESM4Ingredient} objects
-- @field [parent=#types] #ESM4Ingredient ESM4Ingredient

--- Functions for @{#ESM4Light} objects
-- @field [parent=#types] #ESM4Light ESM4Light

--- Functions for @{#ESM4Miscellaneous} objects
-- @field [parent=#types] #ESM4Miscellaneous ESM4Miscellaneous

--- Functions for @{#ESM4Potion} objects
-- @field [parent=#types] #ESM4Potion ESM4Potion

--- Functions for @{#ESM4Static} objects
-- @field [parent=#types] #ESM4Static ESM4Static

--- Functions for @{#ESM4Weapon} objects
-- @field [parent=#types] #ESM4Weapon ESM4Weapon

---
-- @type ESM4Door
-- @extends #Lockable
-- @field #Lockable baseType @{#Lockable}

---
-- Whether the object is a ESM4Door.
-- @function [parent=#ESM4Door] objectIsInstance
-- @param openmw.core#GameObject object
-- @return #boolean

---
-- Whether the door is a teleport.
-- @function [parent=#ESM4Door] isTeleport
-- @param openmw.core#GameObject object
-- @return #boolean

---
-- Destination (only if a teleport door).
-- @function [parent=#ESM4Door] destPosition
-- @param openmw.core#GameObject object
-- @return openmw.util#Vector3

---
-- Destination rotation (only if a teleport door).
-- @function [parent=#ESM4Door] destRotation
-- @param openmw.core#GameObject object
-- @return openmw.util#Transform

---
-- Destination cell (only if a teleport door).
-- @function [parent=#ESM4Door] destCell
-- @param openmw.core#GameObject object
-- @return openmw.core#Cell

---
-- Returns the read-only @{#ESM4DoorRecord} of a door
-- @function [parent=#ESM4Door] record
-- @param #any objectOrRecordId
-- @return #ESM4DoorRecord

---
-- Returns a read-only list of all @{#ESM4DoorRecord}s in the world database.
-- @function [parent=#ESM4Door] records
-- @return #list<#ESM4DoorRecord>

---
-- @type ESM4DoorRecord
-- @field #string id Record id
-- @field #string name Human-readable name
-- @field #string model VFS path to the model

return nil
