---
-- `openmw.types` defines functions for specific types of game objects.
-- @module types
-- @usage local types = require('openmw.types')

--- Common @{#Actor} functions for Creature, NPC, and Player.
-- @field [parent=#types] #Actor Actor

--- Common functions for Creature, NPC, and Player.
-- @type Actor

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
-- @function [parent=#Actor] runSpeed
-- @param openmw.core#GameObject actor
-- @return #number

---
-- Speed of walking. For dead actors it still returns a positive value.
-- @function [parent=#Actor] walkSpeed
-- @param openmw.core#GameObject actor
-- @return #number

---
-- Current speed.
-- @function [parent=#Actor] currentSpeed
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
-- @function [parent=#Actor] stance
-- @param openmw.core#GameObject actor
-- @return #number

---
-- Returns `true` if the item is equipped on the actor.
-- @function [parent=#Actor] isEquipped
-- @param openmw.core#GameObject actor
-- @param openmw.core#GameObject item
-- @return #boolean

---
-- Get equipment.
-- Returns a table `slot` -> @{openmw.core#GameObject} of currently equipped items.
-- See @{#EQUIPMENT_SLOT}. Returns empty table if the actor doesn't have
-- equipment slots.
-- @function [parent=#Actor] equipment
-- @param openmw.core#GameObject actor
-- @return #map<#number,openmw.core#GameObject>

---
-- Set equipment.
-- Keys in the table are equipment slots (see @{#EQUIPMENT_SLOT}). Each
-- value can be either a `GameObject` or recordId. Raises an error if
-- the actor doesn't have equipment slots and table is not empty. Can be
-- used only in local scripts and only on self.
-- @function [parent=#Actor] setEquipment
-- @param openmw.core#GameObject actor
-- @param equipment
-- @usage local self = require('openmw.self')
-- local Actor = require('openmw.types').Actor
-- Actor.setEquipment(self, {}) -- unequip all

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


--- @{#Item} functions (all pickable items that can be placed to an inventory or container)
-- @field [parent=#types] #Item Item

--- Functions for pickable items that can be placed to an inventory or container
-- @type Item

---
-- Whether the object is an item.
-- @function [parent=#Item] objectIsInstance
-- @param openmw.core#GameObject object
-- @return #boolean



--- @{#Creature} functions
-- @field [parent=#types] #Creature Creature

---
-- @type Creature
-- @extends #Actor
-- @field #Actor baseType @{#Actor}

---
-- Whether the object is a creature.
-- @function [parent=#Creature] objectIsInstance
-- @param openmw.core#GameObject object
-- @return #boolean



--- @{#NPC} functions
-- @field [parent=#types] #NPC NPC

---
-- @type NPC
-- @extends #Actor
-- @field #Actor baseType @{#Actor}
-- @field [parent=#NPC] #NpcStats stats

---
-- Whether the object is an NPC or a Player.
-- @function [parent=#NPC] objectIsInstance
-- @param openmw.core#GameObject object
-- @return #boolean



--- @{#Player} functions
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



--- @{#Armor} functions
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



--- @{#Book} functions
-- @field [parent=#types] #Book Book

---
-- @type Book
-- @extends #Item
-- @field #Item baseType @{#Item}

---
-- Whether the object is a Book.
-- @function [parent=#Book] objectIsInstance
-- @param openmw.core#GameObject object
-- @return #boolean

--- @{#Clothing} functions


-- @field [parent=#types] #Clothing Clothing

---
-- @type Clothing
-- @extends #Item
-- @field #Item baseType @{#Item}

---
-- Whether the object is a Clothing.
-- @function [parent=#Clothing] objectIsInstance
-- @param openmw.core#GameObject object
-- @return #boolean



--- @{#Ingredient} functions
-- @field [parent=#types] #Ingredient Ingredient

---
-- @type Ingredient
-- @extends #Item
-- @field #Item baseType @{#Item}

---
-- Whether the object is an Ingredient.
-- @function [parent=#Ingredient] objectIsInstance
-- @param openmw.core#GameObject object
-- @return #boolean



--- @{#Light} functions
-- @field [parent=#types] #Light Light

---
-- @type Light
-- @extends #Item
-- @field #Item baseType @{#Item}

---
-- Whether the object is a Light.
-- @function [parent=#Light] objectIsInstance
-- @param openmw.core#GameObject object
-- @return #boolean



--- Functions for @{#Miscellaneous} objects
-- @field [parent=#types] #Miscellaneous Miscellaneous

---
-- @type Miscellaneous
-- @extends #Item
-- @field #Item baseType @{#Item}

---
-- Whether the object is a Miscellaneous.
-- @function [parent=#Miscellaneous] objectIsInstance
-- @param openmw.core#GameObject object
-- @return #boolean



--- @{#Potion} functions
-- @field [parent=#types] #Potion Potion

---
-- @type Potion
-- @extends #Item
-- @field #Item baseType @{#Item}

---
-- Whether the object is a Potion.
-- @function [parent=#Potion] objectIsInstance
-- @param openmw.core#GameObject object
-- @return #boolean



--- @{#Weapon} functions
-- @field [parent=#types] #Weapon Weapon

---
-- @type Weapon
-- @extends #Item
-- @field #Item baseType @{#Item}

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
-- @field #string mwscript MWScript on this door (can be empty)
-- @field #string icon
-- @field #string enchant
-- @field #boolean isMagical
-- @field #boolean isSilver
-- @field #number weight
-- @field #number value
-- @field #number type See @{#Weapon.TYPE}
-- @field #number health
-- @field #number speed
-- @field #number reach
-- @field #number enchant
-- @field #number chopMinDamage
-- @field #number chopMaxDamage
-- @field #number slashMinDamage
-- @field #number slashMaxDamage
-- @field #number thrustMinDamage
-- @field #number thrustMaxDamage



--- @{#Apparatus} functions
-- @field [parent=#types] #Apparatus Apparatus

---
-- @type Apparatus
-- @extends #Item
-- @field #Item baseType @{#Item}

---
-- Whether the object is an Apparatus.
-- @function [parent=#Apparatus] objectIsInstance
-- @param openmw.core#GameObject object
-- @return #boolean

--- @{#Lockpick} functions
-- @field [parent=#types] #Lockpick Lockpick

---
-- @type Lockpick
-- @extends #Item
-- @field #Item baseType @{#Item}

---
-- Whether the object is a Lockpick.
-- @function [parent=#Lockpick] objectIsInstance
-- @param openmw.core#GameObject object
-- @return #boolean

--- @{#Probe} functions
-- @field [parent=#types] #Probe Probe

---
-- @type Probe
-- @extends #Item
-- @field #Item baseType @{#Item}

---
-- Whether the object is a Probe.
-- @function [parent=#Probe] objectIsInstance
-- @param openmw.core#GameObject object
-- @return #boolean

--- @{#Repair} functions
-- @field [parent=#types] #Repair Repair

---
-- @type Repair
-- @extends #Item
-- @field #Item baseType @{#Item}

---
-- Whether the object is a Repair.
-- @function [parent=#Repair] objectIsInstance
-- @param openmw.core#GameObject object
-- @return #boolean

--- @{#Activator} functions
-- @field [parent=#types] #Activator Activator

---
-- @type Activator

---
-- Whether the object is an Activator.
-- @function [parent=#Activator] objectIsInstance
-- @param openmw.core#GameObject object
-- @return #boolean

--- @{#Container} functions
-- @field [parent=#types] #Container Container

---
-- @type Container

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



--- @{#Door} functions
-- @field [parent=#types] #Door Door

---
-- @type Door

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
-- @return openmw.util#Vector3

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

---
-- Whether the object is a Static.
-- @function [parent=#Static] objectIsInstance
-- @param openmw.core#GameObject object
-- @return #boolean

return nil

