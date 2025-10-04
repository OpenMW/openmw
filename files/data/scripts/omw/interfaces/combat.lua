local I = require('openmw.interfaces')
local auxUtil = require('openmw_aux.util')
local functions = require('scripts.omw.interfaces.combatfunctions')

local onHitHandlers = {}

---
-- Table of possible attack source types
-- @type AttackSourceType
-- @field #string Magic
-- @field #string Melee
-- @field #string Ranged
-- @field #string Unspecified

---
-- @type AttackInfo
-- @field [parent=#AttackInfo] #table damage A table mapping stat name (health, fatigue, or magicka) to number. For example, {health = 50, fatigue = 10} will cause 50 damage to health and 10 to fatigue (before adjusting for armor and difficulty). This field is ignored for failed attacks.
-- @field [parent=#AttackInfo] #number strength A number between 0 and 1 representing the attack strength. This field is ignored for failed attacks.
-- @field [parent=#AttackInfo] #boolean successful Whether the attack was successful or not.
-- @field [parent=#AttackInfo] #AttackSourceType sourceType What class of attack this is.
-- @field [parent=#AttackInfo] openmw.self#ATTACK_TYPE type (Optional) Attack variant if applicable. For melee attacks this represents chop vs thrust vs slash. For unarmed creatures this implies which of its 3 possible attacks were used. For other attacks this field can be ignored.
-- @field [parent=#AttackInfo] openmw.types#Actor attacker (Optional) Attacking actor
-- @field [parent=#AttackInfo] openmw.types#Weapon weapon (Optional) Attacking weapon
-- @field [parent=#AttackInfo] #string ammo (Optional) Ammo record ID
-- @field [parent=#AttackInfo] openmw.util#Vector3 hitPos (Optional) Where on the victim the attack is landing. Used to spawn blood effects. Blood effects are skipped if nil.
return {
    --- Basic combat interface
    -- @module Combat
    -- @usage require('openmw.interfaces').Combat
    --
    --I.Combat.addOnHitHandler(function(attack)
    --    -- Adds fatigue loss when hit by draining fatigue when taking health damage
    --    if attack.damage.health and not attack.damage.fatigue then
    --        local strengthFactor = Actor.stats.attributes.strength(self).modified / 100 * 0.66
    --        local enduranceFactor = Actor.stats.attributes.endurance(self).modified / 100 * 0.34
    --        local factor = 1 - math.min(strengthFactor + enduranceFactor, 1)
    --        if factor > 0 then
    --            attack.damage.fatigue = attack.damage.health * factor
    --        end
    --    end
    --end)

    interfaceName = 'Combat',
    interface = {
        --- Interface version
        -- @field [parent=#Combat] #number version
        version = 1,

        --- Add new onHit handler for this actor
        -- If `handler(attack)` returns false, other handlers for
        -- the call will be skipped. where attack is the same @{#AttackInfo} passed to #Combat.onHit
        -- @function [parent=#Combat] addOnHitHandler
        -- @param #function handler The handler.
        addOnHitHandler = function(handler)
            onHitHandlers[#onHitHandlers + 1] = handler
        end,

        --- Calculates the character's armor rating and adjusts damage accordingly.
        -- Note that this function only adjusts the number, use #Combat.applyArmor
        -- to include other side effects.
        -- @function [parent=#Combat] adjustDamageForArmor
        -- @param #number Damage The numeric damage to adjust
        -- @param openmw.core#GameObject actor (Optional) The actor to calculate the armor rating for. Defaults to self.
        -- @return #number Damage adjusted for armor
        adjustDamageForArmor = functions.adjustDamageForArmor,

        --- Calculates a difficulty multiplier based on current difficulty settings
        -- and adjusts damage accordingly. Has no effect if both this actor and the
        -- attacker are NPCs, or if both are Players.
        -- @function [parent=#Combat] adjustDamageForDifficulty
        -- @param #Attack attack The attack to adjust
        -- @param openmw.core#GameObject defendant (Optional) The defendant to make the difficulty adjustment for. Defaults to self.
        adjustDamageForDifficulty = functions.adjustDamageForDifficulty,

        --- Applies this character's armor to the attack. Adjusts damage, reduces item
        -- condition accordingly, progresses armor skill, and plays the armor appropriate
        -- hit sound.
        -- @function [parent=#Combat] applyArmor
        -- @param #Attack attack
        applyArmor = functions.applyArmor,

        --- Computes this character's armor rating.
        -- Note that this interface function is read by the engine to update the UI.
        -- This function can still be overridden same as any other interface, but must not call any functions or interfaces that modify anything.
        -- @function [parent=#Combat] getArmorRating
        -- @param openmw.core#GameObject actor (Optional) The actor to calculate the armor rating for. Defaults to self.
        -- @return #number
        getArmorRating = functions.getArmorRating,

        --- Computes this character's armor rating.
        -- You can override this to return any skill you wish (including non-armor skills, if you so wish).
        -- Note that this interface function is read by the engine to update the UI.
        -- This function can still be overridden same as any other interface, but must not call any functions or interfaces that modify anything.
        -- @function [parent=#Combat] getArmorSkill
        -- @param openmw.core#GameObject item The item
        -- @return #string The armor skill identifier, or unarmored if the item was nil or not an instace of @{openmw.types#Armor}
        getArmorSkill = functions.getArmorSkill,

        --- Computes the armor rating of a single piece of @{openmw.types#Armor}, adjusted for skill
        -- Note that this interface function is read by the engine to update the UI.
        -- This function can still be overridden same as any other interface, but must not call any functions or interfaces that modify anything.
        -- @function [parent=#Combat] getSkillAdjustedArmorRating
        -- @param openmw.core#GameObject item The item
        -- @param openmw.core#GameObject actor (Optional) The actor, defaults to self
        -- @return #number
        getSkillAdjustedArmorRating = functions.getSkillAdjustedArmorRating,

        --- Computes the effective armor rating of a single piece of @{openmw.types#Armor}, adjusted for skill and item condition
        -- @function [parent=#Combat] getEffectiveArmorRating
        -- @param openmw.core#GameObject item The item
        -- @param openmw.core#GameObject actor (Optional) The actor, defaults to self
        -- @return #number
        getEffectiveArmorRating = functions.getEffectiveArmorRating,

        --- Spawns a random blood effect at the given position
        -- @function [parent=#Combat] spawnBloodEffect
        -- @param openmw.util#Vector3 position
        spawnBloodEffect = functions.spawnBloodEffect,

        --- Hit this actor. Normally called as Hit event from the attacking actor, with the same parameters.
        -- @function [parent=#Combat] onHit
        -- @param #AttackInfo attackInfo
        onHit = function(attackInfo) auxUtil.callEventHandlers(onHitHandlers, attackInfo) end,

        --- Picks a random armor slot and returns the item equipped in that slot.
        -- Used to pick which armor to damage / skill to increase when hit during combat.
        -- @function [parent=#Combat] pickRandomArmor
        -- @param openmw.core#GameObject actor (Optional) The actor to pick armor from, defaults to self
        -- @return openmw.core#GameObject The armor equipped in the chosen slot. nil if nothing was equipped in that slot.
        pickRandomArmor = functions.pickRandomArmor,

        --- @{#AttackSourceType}
        -- @field [parent=#Combat] #AttackSourceType ATTACK_SOURCE_TYPES Available attack source types
        ATTACK_SOURCE_TYPES = {
            Magic = 'magic',
            Melee = 'melee',
            Ranged = 'ranged',
            Unspecified = 'unspecified',
        },
    },

    eventHandlers = {
        Hit = function(data) I.Combat.onHit(data) end,
    },
}