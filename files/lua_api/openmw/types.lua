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

