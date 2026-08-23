local core = require('openmw.core')
local I = require('openmw.interfaces')
local self = require('openmw.self')
local storage = require('openmw.storage')
local types = require('openmw.types')
local Actor = types.Actor
local Player = types.Player
local Creature = types.Creature
local Armor = types.Armor
local auxUtil = require('openmw_aux.util')
local isPlayer = Player.objectIsInstance(self)
local common = require('scripts.omw.combat.common')

local godMode = function() return false end
if isPlayer then
    -- openmw.debug is only allowed on player scripts
    godMode = function() return require('openmw.debug').isGodMode() end
end

local function applyArmor(attack)
    local healthDamage = common.getDamage(attack, 'health')
    if healthDamage > 0 then
        local healthDamageAdjusted = I.Combat.adjustDamageForArmor(healthDamage)
        local diff = math.floor(healthDamageAdjusted - healthDamage)
        common.setDamage(attack, 'health', math.max(healthDamageAdjusted, 1))
        local item = I.Combat.pickRandomArmor()
        local skillid = I.Combat.getArmorSkill(item)
        if I.SkillProgression then
            I.SkillProgression.skillUsed(skillid, {useType = I.SkillProgression.SKILL_USE_TYPES.Armor_HitByOpponent})
        end
        if item and Armor.objectIsInstance(item) then
            local attackerIsUnarmedCreature = attack.attacker and not attack.weapon and not attack.ammo and Creature.objectIsInstance(attack.attacker)
            if common.settings:get('unarmedCreatureAttacksDamageArmor') or not attackerIsUnarmedCreature then
                core.sendGlobalEvent('ModifyItemCondition', { actor = self, item = item, amount = diff })
            end

            if attack.muteSound then
                return
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

local function spawnBloodEffect(position)
    if isPlayer and not common.settings:get('spawnBloodEffectsOnPlayer') then
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

local function applyStagger(attack, rawHealthDamage)
    if common.hasDamage(attack) and attack.attacker ~= nil then
        local agilityTerm = Actor.stats.attributes.agility(self).modified * core.getGMST('fKnockDownMult')
        local knockdownTerm = (
            Actor.stats.attributes.agility(self).modified
            * core.getGMST('iKnockDownOddsMult')
            * 0.01
            + core.getGMST('iKnockDownOddsBase')
        )
        local roll = math.random(0,99)
        if rawHealthDamage > 0 and agilityTerm <= rawHealthDamage and knockdownTerm <= roll then
            Actor.setKnockedDown(self, true)
        else
            Actor.setHitRecovery(self, true)
        end
    end
end

local function onHit(data)
    if data.successful and not godMode() then
        local rawHealthDamage = common.getDamage(data, 'health')
        if not data.ignoreArmor then
            I.Combat.applyArmor(data)
        end
        if not data.ignoreDifficulty then
            I.Combat.adjustDamageForDifficulty(data)
        end
        if common.getDamage(data, 'health') > 0 then
            if not data.muteSound then
                core.sound.playSound3d('Health Damage', self)
            end
            if data.hitPos then
                I.Combat.spawnBloodEffect(data.hitPos)
            end
        end
        if not data.ignoreStagger then
            I.Combat.applyStagger(data, rawHealthDamage)
        end
    elseif data.attacker and not data.muteSound and Player.objectIsInstance(data.attacker) then
        core.sound.playSound3d('miss', self)
    end
    Actor._onHit(self, data)
end

I.Combat.addOnHitHandler(onHit)

local interface = auxUtil.shallowCopy(I.Combat)
interface.adjustDamageForArmor = function(damage, actor) return common.adjustDamageForArmor(damage, actor or self) end
interface.adjustDamageForDifficulty = function(attack, defendant) return common.adjustDamageForDifficulty(attack, defendant or self) end
interface.applyArmor = applyArmor
interface.applyStagger = applyStagger
interface.getArmorRating = function(actor) return common.getArmorRating(actor or self) end
interface.getArmorSkill = common.getArmorSkill
interface.getSkillAdjustedArmorRating = function(itemOrId, actor) return common.getSkillAdjustedArmorRating(itemOrId, actor or self) end
interface.getEffectiveArmorRating = function(item, actor) return common.getEffectiveArmorRating(item, actor or self) end
interface.spawnBloodEffect = spawnBloodEffect
interface.pickRandomArmor = function(actor) return common.pickRandomArmor(actor or self) end

return {
    interfaceName = 'Combat',
    interface = interface
}
