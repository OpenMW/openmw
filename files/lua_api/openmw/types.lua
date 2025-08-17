---
-- Defines functions for specific types of game objects.
-- @context global|menu|local|player
-- @module types
-- @usage local types = require('openmw.types')

--- Common @{#Actor} functions for Creature, NPC, and Player.
-- @field [parent=#types] #Actor Actor

--- Common functions for Creature, NPC, and Player.
-- @type Actor

---
-- Get the total weight of everything the actor is carrying, plus modifications from magic effects.
-- @function [parent=#Actor] getEncumbrance
-- @param openmw.core#GameObject actor
-- @return #number

---
-- Get the total weight that the actor can carry.
-- @function [parent=#Actor] getCapacity
-- @param openmw.core#GameObject actor
-- @return #number

---
-- Check if the given actor is dead (health reached 0, so death process started).
-- @function [parent=#Actor] isDead
-- @param openmw.core#GameObject actor
-- @return #boolean

---
-- Check if the given actor's death process is finished.
-- @function [parent=#Actor] isDeathFinished
-- @param openmw.core#GameObject actor
-- @return #boolean

---
-- Agent bounds to be used for pathfinding functions.
-- @function [parent=#Actor] getPathfindingAgentBounds
-- @param openmw.core#GameObject actor
-- @return #table with `shapeType` and `halfExtents`

---
-- Check if given actor is in the actors processing range.
-- @function [parent=#Actor] isInActorsProcessingRange
-- @param openmw.core#GameObject actor
-- @return #boolean

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
-- Available @{#EQUIPMENT_SLOT} values. Used in `Actor.getEquipment(obj)` and `Actor.setEquipment(obj, eqp)`.
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
--
--   * With a single argument: returns a table `slot` -> @{openmw.core#GameObject} of currently equipped items.
-- See @{#EQUIPMENT_SLOT}. Returns empty table if the actor doesn't have equipment slots.
--   * With two arguments: returns an item equipped to the given slot.
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
-- Clears the actor's selected castable(spell or enchanted item)
-- @function [parent=#Actor] clearSelectedCastable
-- @param openmw.core#GameObject actor

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
-- if effect.magnitude ~= 0 then
--     print(effect.id..', attribute='..tostring(effect.affectedAttribute)..', skill='..tostring(effect.affectedSkill)..', magnitude='..tostring(effect.magnitude))
-- else
--     print('No Telekinesis effect')
-- end
-- @usage -- Check for a specific effect targeting a specific attribute.
-- local effect = Actor.activeEffects(self):getEffect(core.magic.EFFECT_TYPE.FortifyAttribute, 'luck')
-- if effect.magnitude ~= 0 then
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
-- @return openmw.core#ActiveEffect

---
-- Completely removes the active effect from the actor.
-- @function [parent=#ActorActiveEffects] remove
-- @param self
-- @param #string effectId effect ID
-- @param #string extraParam Optional skill or attribute ID

--- (Note that using this function will override and conflict with all other sources of this effect, you probably want to use @{#ActorActiveEffects.modify} instead, this function is provided for mwscript parity only)
-- Permanently modifies the magnitude of an active effect to be exactly equal to the provided value.
-- Note that although the modification is permanent, the magnitude will not stay equal to the value if any active spells with this effects are added/removed.
-- Also see the notes on @{#ActorActiveEffects.modify}
-- @function [parent=#ActorActiveEffects] set
-- @param self
-- @param #number value
-- @param #string effectId effect ID
-- @param #string extraParam Optional skill or attribute ID

---
-- Permanently modifies the magnitude of an active effect by modifying it by the provided value. Note that some active effect values, such as fortify attribute effects, have no practical effect of their own, and must be paired with explicitly modifying the target stat to have any effect.
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

--- Read-only list of spells currently affecting the actor. Can be iterated over for a list of @{openmw.core#ActiveSpell}
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
-- @usage -- Print all information about active spells
-- for id, params in pairs(Actor.activeSpells(self)) do
--     print('active spell '..tostring(id)..':')
--     print('  name: '..tostring(params.name))
--     print('  id: '..tostring(params.id))
--     print('  item: '..tostring(params.item))
--     print('  caster: '..tostring(params.caster))
--     print('  effects: '..tostring(params.effects))
--     for _, effect in pairs(params.effects) do
--         print('  -> effects['..tostring(effect)..']:')
--         print('       id: '..tostring(effect.id))
--         print('       name: '..tostring(effect.name))
--         print('       affectedSkill: '..tostring(effect.affectedSkill))
--         print('       affectedAttribute: '..tostring(effect.affectedAttribute))
--         print('       magnitudeThisFrame: '..tostring(effect.magnitudeThisFrame))
--         print('       minMagnitude: '..tostring(effect.minMagnitude))
--         print('       maxMagnitude: '..tostring(effect.maxMagnitude))
--         print('       duration: '..tostring(effect.duration))
--         print('       durationLeft: '..tostring(effect.durationLeft))
--     end
-- end

---
-- Get whether any instance of the specific spell is active on the actor.
-- @function [parent=#ActorActiveSpells] isSpellActive
-- @param self
-- @param #any recordOrId A record or string record ID. Valid records are @{openmw.core#Spell}, enchanted @{#Item}, @{#IngredientRecord}, or @{#PotionRecord}.
-- @return true if spell is active, false otherwise

---
-- If true, the actor has not used this power in the last 24h. Will return true for powers the actor does not have.
-- @function [parent=#ActorActiveSpells] canUsePower
-- @param self
-- @param #any spellOrId A @{openmw.core#Spell} or string record id.

---
-- Remove an active spell based on active spell ID (see @{openmw_core#ActiveSpell.activeSpellId}). Can only be used in global scripts or on self. Can only be used to remove spells with the temporary flag set (see @{openmw_core#ActiveSpell.temporary}).
-- @function [parent=#ActorActiveSpells] remove
-- @param self
-- @param #any id Active spell ID.

---
-- Adds a new spell to the list of active spells (only in global scripts or on self).
-- Note that this does not play any related VFX or sounds.
-- Note that this should not be used to add spells without durations (i.e. abilities, curses, and diseases) as they will expire instantly. Use @{#ActorSpells.add} instead.
-- @function [parent=#ActorActiveSpells] add
-- @param self
-- @param #table options A table of parameters. Must contain the following required parameters:
--
--   * `id` - A string record ID. Valid records are @{openmw.core#Spell}, enchanted @{#Item}, @{#IngredientRecord}, or @{#PotionRecord}.
--   * `effects` - A list of indexes of the effects to be applied. These indexes must be in range of the record's list of @{openmw.core#MagicEffectWithParams}. Note that for Ingredients, normal ingredient consumption rules will be applied to effects.
--
-- And may contain the following optional parameters:
--
--   * `name` - The name to show in the list of active effects in the UI. Default: Name of the record identified by the id.
--   * `ignoreResistances` - If true, resistances will be ignored. Default: false
--   * `ignoreSpellAbsorption` - If true, spell absorption will not be applied. Default: false.
--   * `ignoreReflect` - If true, reflects will not be applied. Default: false.
--   * `caster` - A game object that identifies the caster. Default: nil
--   * `item` - A game object that identifies the specific enchanted item instance used to cast the spell. Default: nil
--   * `stackable` - If true, the spell will be able to stack. If false, existing instances of spells with the same id from the same source (where source is caster + item)
--   * `quiet` - If true, no messages will be printed if the spell is an Ingredient and it had no effect. Always true if the target is not the player.
-- @usage
-- -- Adds the effect of the chameleon spell to the character
-- Actor.activeSpells(self):add({id = 'chameleon', effects = { 0 }})
-- @usage
-- -- Adds the effect of a standard potion of intelligence, without consuming any potions from the character's inventory.
-- -- Note that stackable = true to let the effect stack like a potion should.
-- Actor.activeSpells(self):add({id = 'p_fortify_intelligence_s', effects = { 0 }, stackable = true})
-- @usage
-- -- Adds the negative effect of Greef twice over, and renames it to Good Greef.
-- Actor.activeSpells(self):add({id = 'potion_comberry_brandy_01', effects = { 1, 1 }, stackable = true, name = 'Good Greef'})
-- @usage
-- -- Has the same effect as if the actor ate a chokeweed. With the same variable effect based on skill / random chance.
-- Actor.activeSpells(self):add({id = 'ingred_chokeweed_01', effects = { 0 }, stackable = true, name = 'Chokeweed'})
-- -- Same as above, but uses a different index. Note that if multiple indexes are used, the randomicity is applied separately for each effect.
-- Actor.activeSpells(self):add({id = 'ingred_chokeweed_01', effects = { 1 }, stackable = true, name = 'Chokeweed'})

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
-- for _, spell in pairs(core.magic.spells.records) do
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

--- Values affect how much each attribute can be increased at level up, and are all reset to 0 upon level up.
-- @type SkillIncreasesForAttributeStats
-- @field #number agility Number of contributions to agility for the next level up.
-- @field #number endurance Number of contributions to endurance for the next level up.
-- @field #number intelligence Number of contributions to intelligence for the next level up.
-- @field #number luck Number of contributions to luck for the next level up.
-- @field #number personality Number of contributions to personality for the next level up.
-- @field #number speed Number of contributions to speed for the next level up.
-- @field #number strength Number of contributions to strength for the next level up.
-- @field #number willpower Number of contributions to willpower for the next level up.

--- Values affect the graphic used on the level up screen, and are all reset to 0 upon level up.
-- @type SkillIncreasesForSpecializationStats
-- @field #number combat Number of contributions to combat specialization for the next level up.
-- @field #number magic Number of contributions to magic specialization for the next level up.
-- @field #number stealth Number of contributions to stealth specialization for the next level up.

---
-- @type LevelStat
-- @field #number current The actor's current level.
-- @field #number progress The NPC's level progress.
-- @field #SkillIncreasesForAttributeStats skillIncreasesForAttribute The NPC's attribute contributions towards the next level up. Values affect how much each attribute can be increased at level up.
-- @field #SkillIncreasesForSpecializationStats skillIncreasesForSpecialization The NPC's attribute contributions towards the next level up. Values affect the graphic used on the level up screen.

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
-- @type AIStat
-- @field #number base The stat's base value.
-- @field #number modifier The stat's modifier.
-- @field #number modified The actor's current ai value (read-only.)

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
-- @type AIStats

---
-- Alarm (returns @{#AIStat})
-- @function [parent=#AIStats] alarm
-- @param openmw.core#GameObject actor
-- @return #AIStat

---
-- Fight (returns @{#AIStat})
-- @function [parent=#AIStats] fight
-- @param openmw.core#GameObject actor
-- @return #AIStat

---
-- Flee (returns @{#AIStat})
-- @function [parent=#AIStats] flee
-- @param openmw.core#GameObject actor
-- @return #AIStat

---
-- Hello (returns @{#AIStat})
-- @function [parent=#AIStats] hello
-- @param openmw.core#GameObject actor
-- @return #AIStat

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
-- @field #AIStats ai

---
-- Level (returns @{#LevelStat})
-- @function [parent=#ActorStats] level
-- @param openmw.core#GameObject actor
-- @return #LevelStat

--- The actor's stats.
-- @field [parent=#Actor] #ActorStats stats

---
-- @type NpcStats
-- @extends #ActorStats
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
-- (DEPRECATED, use itemData(item).enchantmentCharge) Get this item's current enchantment charge.
-- @function [parent=#Item] getEnchantmentCharge
-- @param openmw.core#GameObject item
-- @return #number The charge remaining. `nil` if the enchantment has never been used, implying the charge is full. Unenchanted items will always return a value of `nil`.

---
-- Checks if the item restocks.
-- Returns true if the object restocks, and false otherwise.
-- @function [parent=#Item] isRestocking
-- @param openmw.core#GameObject item
-- @return #boolean

---
-- (DEPRECATED, use itemData(item).enchantmentCharge) Set this item's enchantment charge.
-- @function [parent=#Item] setEnchantmentCharge
-- @param openmw.core#GameObject item
-- @param #number charge Can be `nil` to reset the unused state / full

---
-- Whether the object is supposed to be carriable. It is true for all items except
-- lights without the Carry flag. Non-carriable lights can still be put into
-- an inventory with an explicit `object:moveInto` call.
-- @function [parent=#Item] isCarriable
-- @param openmw.core#GameObject object
-- @return #boolean

---
-- Set of properties that differentiates one item from another of the same record type; can be used by any script, but only global and self scripts can change values.
-- @function [parent=#Item] itemData
-- @param openmw.core#GameObject item
-- @return #ItemData

---
-- @type ItemData
-- @field #number condition The item's current condition. Time remaining for lights. Uses left for repairs, lockpicks and probes. Current health for weapons and armor.
-- @field #number enchantmentCharge The item's current enchantment charge. Unenchanted items will always return a value of `nil`. Setting this to `nil` will reset the charge of the item.
-- @field #string soul The recordId of the item's current soul. Items without soul will always return a value of `nil`. Setting this to `nil` will remove the soul from the item.

--------------------------------------------------------------------------------
-- @{#Creature} functions
-- @field [parent=#types] #Creature Creature

---
-- @type Creature
-- @extends #Actor
-- @field #Actor baseType @{#Actor}

---
-- A read-only list of all @{#CreatureRecord}s in the world database, may be indexed by recordId.
-- Implements [iterables#List](iterables.html#List) of #CreatureRecord.
-- @field [parent=#Creature] #list<#CreatureRecord> records
-- @usage local creature = types.Creature.records['creature id']  -- get by id
-- @usage local creature = types.Creature.records[1]  -- get by index

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
-- @type CreatureAttack
-- @field #number minDamage Minimum attack damage.
-- @field #number maxDamage Maximum attack damage.

---
-- @type CreatureRecord
-- @field #string id The record ID of the creature
-- @field #string name
-- @field #string baseCreature Record id of a base creature, which was modified to create this one
-- @field #string model VFS path to the creature's model
-- @field #string mwscript MWScript on this creature (can be nil)
-- @field #number soulValue The soul value of the creature record
-- @field #number type The @{#Creature.TYPE} of the creature
-- @field #number baseGold The base barter gold of the creature
-- @field #number combatSkill The base combat skill of the creature. This is the skill value used for all skills with a 'combat' specialization
-- @field #number magicSkill The base magic skill of the creature. This is the skill value used for all skills with a 'magic' specialization
-- @field #number stealthSkill The base stealth skill of the creature. This is the skill value used for all skills with a 'stealth' specialization
-- @field #list<#number> attack A table of the 3 randomly selected attacks used by creatures that do not carry weapons. The table consists of 6 numbers split into groups of 2 values corresponding to minimum and maximum damage in that order.
-- @field #map<#string, #boolean> servicesOffered The services of the creature, in a table. Value is if the service is provided or not, and they are indexed by: Spells, Spellmaking, Enchanting, Training, Repair, Barter, Weapon, Armor, Clothing, Books, Ingredients, Picks, Probes, Lights, Apparatus, RepairItems, Misc, Potions, MagicItems, Travel.
-- @field #list<#TravelDestination> travelDestinations A list of @{#TravelDestination}s for this creature.
-- @field #boolean canFly whether the creature can fly
-- @field #boolean canSwim whether the creature can swim
-- @field #boolean canWalk whether the creature can walk
-- @field #boolean canUseWeapons whether the creature can use weapons and shields
-- @field #boolean isBiped whether the creature is a biped
-- @field #boolean isAutocalc If true, the actors stats will be automatically calculated based on level and class.
-- @field #string primaryFaction Faction ID of the NPCs default faction. Nil if no faction
-- @field #number primaryFactionRank Faction rank of the NPCs default faction. Nil if no faction
-- @field #boolean isEssential whether the creature is essential
-- @field #boolean isRespawning whether the creature respawns after death
-- @field #number bloodType integer representing the blood type of the Creature. Used to generate the correct blood vfx.


--- @{#NPC} functions
-- @field [parent=#types] #NPC NPC

---
-- @type NPC
-- @extends #Actor
-- @field #Actor baseType @{#Actor}
-- @field [parent=#NPC] #NpcStats stats

---
-- Creates an @{#NpcRecord} without adding it to the world database.
-- Use @{openmw_world#(world).createRecord} to add the record to the world.
-- @function [parent=#NPC] createRecordDraft
-- @param #NpcRecord npc A Lua table with the fields of an NpcRecord, with an optional field `template` that accepts an @{#NpcRecord} as a base.
-- @return #NpcRecord A strongly typed NPC record.

---
-- A read-only list of all @{#NpcRecord}s in the world database, may be indexed by recordId.
-- Implements [iterables#List](iterables.html#List) of #NpcRecord.
-- @field [parent=#NPC] #map<#NpcRecord> records
-- @usage local npc = types.NPC.records['npc id']  -- get by id
-- @usage local npc = types.NPC.records[1]  -- get by index

---
-- Whether the object is an NPC or a Player.
-- @function [parent=#NPC] objectIsInstance
-- @param openmw.core#GameObject object
-- @return #boolean

---
-- Get all factions in which NPC has a membership.
-- Note: this function does not take in account an expelling state.
-- @function [parent=#NPC] getFactions
-- @param openmw.core#GameObject actor NPC object
-- @return #list<#string> factionIds List of faction IDs.
-- @usage local NPC = require('openmw.types').NPC;
-- for _, factionId in pairs(types.NPC.getFactions(actor)) do
--     print(factionId);
-- end

---
-- Get rank of given NPC in given faction.
-- Throws an exception if there is no such faction.
-- Note: this function does not take in account an expelling state.
-- @function [parent=#NPC] getFactionRank
-- @param openmw.core#GameObject actor NPC object
-- @param #string faction Faction ID
-- @return #number rank Rank index (from 1), 0 if NPC is not in faction.
-- @usage local NPC = require('openmw.types').NPC;
-- print(NPC.getFactionRank(player, "mages guild");

---
-- Set rank of given NPC in given faction.
-- Throws an exception if there is no such faction, target rank does not exist or actor is not a member of given faction.
-- For NPCs faction also should be an NPC's primary faction.
-- @function [parent=#NPC] setFactionRank
-- @param openmw.core#GameObject actor NPC object
-- @param #string faction Faction ID
-- @param #number value Rank index (from 1).
-- @usage local NPC = require('openmw.types').NPC;
-- NPC.setFactionRank(player, "mages guild", 6);

---
-- Adjust rank of given NPC in given faction.
-- Throws an exception if there is no such faction or actor is not a member of given faction.
-- For NPCs faction also should be an NPC's primary faction.
-- Notes:
--
--   * If rank should become <= 0 after modification, function set rank to lowest available rank.
--   * If rank should become > 0 after modification, but target rank does not exist, function set rank to the highest valid rank.
-- @function [parent=#NPC] modifyFactionRank
-- @param openmw.core#GameObject actor NPC object
-- @param #string faction Faction ID
-- @param #number value Rank index (from 1) modifier. If rank reaches 0 for player character, he leaves the faction.
-- @usage local NPC = require('openmw.types').NPC;
-- NPC.modifyFactionRank(player, "mages guild", 1);

---
-- Add given actor to given faction.
-- Throws an exception if there is no such faction or target actor is not player.
-- Function does nothing if valid target actor is already a member of target faction.
-- @function [parent=#NPC] joinFaction
-- @param openmw.core#GameObject actor NPC object
-- @param #string faction Faction ID
-- @usage local NPC = require('openmw.types').NPC;
-- NPC.joinFaction(player, "mages guild");

---
-- Remove given actor from given faction.
-- Function removes rank data and expelling state, but keeps a reputation in target faction.
-- Throws an exception if there is no such faction or target actor is not player.
-- Function does nothing if valid target actor is already not member of target faction.
-- @function [parent=#NPC] leaveFaction
-- @param openmw.core#GameObject actor NPC object
-- @param #string faction Faction ID
-- @usage local NPC = require('openmw.types').NPC;
-- NPC.leaveFaction(player, "mages guild");

---
-- Get reputation of given actor in given faction.
-- Throws an exception if there is no such faction.
-- @function [parent=#NPC] getFactionReputation
-- @param openmw.core#GameObject actor NPC object
-- @param #string faction Faction ID
-- @return #number reputation Reputation level, 0 if NPC is not in faction.
-- @usage local NPC = require('openmw.types').NPC;
-- print(NPC.getFactionReputation(player, "mages guild"));

---
-- Set reputation of given actor in given faction.
-- Throws an exception if there is no such faction.
-- @function [parent=#NPC] setFactionReputation
-- @param openmw.core#GameObject actor NPC object
-- @param #string faction Faction ID
-- @param #number value Reputation value
-- @usage local NPC = require('openmw.types').NPC;
-- NPC.setFactionReputation(player, "mages guild", 100);

---
-- Adjust reputation of given actor in given faction.
-- Throws an exception if there is no such faction.
-- @function [parent=#NPC] modifyFactionReputation
-- @param openmw.core#GameObject actor NPC object
-- @param #string faction Faction ID
-- @param #number value Reputation modifier value
-- @usage local NPC = require('openmw.types').NPC;
-- NPC.modifyFactionReputation(player, "mages guild", 5);

---
-- Expel NPC from given faction.
-- Throws an exception if there is no such faction.
-- Note: expelled NPC still keeps his rank and reputation in faction, he just get an additonal flag for given faction.
-- @function [parent=#NPC] expel
-- @param openmw.core#GameObject actor NPC object
-- @param #string faction Faction ID
-- @usage local NPC = require('openmw.types').NPC;
-- NPC.expel(player, "mages guild");

---
-- Clear expelling of NPC from given faction.
-- Throws an exception if there is no such faction.
-- @function [parent=#NPC] clearExpelled
-- @param openmw.core#GameObject actor NPC object
-- @param #string faction Faction ID
-- @usage local NPC = require('openmw.types').NPC;
-- NPC.clearExpelled(player, "mages guild");

---
-- Check if NPC is expelled from given faction.
-- Throws an exception if there is no such faction.
-- @function [parent=#NPC] isExpelled
-- @param openmw.core#GameObject actor NPC object
-- @param #string faction Faction ID
-- @return #boolean isExpelled True if NPC is expelled from the faction.
-- @usage local NPC = require('openmw.types').NPC;
-- local result = NPC.isExpelled(player, "mages guild");

---
-- Returns the current disposition of the provided NPC. This is their derived disposition, after modifiers such as personality and faction relations are taken into account.
-- @function [parent=#NPC] getDisposition
-- @param openmw.core#GameObject object
-- @param openmw.core#GameObject player The player that you want to check the disposition for.
-- @return #number

---
-- Returns the current base disposition of the provided NPC. This is their base disposition, before modifiers such as personality and faction relations are taken into account.
-- @function [parent=#NPC] getBaseDisposition
-- @param openmw.core#GameObject object
-- @param openmw.core#GameObject player The player that you want to check the disposition for.
-- @return #number

---
-- Set the base disposition of the provided NPC (only in global scripts or on self).
-- @function [parent=#NPC] setBaseDisposition
-- @param openmw.core#GameObject object
-- @param openmw.core#GameObject player The player that you want to set the disposition for.
-- @param #number value Base disposition is set to this value

---
-- Modify the base disposition of the provided NPC by a certain amount (only in global scripts or on self).
-- @function [parent=#NPC] modifyBaseDisposition
-- @param openmw.core#GameObject object
-- @param openmw.core#GameObject player The player that you want to modify the disposition for.
-- @param #number value Base disposition modification value

--- @{#Classes}: Class Data
-- @field [parent=#NPC] #Classes classes

---
-- A read-only list of all @{#ClassRecord}s in the world database, may be indexed by recordId.
-- Implements [iterables#List](iterables.html#List) of #ClassRecord.
-- @field [parent=#Classes] #list<#ClassRecord> records
-- @usage local class = types.NPC.classes.records['class id']  -- get by id
-- @usage local class = types.NPC.classes.records[1]  -- get by index

---
-- Returns a read-only @{#ClassRecord}
-- @function [parent=#Classes] record
-- @param #string recordId
-- @return #ClassRecord

---
-- Class data record
-- @type ClassRecord
-- @field #string id Class id
-- @field #string name Class name
-- @field #list<#string> attributes A read-only list containing the specialized attributes of the class.
-- @field #list<#string> majorSkills A read-only list containing the major skills of the class.
-- @field #list<#string> minorSkills A read-only list containing the minor skills of the class.
-- @field #string description Class description
-- @field #boolean isPlayable True if the player can play as this class
-- @field #string specialization Class specialization. Either combat, magic, or stealth.

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

--- @{#Races}: Race data
-- @field [parent=#NPC] #Races races

---
-- A read-only list of all @{#RaceRecord}s in the world database.
-- Implements [iterables#List](iterables.html#List) of #RaceRecord.
-- @field [parent=#Races] #list<#RaceRecord> records
-- @usage local race = types.NPC.races.records['race id']  -- get by id
-- @usage local race = types.NPC.races.records[1]  -- get by index

---
-- Returns a read-only @{#RaceRecord}
-- @function [parent=#Races] record
-- @param #string recordId
-- @return #RaceRecord

---
-- Race data record
-- @type RaceRecord
-- @field #string id Race id
-- @field #string name Race name
-- @field #string description Race description
-- @field #map<#string, #number> skills A map of bonus skill points by skill ID
-- @field #list<#string> spells A read-only list containing the ids of all spells inherent to the race
-- @field #boolean isPlayable True if the player can pick this race in character generation
-- @field #boolean isBeast True if this race is a beast race
-- @field #GenderedNumber height Height values
-- @field #GenderedNumber weight Weight values
-- @field #map<#string, #GenderedNumber> attributes A read-only table of attribute ID to base value
-- @usage -- Get base strength for men
-- strength = types.NPC.races.records[1].attributes.strength.male

---
-- @type GenderedNumber
-- @field #number male Male value
-- @field #number female Female value

---
-- @type NpcRecord
-- @field #string id The record ID of the NPC
-- @field #string name
-- @field #string race
-- @field #string class ID of the NPC's class (e.g. acrobat)
-- @field #string model Path to the model associated with this NPC, used for animations.
-- @field #string mwscript MWScript on this NPC (can be nil)
-- @field #string hair Path to the hair body part model
-- @field #string head Path to the head body part model
-- @field #number baseGold The base barter gold of the NPC
-- @field #number baseDisposition NPC's starting disposition
-- @field #boolean isMale The gender setting of the NPC
-- @field #map<#string, #boolean> servicesOffered The services of the NPC, in a table. Value is if the service is provided or not, and they are indexed by: Spells, Spellmaking, Enchanting, Training, Repair, Barter, Weapon, Armor, Clothing, Books, Ingredients, Picks, Probes, Lights, Apparatus, RepairItems, Misc, Potions, MagicItems, Travel.
-- @field #list<#TravelDestination> travelDestinations A list of @{#TravelDestination}s for this NPC.
-- @field #boolean isEssential whether the NPC is essential
-- @field #boolean isRespawning whether the NPC respawns after death
-- @field #number bloodType integer representing the blood type of the NPC. Used to generate the correct blood vfx.

---
-- @type TravelDestination
-- @field #string cellId ID of the Destination cell for this TravelDestination, Can be used with @{openmw_world#(world).getCellById}.
-- @field openmw.util#Vector3 position Destination position for this TravelDestination.
-- @field openmw.util#Transform rotation Destination rotation for this TravelDestination.

--------------------------------------------------------------------------------
-- @{#PLAYER} functions
-- @field [parent=#types] #PLAYER Player

---
-- @type PLAYER
-- @extends #NPC
-- @field #NPC baseType @{#NPC}

---
-- Whether the object is a player.
-- @function [parent=#PLAYER] objectIsInstance
-- @param openmw.core#GameObject object
-- @return #boolean

---
-- Returns the bounty or crime level of the player
-- @function [parent=#PLAYER] getCrimeLevel
-- @param openmw.core#GameObject player
-- @return #number

---
-- Sets the bounty or crime level of the player, may only be used in global scripts
-- @function [parent=#PLAYER] setCrimeLevel
-- @param openmw.core#GameObject player
-- @param #number crimeLevel The requested crime level

---
-- @type OFFENSE_TYPE_IDS
-- @field #number Theft
-- @field #number Assault
-- @field #number Murder
-- @field #number Trespassing
-- @field #number SleepingInOwnedBed
-- @field #number Pickpocket

---
-- Available @{#OFFENSE_TYPE_IDS} values. Used in `I.Crimes.commitCrime`.
-- @field [parent=#PLAYER] #OFFENSE_TYPE_IDS OFFENSE_TYPE

---
-- Whether the character generation for this player is finished.
-- @function [parent=#PLAYER] isCharGenFinished
-- @param openmw.core#GameObject player
-- @return #boolean

---
-- Whether teleportation for this player is enabled.
-- @function [parent=#PLAYER] isTeleportingEnabled
-- @param openmw.core#GameObject player
-- @return #boolean

---
-- Enables or disables teleportation for this player.
-- @function [parent=#PLAYER] setTeleportingEnabled
-- @param openmw.core#GameObject player
-- @param #boolean state True to enable teleporting, false to disable.

---
-- Returns a list containing quests @{#PLAYERQuest} for the specified player, indexed by quest ID.
-- @function [parent=#PLAYER] quests
-- @param openmw.core#GameObject player
-- @return #list<#PLAYERQuest>
-- @usage -- Get stage of a specific quest
-- stage = types.Player.quests(player)["ms_fargothring"].stage
-- @usage -- Start a new quest, add it to the player's quest list but don't add any journal entries
-- types.Player.quests(player)["ms_fargothring"].stage = 0

---
-- Returns @{#PlayerJournal}, which contains the read-only access to journal text data accumulated by the player.
-- Not the same as @{openmw_core#Dialogue.journal} which holds raw game records: with placeholders for dynamic variables and no player-specific info.
-- @function [parent=#PLAYER] journal
-- @param openmw.core#GameObject player
-- @return #PlayerJournal
-- @usage -- Get text of the 1st journal entry player made
-- local entryText = types.Player.journal(player).journalTextEntries[1].text
-- @usage -- Get the number of "my trade" conversation topic lines the player journal accumulated
-- local num = #types.Player.journal(player).topics["my trade"].entries

---
-- A read-only list of player's accumulated journal (quest etc.) entries (@{#PlayerJournalTextEntry} elements), ordered from oldest entry to newest.
-- Implements [iterables#list-iterable](iterables.html#list-iterable) of @{#PlayerJournalTextEntry}.
-- @field [parent=#PlayerJournal] #list<#PlayerJournalTextEntry> journalTextEntries
-- @usage -- The `firstQuestName` variable below is likely to be "a1_1_findspymaster" in vanilla MW
-- local firstQuestName = types.Player.journal(player).journalTextEntries[1].questId
-- @usage -- The number of journal entries accumulated in the player journal
-- local num = #types.Player.journal(player).journalTextEntries
-- @usage -- Print all journal entries accumulated in the player journal
-- for idx, journalEntry in pairs(types.Player.journal(player).journalTextEntries) do
--     print(idx, journalEntry.text)
-- end

---
-- A read-only table of player's accumulated @{#PlayerJournalTopic}s, indexed by the topic name.
-- Implements [iterables#Map](iterables.html#map-iterable) of @{#PlayerJournalTopic}.
-- Topic name index doesn't have to be lowercase.
-- @field [parent=#PlayerJournal] #map<#string, #PlayerJournalTopic> topics
-- @usage local record = types.Player.journal(player).topics["my trade"]
-- @usage local record = types.Player.journal(player).topics["Vivec"]

---
-- @type PlayerJournalTopic
-- @field #string id Topic id. It's a lowercase version of name.
-- @field #string name Topic name. Same as id, but with upper cases preserved.

---
-- A read-only list of player's accumulated conversation lines (@{#PlayerJournalTopicEntry}) for this topic.
-- Implements [iterables#list-iterable](iterables.html#list-iterable) of #PlayerJournalTopicEntry.
-- @field [parent=#PlayerJournalTopic] #list<#PlayerJournalTopicEntry> entries
-- @usage -- First NPC topic line entry in the "Background" topic
-- local firstBackgroundLine = types.Player.journal(player).topics["Background"].entries[1]
-- @usage -- The number of topic entries accumulated in the player journal for "Vivec"
-- local num = #types.Player.journal(player).topics["vivec"].entries
-- @usage -- Print all conversation lines accumulated in the player journal for "Balmora"
-- for idx, topicEntry in pairs(types.Player.journal(player).topics["balmora"].entries) do
--     print(idx, topicEntry.text)
-- end

---
-- @type PlayerJournalTopicEntry
-- @field #string text Text of this topic line.
-- @field #string actor Name of an NPC who is recorded in the player journal as an origin of this topic line.

---
-- Identifier for this topic line. Is unique only within the @{#PlayerJournalTopic} it belongs to.
-- Has a counterpart in raw data game dialogue records at @{openmw_core#DialogueRecordInfo} held by @{openmw_core#Dialogue.topic}
-- @field [parent=#PlayerJournalTopicEntry] #string id

---
-- @type PlayerJournalTextEntry
-- @field #string text Text of this journal entry.
-- @field #string questId Quest id this journal entry is associated with. Can be nil if there is no quest associated with this entry or if journal quest sorting functionality is not available in game.
-- @field #number day Number of the day this journal entry was written at.
-- @field #number month Number of the month this journal entry was written at.
-- @field #number dayOfMonth Number of the day in the month this journal entry was written at.

---
-- Identifier for this journal entry line. Is unique only within the @{#PlayerJournalTextEntry} it belongs to.
-- Has a counterpart in raw data game dialogue records at @{openmw_core#DialogueRecordInfo} held by @{openmw_core#Dialogue.journal}
-- @field [parent=#PlayerJournalTextEntry] #string id

---
-- @type PlayerQuest
-- @field #string id The quest id.
-- @field #number stage The quest stage (global and player scripts can change it). Changing the stage starts the quest if it wasn't started.
-- @field #boolean started Whether the quest is started.
-- @field #boolean finished Whether the quest is finished (global and player scripts can change it).

---
-- Sets the quest stage for the given quest, on the given player, and adds the entry to the journal, if there is an entry at the specified stage. Can only be used in global or player scripts.
-- @function [parent=#PLAYERQuest] addJournalEntry
-- @param self
-- @param #number stage Quest stage
-- @param openmw.core#GameObject actor (optional) The actor who is the source of the journal entry, it may be used in journal entries with variables such as `%name(The speaker's name)` or `%race(The speaker's race)`.

---
-- Get state of a control switch. I.e. is the player able to move/fight/jump/etc.
-- @function [parent=#PLAYER] getControlSwitch
-- @param openmw.core#GameObject player
-- @param #ControlSwitch key Control type (see @{openmw.types#CONTROL_SWITCH})
-- @return #boolean

---
-- Set state of a control switch. I.e. forbid or allow the player to move/fight/jump/etc.
-- Can be used only in global or player scripts.
-- @function [parent=#PLAYER] setControlSwitch
-- @param openmw.core#GameObject player
-- @param #ControlSwitch key Control type (see @{openmw.types#CONTROL_SWITCH})
-- @param #boolean value

---
-- String id of a @{#CONTROL_SWITCH}
-- @type ControlSwitch

---
-- @type CONTROL_SWITCH
-- @field [parent=#CONTROL_SWITCH] #ControlSwitch Controls Ability to move
-- @field [parent=#CONTROL_SWITCH] #ControlSwitch Fighting Ability to attack
-- @field [parent=#CONTROL_SWITCH] #ControlSwitch Jumping Ability to jump
-- @field [parent=#CONTROL_SWITCH] #ControlSwitch Looking Ability to change view direction
-- @field [parent=#CONTROL_SWITCH] #ControlSwitch Magic Ability to use magic
-- @field [parent=#CONTROL_SWITCH] #ControlSwitch ViewMode Ability to toggle 1st/3rd person view
-- @field [parent=#CONTROL_SWITCH] #ControlSwitch VanityMode Vanity view if player doesn't touch controls for a long time

---
-- Values that can be used with getControlSwitch/setControlSwitch.
-- @field [parent=#PLAYER] #CONTROL_SWITCH CONTROL_SWITCH

---
-- @function [parent=#PLAYER] getBirthSign
-- @param openmw.core#GameObject player
-- @return #string The player's birth sign

---
-- Can be used only in global scripts. Note that this does not update the player's spells.
-- @function [parent=#PLAYER] setBirthSign
-- @param openmw.core#GameObject player
-- @param #any recordOrId Record or string ID of the birth sign to assign

--- @{#BirthSigns}: Birth Sign Data
-- @field [parent=#PLAYER] #BirthSigns birthSigns

---
-- A read-only list of all @{#BirthSignRecord}s in the world database.
-- Implements [iterables#List](iterables.html#List) of #BirthSignRecord.
-- @field [parent=#BirthSigns] #list<#BirthSignRecord> records
-- @usage local birthSign = types.Player.birthSigns.records['birthsign id']  -- get by id
-- @usage local birthSign = types.Player.birthSigns.records[1]  -- get by index

---
-- Returns a read-only @{#BirthSignRecord}
-- @function [parent=#BirthSigns] record
-- @param #string recordId
-- @return #BirthSignRecord

---
-- Birth sign data record
-- @type BirthSignRecord
-- @field #string id Birth sign id
-- @field #string name Birth sign name
-- @field #string description Birth sign description
-- @field #string texture Birth sign texture
-- @field #list<#string> spells A read-only list containing the ids of all spells gained from this sign.

---
-- Send an event to menu scripts.
-- @function [parent=#PLAYER] sendMenuEvent
-- @param openmw.core#GameObject player
-- @param #string eventName
-- @param eventData

--------------------------------------------------------------------------------
-- @{#Armor} functions
-- @field [parent=#types] #Armor Armor

---
-- @type Armor
-- @extends #Item
-- @field #Item baseType @{#Item}

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

---
-- A read-only list of all @{#ArmorRecord}s in the world database.
-- Implements [iterables#List](iterables.html#List) of #ArmorRecord.
-- @field [parent=#Armor] #list<#ArmorRecord> records
-- @usage local record = types.Armor.records['example_recordid']
-- @usage local record = types.Armor.records[1]

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
-- @field #string mwscript MWScript on this armor (can be nil)
-- @field #string icon VFS path to the icon
-- @field #string enchant The enchantment ID of this armor (can be nil)
-- @field #number weight
-- @field #number value
-- @field #number type See @{#Armor.TYPE}
-- @field #number health
-- @field #number baseArmor The base armor rating of this armor
-- @field #number enchantCapacity

---
-- Creates a @{#ArmorRecord} without adding it to the world database, for the armor to appear correctly on the body, make sure to use a template as described below.
-- Use @{openmw_world#(world).createRecord} to add the record to the world.
-- @function [parent=#Armor] createRecordDraft
-- @param #ArmorRecord armor A Lua table with the fields of a ArmorRecord, with an additional field `template` that accepts a @{#ArmorRecord} as a base.
-- @return #ArmorRecord A strongly typed Armor record.
-- @usage local armorTemplate = types.Armor.record('orcish_cuirass')
-- local armorTable = {name = "Better Orcish Cuirass",template = armorTemplate,baseArmor = armorTemplate.baseArmor + 10}
--  --This is the new record we want to create, with a record provided as a template.
-- local recordDraft = types.Armor.createRecordDraft(armorTable)--Need to convert the table into the record draft
-- local newRecord = world.createRecord(recordDraft)--This creates the actual record
-- world.createObject(newRecord.id):moveInto(playerActor)--Create an instance of this object, and move it into the player's inventory


--- @{#Book} functions
-- @field [parent=#types] #Book Book

---
-- @type Book
-- @extends #Item
-- @field #Item baseType @{#Item}

---
-- A read-only list of all @{#BookRecord}s in the world database.
-- Implements [iterables#List](iterables.html#List) of #BookRecord.
-- @field [parent=#Book] #list<#BookRecord> records
-- @usage local record = types.Book.records['example_recordid']
-- @usage local record = types.Book.records[1]

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

--- DEPRECATED, use @{openmw.core#Skill}
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
-- @field #string mwscript MWScript on this book (can be nil)
-- @field #string icon VFS path to the icon
-- @field #string enchant The enchantment ID of this book (can be nil)
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
-- @param #BookRecord book A Lua table with the fields of a BookRecord, with an optional field `template` that accepts a @{#BookRecord} as a base.
-- @return #BookRecord A strongly typed Book record.

--- @{#Clothing} functions
-- @field [parent=#types] #Clothing Clothing

---
-- @type Clothing
-- @extends #Item
-- @field #Item baseType @{#Item}

---
-- A read-only list of all @{#ClothingRecord}s in the world database.
-- Implements [iterables#List](iterables.html#List) of #ClothingRecord.
-- @field [parent=#Clothing] #list<#ClothingRecord> records
-- @usage local record = types.Clothing.records['example_recordid']
-- @usage local record = types.Clothing.records[1]

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
-- Creates a @{#ClothingRecord} without adding it to the world database, for the clothing to appear correctly on the body, make sure to use a template as described below.
-- Use @{openmw_world#(world).createRecord} to add the record to the world.
-- @function [parent=#Clothing] createRecordDraft
-- @param #ClothingRecord clothing A Lua table with the fields of a ClothingRecord, with an additional field `template` that accepts a @{#ClothingRecord} as a base.
-- @return #ClothingRecord A strongly typed clothing record.
-- @usage local clothingTemplate = types.Clothing.record('exquisite_robe_01')
-- local clothingTable = {name = "Better Exquisite Robe",template = clothingTemplate,enchantCapacity = clothingTemplate.enchantCapacity + 10}
--  --This is the new record we want to create, with a record provided as a template.
-- local recordDraft = types.Clothing.createRecordDraft(clothingTable)--Need to convert the table into the record draft
-- local newRecord = world.createRecord(recordDraft)--This creates the actual record
-- world.createObject(newRecord.id):moveInto(playerActor)--Create an instance of this object, and move it into the player's inventory

---
-- @type ClothingRecord
-- @field #string id Record id
-- @field #string name Name of the clothing
-- @field #string model VFS path to the model
-- @field #string mwscript MWScript on this clothing (can be nil)
-- @field #string icon VFS path to the icon
-- @field #string enchant The enchantment ID of this clothing (can be nil)
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

---
-- A read-only list of all @{#IngredientRecord}s in the world database.
-- Implements [iterables#List](iterables.html#List) of #IngredientRecord.
-- @field [parent=#Ingredient] #list<#IngredientRecord> records
-- @usage local record = types.Ingredient.records['example_recordid']
-- @usage local record = types.Ingredient.records[1]

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
-- @field #string mwscript MWScript on this potion (can be nil)
-- @field #string icon VFS path to the icon
-- @field #number weight
-- @field #number value
-- @field #list<openmw.core#MagicEffectWithParams> effects The effects (@{#list<openmw.core#MagicEffectWithParams>}) of the ingredient


--- @{#LOCKABLE} functions
-- @field [parent=#types] #LOCKABLE Lockable

---
-- Whether the object is a Lockable.
-- @function [parent=#LOCKABLE] objectIsInstance
-- @param openmw.core#GameObject object
-- @return #boolean

---
-- Returns the key record of a lockable object(door, container)
-- @function [parent=#LOCKABLE] getKeyRecord
-- @param openmw.core#GameObject object
-- @return #MiscellaneousRecord

---
-- Sets the key of a lockable object(door, container); removes it if empty string is provided. Must be used in a global script.
-- @function [parent=#LOCKABLE] setKeyRecord
-- @param openmw.core#GameObject object
-- @param #any miscOrId @{#MiscellaneousRecord} or string misc item id Record ID of the key to use.

---
-- Returns the trap spell of a lockable object(door, container)
-- @function [parent=#LOCKABLE] getTrapSpell
-- @param openmw.core#GameObject object
-- @return openmw.core#Spell

---
-- Sets the trap spell of a lockable object(door, container); removes it if empty string is provided. Must be used in a global script.
-- @function [parent=#LOCKABLE] setTrapSpell
-- @param openmw.core#GameObject object
-- @param #any spellOrId @{openmw.core#Spell} or string spell id Record ID for the trap to use

---
-- Returns the lock level of a lockable object(door, container). Does not determine if an object is locked or not, if an object is locked while this is set above 0, this value will be used if no other value is specified.
-- @function [parent=#LOCKABLE] getLockLevel
-- @param openmw.core#GameObject object
-- @return #number


---
-- Returns true if the lockable object is locked, and false if it is not.
-- @function [parent=#LOCKABLE] isLocked
-- @param openmw.core#GameObject object
-- @return #boolean


---
-- Sets the lock level level of a lockable object(door, container);Locks if not already locked; Must be used in a global script.
-- @function [parent=#LOCKABLE] lock
-- @param openmw.core#GameObject object
-- @param #number lockLevel Level to lock the object at. Optional, if not specified, then 1 will be used, or the previous level if it was locked before.

---
-- Unlocks the lockable object. Does not change the lock level, it can be kept for future use.
-- @function [parent=#LOCKABLE] unlock
-- @param openmw.core#GameObject object



--- @{#Light} functions
-- @field [parent=#types] #Light Light

---
-- @type Light
-- @extends #Item
-- @field #Item baseType @{#Item}

---
-- A read-only list of all @{#LightRecord}s in the world database.
-- Implements [iterables#List](iterables.html#List) of #LightRecord.
-- @field [parent=#Light] #list<#LightRecord> records
-- @usage local record = types.Light.records['example_recordid']
-- @usage local record = types.Light.records[1]

---
-- Whether the object is a Light.
-- @function [parent=#Light] objectIsInstance
-- @param openmw.core#GameObject object
-- @return #boolean

---
-- Creates a @{#LightRecord} without adding it to the world database.
-- Use @{openmw_world#(world).createRecord} to add the record to the world.
-- @function [parent=#Light] createRecordDraft
-- @param #LightRecord light A Lua table with the fields of a LightRecord, with an optional field `template` that accepts a @{#LightRecord} as a base.
-- @return #LightRecord A strongly typed Light record.

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
-- @field #string mwscript MWScript on this light (can be nil)
-- @field #string icon VFS path to the icon
-- @field #string sound VFS path to the sound
-- @field #number weight
-- @field #number value
-- @field #number duration
-- @field #number radius
-- @field openmw.util#Color color
-- @field #boolean isCarriable True if the light can be carried by actors and appears up in their inventory.
-- @field #boolean isDynamic If true, the light will apply to actors and other moving objects
-- @field #boolean isFire True if the light acts like a fire.
-- @field #boolean isFlicker
-- @field #boolean isFlickerSlow
-- @field #boolean isNegative If true, the light will reduce light instead of increasing it.
-- @field #boolean isOffByDefault If true, the light will not emit any light or sound while placed in the world. It will still work in the inventory.
-- @field #boolean isPulse
-- @field #boolean isPulseSlow



--- Functions for @{#Miscellaneous} objects
-- @field [parent=#types] #Miscellaneous Miscellaneous

---
-- @type Miscellaneous
-- @extends #Item
-- @field #Item baseType @{#Item}

---
-- A read-only list of all @{#MiscellaneousRecord}s in the world database.
-- Implements [iterables#List](iterables.html#List) of #MiscellaneousRecord.
-- @field [parent=#Miscellaneous] #list<#MiscellaneousRecord> records
-- @usage local record = types.Miscellaneous.records['example_recordid']
-- @usage local record = types.Miscellaneous.records[1]

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
-- (DEPRECATED, use itemData(item).soul) Returns the read-only soul of a miscellaneous item
-- @function [parent=#Miscellaneous] getSoul
-- @param openmw.core#GameObject object
-- @return #string

---
-- Creates a @{#MiscellaneousRecord} without adding it to the world database.
-- Use @{openmw_world#(world).createRecord} to add the record to the world.
-- @function [parent=#Miscellaneous] createRecordDraft
-- @param #MiscellaneousRecord miscellaneous A Lua table with the fields of a MiscellaneousRecord, with an optional field `template` that accepts a @{#MiscellaneousRecord} as a base.
-- @return #MiscellaneousRecord A strongly typed Miscellaneous record.

---
-- (DEPRECATED, use itemData(item).soul) Sets the soul of a miscellaneous item, intended for soul gem objects; Must be used in a global script.
-- @function [parent=#Miscellaneous] setSoul
-- @param openmw.core#GameObject object
-- @param #string soulId Record ID for the soul of the creature to use

---
-- @type MiscellaneousRecord
-- @field #string id The record ID of the miscellaneous item
-- @field #string name The name of the miscellaneous item
-- @field #string model VFS path to the model
-- @field #string mwscript MWScript on this miscellaneous item (can be nil)
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

---
-- A read-only list of all @{#PotionRecord}s in the world database.
-- Implements [iterables#List](iterables.html#List) of #PotionRecord.
-- @field [parent=#Potion] #list<#PotionRecord> records
-- @usage local record = types.Potion.records['example_recordid']
-- @usage local record = types.Potion.records[1]

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
-- @param #PotionRecord potion A Lua table with the fields of a PotionRecord, with an optional field `template` that accepts a @{#PotionRecord} as a base.
-- @return #PotionRecord A strongly typed Potion record.

---
-- @type PotionRecord
-- @field #string id Record id
-- @field #string name Human-readable name
-- @field #string model VFS path to the model
-- @field #string mwscript MWScript on this potion (can be nil)
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

---
-- A read-only list of all @{#WeaponRecord}s in the world database.
-- Implements [iterables#List](iterables.html#List) of #WeaponRecord.
-- @field [parent=#Weapon] #list<#WeaponRecord> records
-- @usage local record = types.Weapon.records['example_recordid']
-- @usage local record = types.Weapon.records[1]

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
-- @field #string mwscript MWScript on this weapon (can be nil)
-- @field #string icon VFS path to the icon
-- @field #string enchant The enchantment ID of this weapon (can be nil)
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
-- @param #WeaponRecord weapon A Lua table with the fields of a WeaponRecord, with an optional field `template` that accepts a @{#WeaponRecord} as a base.
-- @return #WeaponRecord A strongly typed Weapon record.

--- @{#Apparatus} functions
-- @field [parent=#types] #Apparatus Apparatus

---
-- @type Apparatus
-- @extends #Item
-- @field #Item baseType @{#Item}


---
-- A read-only list of all @{#ApparatusRecord}s in the world database.
-- Implements [iterables#List](iterables.html#List) of #ApparatusRecord.
-- @field [parent=#Apparatus] #list<#ApparatusRecord> records
-- @usage local record = types.Apparatus.records['example_recordid']
-- @usage local record = types.Apparatus.records[1]

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
-- @field #string mwscript MWScript on this apparatus (can be nil)
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

---
-- A read-only list of all @{#LockpickRecord}s in the world database.
-- Implements [iterables#List](iterables.html#List) of #LockpickRecord.
-- @field [parent=#Lockpick] #list<#LockpickRecord> records
-- @usage local record = types.Lockpick.records['example_recordid']
-- @usage local record = types.Lockpick.records[1]

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
-- @field #string mwscript MWScript on this lockpick (can be nil)
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

---
-- A read-only list of all @{#ProbeRecord}s in the world database.
-- Implements [iterables#List](iterables.html#List) of #ProbeRecord.
-- @field [parent=#Probe] #list<#ProbeRecord> records
-- @usage local record = types.Probe.records['example_recordid']
-- @usage local record = types.Probe.records[1]

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
-- @field #string mwscript MWScript on this probe (can be nil)
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

---
-- A read-only list of all @{#RepairRecord}s in the world database.
-- Implements [iterables#List](iterables.html#List) of #RepairRecord.
-- @field [parent=#Repair] #list<#RepairRecord> records
-- @usage local record = types.Repair.records['example_recordid']
-- @usage local record = types.Repair.records[1]

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
-- @field #string mwscript MWScript on this repair tool (can be nil)
-- @field #string icon VFS path to the icon
-- @field #number maxCondition The maximum number of uses of this repair tool
-- @field #number weight
-- @field #number value
-- @field #number quality The quality of the repair tool

--- @{#Activator} functions
-- @field [parent=#types] #Activator Activator

---
-- @type Activator

---
-- A read-only list of all @{#ActivatorRecord}s in the world database.
-- Implements [iterables#List](iterables.html#List) of #ActivatorRecord.
-- @field [parent=#Activator] #list<#ActivatorRecord> records
-- @usage local record = types.Activator.records['example_recordid']
-- @usage local record = types.Activator.records[1]

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
-- @field #string mwscript MWScript on this activator (can be nil)

---
-- Creates a @{#ActivatorRecord} without adding it to the world database.
-- Use @{openmw_world#(world).createRecord} to add the record to the world.
-- @function [parent=#Activator] createRecordDraft
-- @param #ActivatorRecord activator A Lua table with the fields of a ActivatorRecord, with an optional field `template` that accepts a @{#ActivatorRecord} as a base.
-- @return #ActivatorRecord A strongly typed Activator record.


--------------------------------------------------------------------------------
-- @{#Container} functions
-- @field [parent=#types] #Container Container

---
-- @type Container
-- @extends #LOCKABLE
-- @field #LOCKABLE baseType @{#LOCKABLE}

---
-- A read-only list of all @{#ContainerRecord}s in the world database.
-- Implements [iterables#List](iterables.html#List) of #ContainerRecord.
-- @field [parent=#Container] #list<#ContainerRecord> records
-- @usage local record = types.Container.records['example_recordid']
-- @usage local record = types.Container.records[1]

---
-- Container content.
-- @function [parent=#Container] content
-- @param openmw.core#GameObject object
-- @return openmw.core#Inventory

---
-- Container content (same as `Container.content`, added for consistency with `Actor.inventory`).
-- @function [parent=#Container] inventory
-- @param openmw.core#GameObject object
-- @return openmw.core#Inventory

---
-- Whether the object is a Container.
-- @function [parent=#Container] objectIsInstance
-- @param openmw.core#GameObject object
-- @return #boolean

---
-- Returns the total weight of everything in a container
-- @function [parent=#Container] getEncumbrance
-- @param openmw.core#GameObject object
-- @return #number

---
-- Returns the capacity of a container
-- @function [parent=#Container] getCapacity
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
-- @field #string mwscript MWScript on this container (can be nil)
-- @field #number weight capacity of this container
-- @field #boolean isOrganic Whether items can be placed in the container
-- @field #boolean isRespawning Whether the container respawns its contents

--------------------------------------------------------------------------------
-- @{#Door} functions
-- @field [parent=#types] #Door Door

---
-- @type Door
-- @extends #LOCKABLE
-- @field #LOCKABLE baseType @{#LOCKABLE}

--- Door.STATE
-- @type DoorSTATE
-- @field #number Idle The door is either closed or open (usually closed).
-- @field #number Opening The door is in the process of opening.
-- @field #number Closing The door is in the process of closing.

--- @{#DoorSTATE}
-- @field [parent=#Door] #DoorSTATE STATE
-- @usage local state = types.Door.STATE["Idle"]

---
-- A read-only list of all @{#DoorRecord}s in the world database.
-- Implements [iterables#List](iterables.html#List) of #DoorRecord.
-- @field [parent=#Door] #list<#DoorRecord> records
-- @usage local record = types.Door.records['example_recordid']
-- @usage local record = types.Door.records[1]

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
-- Gets the state of the door.
-- @function [parent=#Door] getDoorState
-- @param openmw.core#GameObject object
-- @return #DoorSTATE

---
-- Checks if the door is fully open.
-- Returns false if the door is currently opening or closing.
-- @function [parent=#Door] isOpen
-- @param openmw.core#GameObject object
-- @return #boolean

---
-- Checks if the door is fully closed.
-- Returns false if the door is currently opening or closing.
-- @function [parent=#Door] isClosed
-- @param openmw.core#GameObject object
-- @return #boolean

---
-- Opens/Closes the door. Can only be used in global scripts or on self.
-- @function [parent=#Door] activateDoor
-- @param openmw.core#GameObject object
-- @param #boolean openState Optional whether the door should be opened or closed. If not provided, the door will switch to the opposite state.
-- @usage types.Door.activateDoor(doorObject)
-- @usage types.Door.activateDoor(doorObject, true)
-- @usage types.Door.activateDoor(doorObject, false)

---
-- @type DoorRecord
-- @field #string id Record id
-- @field #string name Human-readable name
-- @field #string model VFS path to the model
-- @field #string mwscript MWScript on this door (can be nil)
-- @field #string openSound The sound id for door opening
-- @field #string closeSound The sound id for door closing


--- Functions for @{#Static} objects
-- @field [parent=#types] #Static Static

---
-- @type Static

---
-- A read-only list of all @{#StaticRecord}s in the world database.
-- Implements [iterables#List](iterables.html#List) of #StaticRecord.
-- @field [parent=#Static] #list<#StaticRecord> records
-- @usage local record = types.Static.records['example_recordid']
-- @usage local record = types.Static.records[1]

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
-- @field [parent=#types] #CreatureLevelledList LevelledCreature

---
-- @type CreatureLevelledList

---
-- A read-only list of all @{#CreatureLevelledListRecord}s in the world database.
-- Implements [iterables#List](iterables.html#List) of #CreatureLevelledListRecord.
-- @field [parent=#CreatureLevelledList] #list<#CreatureLevelledListRecord> records
-- @usage local record = types.CreatureLevelledList.records['example_recordid']
-- @usage local record = types.CreatureLevelledList.records[1]

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
-- @param openmw.core#CreatureLevelledListRecord listRecord The list
-- @param #number MaxLvl The maximum level to select entries for
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

--- Functions for @{#ESM4Flora} objects
-- @field [parent=#types] #ESM4Flora ESM4Flora

--- Functions for @{#ESM4Terminal} objects
-- @field [parent=#types] #ESM4Terminal ESM4Terminal

--- Functions for @{#ESM4Ingredient} objects
-- @field [parent=#types] #ESM4Ingredient ESM4Ingredient

--- Functions for @{#ESM4ItemMod} objects
-- @field [parent=#types] #ESM4ItemMod ESM4ItemMod

--- Functions for @{#ESM4Light} objects
-- @field [parent=#types] #ESM4Light ESM4Light

--- Functions for @{#ESM4Miscellaneous} objects
-- @field [parent=#types] #ESM4Miscellaneous ESM4Miscellaneous

--- Functions for @{#ESM4MovableStatic} objects
-- @field [parent=#types] #ESM4MovableStatic ESM4MovableStatic

--- Functions for @{#ESM4Potion} objects
-- @field [parent=#types] #ESM4Potion ESM4Potion

--- Functions for @{#ESM4Static} objects
-- @field [parent=#types] #ESM4Static ESM4Static

--- Functions for @{#ESM4StaticCollection} objects
-- @field [parent=#types] #ESM4StaticCollection ESM4StaticCollection

--- Functions for @{#ESM4Weapon} objects
-- @field [parent=#types] #ESM4Weapon ESM4Weapon

---
-- @type ESM4Terminal

---
-- A read-only list of all @{#ESM4TerminalRecord}s in the world database.
-- Implements [iterables#List](iterables.html#List) of #ESM4TerminalRecord.
-- @field [parent=#ESM4Terminal] #list<#ESM4TerminalRecord> records
-- @usage local record = types.ESM4Terminal.records['example_recordid']
-- @usage local record = types.ESM4Terminal.records[1]

---
-- Whether the object is a ESM4Terminal.
-- @function [parent=#ESM4Terminal] objectIsInstance
-- @param openmw.core#GameObject object
-- @return #boolean

---
-- Returns the read-only @{#ESM4TerminalRecord} of a terminal
-- @function [parent=#ESM4Terminal] record
-- @param #any objectOrRecordId
-- @return #ESM4TerminalRecord

---
-- @type ESM4TerminalRecord
-- @field #string id Record id (Form ID)
-- @field #string editorId Human-readable ID
-- @field #string name Human-readable name
-- @field #string model VFS path to the model
-- @field #string text Text body of the terminal record
-- @field #string resultText Result text of the terminal record

---
-- @type ESM4Door
-- @extends #LOCKABLE
-- @field #LOCKABLE baseType @{#LOCKABLE}

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
-- A read-only list of all @{#ESM4DoorRecord}s in the world database.
-- Implements [iterables#List](iterables.html#List) of #ESM4DoorRecord.
-- @field [parent=#ESM4Door] #list<#ESM4DoorRecord> records
-- @usage local record = types.ESM4Door.records['example_recordid']
-- @usage local record = types.ESM4Door.records[1]

---
-- @type ESM4DoorRecord
-- @field #string id Record id
-- @field #string name Human-readable name
-- @field #string model VFS path to the model
-- @field #string openSound FormId of the door opening sound
-- @field #string closeSound FormId of the door closing sound

return nil
