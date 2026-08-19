local core = require('openmw.core')
local types = require('openmw.types')
local Actor = types.Actor
local Armor = types.Armor
local Book = types.Book
local Clothing = types.Clothing
local Container = types.Container
local Ingredient = types.Ingredient
local Lockable = types.Lockable
local Potion = types.Potion
local Weapon = types.Weapon

-- Translate the given ID into a magic record, meaning a record with an .effects member.
-- For example, if id is the record ID of an armor, this will find the corresponding ESM3_Enchantment record
local function getMagicRecord(id)
    -- Try spell/enchantment tables
    local record = core.magic.spells.records[id]
    if record then
        return record
    end
    record = core.magic.enchantments.records[id]
    if record then
        return record
    end

    -- Enchanted items
    for _, enchantable in pairs({ Armor, Weapon, Clothing, Book }) do
        record = enchantable.record(id)
        if record and record.enchant then
            return core.magic.enchantments.records[record.enchant]
        end
    end

    -- Try potion/ingredient
    record = Potion.record(id)
    if record then
        return record
    end
    record = Ingredient.record(id)
    if record then
        return record
    end

    print('Warning: No such magic item: '..tostring(id))

end

local function getMagicEffectSound(mgef, type)
    local sound = mgef[type..'Sound']
    if not sound and mgef.school then
        local skill = core.stats.Skill.record(mgef.school)
        if skill and skill.school then
            return skill.school[type..'Sound']
        end
    end
    return sound
end

local function playMagicEffectVfx(mgef, staticId, target, allowEffectLoop)
    local static = types.Static.records[staticId]
    if static and static.model then
        local eventParams = {
            model = static.model,
            options = {
                vfxId = mgef.id,
                particleTextureOverride = mgef.particle,
                loop = mgef.continuousVfx and allowEffectLoop,
            }
        }
        if Actor.objectIsInstance(target) then
            target:sendEvent('AddVfx', eventParams)
        else
            eventParams.position = target.position
            core.sendGlobalEvent('SpawnVfx', eventParams)
        end
    end
end

local function playMagicEffects(target, type, effects, allowEffectLoop)
    local addedStatic = {}
    local addedSounds = {}
    for _, effect in pairs(effects) do
        -- MagicEffectWithParams includes its mgef record directly as effect.effect
        -- But ActiveSpellEffect does not so we need to use the id instead.
        local mgef = core.magic.effects.records[effect.id]

        local sound = getMagicEffectSound(mgef, type)
        if sound and not addedSounds[sound] then
            addedSounds[sound] = true
            core.sendGlobalEvent('PlaySound3d', {sound = sound, position = target})
        end

        local staticId = mgef[type..'Static'] or ('vfx_default'..type)
        if not addedStatic[staticId] then
            addedStatic[staticId] = true
            playMagicEffectVfx(mgef, staticId, target, allowEffectLoop)
        end

        if not Actor.objectIsInstance(target) then
            target:sendEvent('AddGlow', {
                color = mgef.color,
                duration = 1.5
            })
        end
    end
end

local function isOrganicContainer(target)
    if Container.objectIsInstance(target) then
        local record = Container.records[target.recordId]
        return record and record.isOrganic
    end
    return false
end

local function targetIsValid(target)
    if not target then return false end
    -- Spells can only be inflicted on Actors in processing range, and Lockables.
    local isValidActor = Actor.objectIsInstance(target) and Actor.isInActorsProcessingRange(target)
    return isValidActor or (Lockable.objectIsInstance(target) and not isOrganicContainer(target))
end


local function findByIndex(effects, index)
    if not effects then return nil end
    for _, effect in pairs(effects) do
        if effect.index == index then return effect end
    end
    return nil
end

local function filterByIndex(effects, filter)
    if not filter or #filter == 0 then return effects end
    local effectsByIndex = {}
    for _, index in pairs(filter) do
        local effect = findByIndex(effects, index)
        if effect then
            effectsByIndex[#effectsByIndex + 1] = effect
        else
            print('Warning: Spell has no such effect [index='..tostring(index)..']')
        end
    end
    return effectsByIndex
end


return {
    playMagicEffects = playMagicEffects,
    getMagicRecord = getMagicRecord,
    filterByIndex = filterByIndex,
    isOrganicContainer = isOrganicContainer,
}
