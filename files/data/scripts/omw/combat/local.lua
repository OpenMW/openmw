local animation = require('openmw.animation')
local async = require('openmw.async')
local core = require('openmw.core')
local I = require('openmw.interfaces')
local self = require('openmw.self')
local storage = require('openmw.storage')
local types = require('openmw.types')
local util = require('openmw.util')
local Actor = types.Actor
local Weapon = types.Weapon
local Player = types.Player
local Creature = types.Creature
local Armor = types.Armor
local isPlayer = Player.objectIsInstance(self)

local godMode = function() return false end
if isPlayer then
    -- openmw.debug is only allowed on player scripts
    godMode = function() return require('openmw.debug').isGodMode() end
end

local onHitHandlers = {}

local settings = storage.globalSection('SettingsOMWCombat')

local function getSkill(actor, skillId)
    if Creature.objectIsInstance(actor) then
        local specialization = core.stats.Skill.record(skillId).specialization
        local creatureRecord = Creature.record(actor)
        return creatureRecord[specialization..'Skill']
    else
        return types.NPC.stats.skills[skillId](actor).modified
    end
end

local armorTypeGmst = {
    [Armor.TYPE.Boots] = core.getGMST('iBootsWeight'),
    [Armor.TYPE.Cuirass] = core.getGMST('iCuirassWeight'),
    [Armor.TYPE.Greaves] = core.getGMST('iGreavesWeight'),
    [Armor.TYPE.Helmet] = core.getGMST('iHelmWeight'),
    [Armor.TYPE.LBracer] = core.getGMST('iGauntletWeight'),
    [Armor.TYPE.LGauntlet] = core.getGMST('iGauntletWeight'),
    [Armor.TYPE.LPauldron] = core.getGMST('iPauldronWeight'),
    [Armor.TYPE.RBracer] = core.getGMST('iGauntletWeight'),
    [Armor.TYPE.RGauntlet] = core.getGMST('iGauntletWeight'),
    [Armor.TYPE.RPauldron] = core.getGMST('iPauldronWeight'),
    [Armor.TYPE.Shield] = core.getGMST('iShieldWeight'),
}

local armorSlots = {
    Actor.EQUIPMENT_SLOT.Boots,
    Actor.EQUIPMENT_SLOT.Cuirass,
    Actor.EQUIPMENT_SLOT.Greaves,
    Actor.EQUIPMENT_SLOT.Helmet,
    Actor.EQUIPMENT_SLOT.LeftGauntlet,
    Actor.EQUIPMENT_SLOT.LeftPauldron,
    Actor.EQUIPMENT_SLOT.RightGauntlet,
    Actor.EQUIPMENT_SLOT.RightPauldron,
    Actor.EQUIPMENT_SLOT.CarriedLeft,
}

local function getArmorSkill(item)
    if not item or not Armor.objectIsInstance(item) then
        return 'unarmored'
    end
    local record = Armor.record(item)
    local weightGmst = armorTypeGmst[record.type]
    local epsilon = 0.0005
    if record.weight <= weightGmst * core.getGMST('fLightMaxMod') + epsilon then
        return 'lightarmor'
    elseif record.weight <= weightGmst * core.getGMST('fMedMaxMod') + epsilon then
        return 'mediumarmor'
    else
        return 'heavyarmor'
    end
end

local function getSkillAdjustedArmorRating(item, actor)
    local record = Armor.record(item)
    local skillid = I.Combat.getArmorSkill(item)
    local skill = getSkill(actor, skillid)
    if record.weight == 0 then
        return record.baseArmor
    end
    return record.baseArmor * skill / core.getGMST('iBaseArmorSkill')
end

local function getEffectiveArmorRating(item, actor)
    local record = Armor.record(item)
    local rating = getSkillAdjustedArmorRating(item, actor)
    if record.health and record.health ~= 0 then
        rating = rating * (types.Item.itemData(item).condition / record.health)
    end
    return rating
end

