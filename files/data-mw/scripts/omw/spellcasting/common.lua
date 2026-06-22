local core = require('openmw.core')
local types = require('openmw.types')
local Actor = types.Actor
local Armor = types.Armor
local Book = types.Book
local Clothing = types.Clothing
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

    print('Warning: No such spell: '..tostring(id))

end

local function playMagicEffects(target, type, effects, allowEffectLoop)
    local addedStatic = {}
    local addedSounds = {}
    for _, effect in pairs(effects) do
        -- MagicEffectWithParams includes its mgef record directly as effect.effect
        -- But ActiveSpellEffect does not so we need to use the id instead.
        local mgef = core.magic.effects.records[effect.id]
        local school = core.stats.Skill.record(mgef.school).school

        local sound = mgef[type..'Sound'] or school[type..'Sound']
        if sound and not addedSounds[sound] then
            addedSounds[sound] = true
            core.sendGlobalEvent('PlaySound3d', {sound = sound, position = target})
        end

        local static = mgef[type..'Static'] or ('vfx_default'..type)
        local loop = mgef.continuousVfx and allowEffectLoop

        if not addedStatic[static] then
            addedStatic[static] = true
            local static = types.Static.records[static]
            local model = static and static.model or nil
            if model then
                local eventParams = {
                    model = model,
                    options = {
                        vfxId = mgef.id,
                        particleTextureOverride = mgef.particle,
                        loop = loop,
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
        if not Actor.objectIsInstance(target) then
            target:sendEvent('AddGlow', {
                color = mgef.color,
                duration = 1.5
            })
        end
    end
end

local function findActiveSpell(target, activeSpellId)
    for k, v in pairs(Actor.activeSpells(target)) do
        if v.activeSpellId == activeSpellId then
            return v
        end
    end
end

local function targetIsValid(target)
        -- Spells can only be inflicted on Actors and Lockables.
    return target ~= nil and (Actor.objectIsInstance(target) or Lockable.objectIsInstance(target))
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
    findActiveSpell = findActiveSpell,
    filterByIndex = filterByIndex,
}