local function getArmorRating(actor)
    local magicShield = Actor.activeEffects(actor):getEffect(core.magic.EFFECT_TYPE.Shield).magnitude

    if Creature.objectIsInstance(actor) then
        return magicShield
    end

    local equipment = Actor.getEquipment(actor)
    local ratings = {}
    local unarmored = getSkill(actor, 'unarmored')
    local fUnarmoredBase1 = core.getGMST('fUnarmoredBase1')
    local fUnarmoredBase2 = core.getGMST('fUnarmoredBase2')

    for _, v in pairs(armorSlots) do
        if equipment[v] and Armor.objectIsInstance(equipment[v]) then
            ratings[v] = I.Combat.getEffectiveArmorRating(equipment[v], actor)
        else
            -- Unarmored
            ratings[v] = (fUnarmoredBase1 * unarmored) * (fUnarmoredBase2 * unarmored)
        end
    end

    return ratings[Actor.EQUIPMENT_SLOT.Cuirass] * 0.3
        +  ratings[Actor.EQUIPMENT_SLOT.CarriedLeft] * 0.1
        +  ratings[Actor.EQUIPMENT_SLOT.Helmet] * 0.1
        +  ratings[Actor.EQUIPMENT_SLOT.Greaves] * 0.1
        +  ratings[Actor.EQUIPMENT_SLOT.Boots] * 0.1
        +  ratings[Actor.EQUIPMENT_SLOT.LeftPauldron] * 0.1
        +  ratings[Actor.EQUIPMENT_SLOT.RightPauldron] * 0.1
        +  ratings[Actor.EQUIPMENT_SLOT.LeftGauntlet] * 0.05
        +  ratings[Actor.EQUIPMENT_SLOT.RightGauntlet] * 0.05
        +  magicShield
end

local function adjustDamageForArmor(damage, actor)
    local armor = I.Combat.getArmorRating(actor)
    local x = damage / (damage + armor)
    return damage * math.max(x, core.getGMST('fCombatArmorMinMult'))
end

local function pickRandomArmor(actor)
    local slot = nil
    local roll = math.random(0, 99) -- randIntUniform(0, 100)
    if roll >= 90 then
        slot = Actor.EQUIPMENT_SLOT.CarriedLeft
        local item = Actor.getEquipment(actor, slot)
        local haveShield = item and Armor.objectIsInstance(item)
        if settings:get('redistributeShieldHitsWhenNotWearingShield') and not haveShield then
            if roll >= 95 then
                slot = Actor.EQUIPMENT_SLOT.Cuirass
            else
                slot = Actor.EQUIPMENT_SLOT.LeftPauldron
            end
        end
    elseif roll >= 85 then
        slot = Actor.EQUIPMENT_SLOT.RightGauntlet
    elseif roll >= 80 then
        slot = Actor.EQUIPMENT_SLOT.LeftGauntlet
    elseif roll >= 70 then
        slot = Actor.EQUIPMENT_SLOT.RightPauldron
    elseif roll >= 60 then
        slot = Actor.EQUIPMENT_SLOT.LeftPauldron
    elseif roll >= 50 then
        slot = Actor.EQUIPMENT_SLOT.Boots
    elseif roll >= 40 then
        slot = Actor.EQUIPMENT_SLOT.Greaves
    elseif roll >= 30 then
        slot = Actor.EQUIPMENT_SLOT.Helmet
    else
        slot = Actor.EQUIPMENT_SLOT.Cuirass
    end

    return Actor.getEquipment(actor, slot)
end

local function getDamage(attack, what)
    if attack.damage then
        return attack.damage[what] or 0
    end
end

local function setDamage(attack, what, damage)
    attack.damage = attack.damage or {}
    attack.damage[what] = damage
end

local function applyArmor(attack)
    local healthDamage = getDamage(attack, 'health')
    if healthDamage > 0 then
        local healthDamageAdjusted = I.Combat.adjustDamageForArmor(healthDamage)
        local diff = math.floor(healthDamageAdjusted - healthDamage)
        setDamage(attack, 'health', math.max(healthDamageAdjusted, 1))
        local item = I.Combat.pickRandomArmor()
        local skillid = I.Combat.getArmorSkill(item)
        if I.SkillProgression then
            I.SkillProgression.skillUsed(skillid, {useType = I.SkillProgression.SKILL_USE_TYPES.Armor_HitByOpponent})
        end
        if item and Armor.objectIsInstance(item) then
            local attackerIsUnarmedCreature = attack.attacker and not attack.weapon and Creature.objectIsInstance(attack.attacker)
            if settings:get('unarmedCreatureAttacksDamageArmor') or not attackerIsUnarmedCreature then
                core.sendGlobalEvent('ModifyItemCondition', { actor = self, item = item, amount = diff })
            end

            if skillid == 'lightarmor' then
                core.sound.playSound3d('Light Armor Hit', self)
            elseif skillid == 'mediumarmor' then
                core.sound.playSound3d('Medium Armor Hit', self)
            elseif skillid == 'heavyarmor' then
                core.sound.playSound3d('Heavy Armor Hit', self)
            else
                core.sound.playSound3d('Hand To Hand Hit', self)
            end
        end
    end
end

local function adjustDamageForDifficulty(attack, defendant)
    local attackerIsPlayer = attack.attacker and Player.objectIsInstance(attack.attacker)
    -- The interface guarantees defendant is never nil
    local defendantIsPlayer = Player.objectIsInstance(defendant)
    -- If both characters are NPCs or both characters are players then
    -- difficulty settings do not apply
    if attackerIsPlayer == defendantIsPlayer then return end

    local fDifficultyMult = core.getGMST('fDifficultyMult')
    local difficultyTerm = core.getGameDifficulty() * 0.01
    local x = 0

    if defendantIsPlayer then
        -- Defending actor is a player
        if difficultyTerm > 0 then
            x = difficultyTerm * fDifficultyMult
        else
            x = difficultyTerm / fDifficultyMult
        end
    elseif attackerIsPlayer then
        -- Attacking actor is a player
        if difficultyTerm > 0 then
            x = -difficultyTerm / fDifficultyMult
        else
            x = -difficultyTerm * fDifficultyMult
        end
    end

    setDamage(attack, 'health', getDamage(attack, 'health') * (1 + x))
end

local function spawnBloodEffect(position)
    if isPlayer and not settings:get('spawnBloodEffectsOnPlayer') then
        return
    end

    local bloodEffectModel = string.format('Blood_Model_%d', math.random(0, 2)) -- randIntUniformClosed(0, 2)

    -- TODO: implement a Misc::correctMeshPath equivalent instead?
    -- All it ever does it append 'meshes\\' though
    bloodEffectModel = 'meshes/'..core.getGMST(bloodEffectModel)

    local record = self.object.type.record(self.object)
    local bloodTexture = string.format('Blood_Texture_%d', record.bloodType)
    bloodTexture = core.getGMST(bloodTexture)
    if not bloodTexture or bloodTexture == '' then
        bloodTexture = core.getGMST('Blood_Texture_0')
    end
    core.sendGlobalEvent('SpawnVfx', {
        model = bloodEffectModel,
        position = position,
        options = {
            mwMagicVfx = false,
            particleTextureOverride = bloodTexture,
            useAmbientLight = false,
        },
    })
end

local function onHit(data)
    for i = #onHitHandlers, 1, -1 do
        if onHitHandlers[i](data) == false then
            return -- skip other handlers
        end
    end
    if data.successful and not godMode() then
        I.Combat.applyArmor(data)
        I.Combat.adjustDamageForDifficulty(data)
        if getDamage(data, 'health') > 0 then
            core.sound.playSound3d('Health Damage', self)
            if data.hitPos then
                spawnBloodEffect(data.hitPos)
            end
        end
    elseif data.attacker and Player.objectIsInstance(data.attacker) then
        core.sound.playSound3d('miss', self)
    end
    Actor._onHit(self, data)
end

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
-- @field [parent=#AttackInfo] openmw.types#Weapon ammo (Optional) Ammo
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
        version = 0,

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
        adjustDamageForArmor = function(damage, actor) return adjustDamageForArmor(damage, actor or self) end,

        --- Calculates a difficulty multiplier based on current difficulty settings
        -- and adjusts damage accordingly. Has no effect if both this actor and the
        -- attacker are NPCs, or if both are Players.
        -- @function [parent=#Combat] adjustDamageForDifficulty
        -- @param #Attack attack The attack to adjust
        -- @param openmw.core#GameObject defendant (Optional) The defendant to make the difficulty adjustment for. Defaults to self.
        adjustDamageForDifficulty = function(attack, defendant) return adjustDamageForDifficulty(attack, defendant or self) end,

        --- Applies this character's armor to the attack. Adjusts damage, reduces item
        -- condition accordingly, progresses armor skill, and plays the armor appropriate
        -- hit sound.
        -- @function [parent=#Combat] applyArmor
        -- @param #Attack attack
        applyArmor = applyArmor,

        --- Computes this character's armor rating.
        -- Note that this interface function is read by the engine to update the UI.
        -- This function can still be overridden same as any other interface, but must not call any functions or interfaces that modify anything.
        -- @function [parent=#Combat] getArmorRating
        -- @param openmw.core#GameObject actor (Optional) The actor to calculate the armor rating for. Defaults to self.
        -- @return #number
        getArmorRating = function(actor) return getArmorRating(actor or self) end,

        --- Computes this character's armor rating.
        -- You can override this to return any skill you wish (including non-armor skills, if you so wish).
        -- Note that this interface function is read by the engine to update the UI.
        -- This function can still be overridden same as any other interface, but must not call any functions or interfaces that modify anything.
        -- @function [parent=#Combat] getArmorSkill
        -- @param openmw.core#GameObject item The item
        -- @return #string The armor skill identifier, or unarmored if the item was nil or not an instace of @{openmw.types#Armor}
        getArmorSkill = getArmorSkill,

        --- Computes the armor rating of a single piece of @{openmw.types#Armor}, adjusted for skill
        -- Note that this interface function is read by the engine to update the UI.
        -- This function can still be overridden same as any other interface, but must not call any functions or interfaces that modify anything.
        -- @function [parent=#Combat] getSkillAdjustedArmorRating
        -- @param openmw.core#GameObject item The item
        -- @param openmw.core#GameObject actor (Optional) The actor, defaults to self
        -- @return #number
        getSkillAdjustedArmorRating = function(item, actor) return getSkillAdjustedArmorRating(item, actor or self) end,

        --- Computes the effective armor rating of a single piece of @{openmw.types#Armor}, adjusted for skill and item condition
        -- @function [parent=#Combat] getEffectiveArmorRating
        -- @param openmw.core#GameObject item The item
        -- @param openmw.core#GameObject actor (Optional) The actor, defaults to self
        -- @return #number
        getEffectiveArmorRating = function(item, actor) return getEffectiveArmorRating(item, actor or self) end,

        --- Spawns a random blood effect at the given position
        -- @function [parent=#Combat] spawnBloodEffect
        -- @param openmw.util#Vector3 position
        spawnBloodEffect = spawnBloodEffect,

        --- Hit this actor. Normally called as Hit event from the attacking actor, with the same parameters.
        -- @function [parent=#Combat] onHit
        -- @param #AttackInfo attackInfo
        onHit = onHit,

        --- Picks a random armor slot and returns the item equipped in that slot.
        -- Used to pick which armor to damage / skill to increase when hit during combat.
        -- @function [parent=#Combat] pickRandomArmor
        -- @param openmw.core#GameObject actor (Optional) The actor to pick armor from, defaults to self
        -- @return openmw.core#GameObject The armor equipped in the chosen slot. nil if nothing was equipped in that slot.
        pickRandomArmor = function(actor) return pickRandomArmor(actor or self) end,

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
