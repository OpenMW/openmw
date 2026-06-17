local I = require('openmw.interfaces')
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

        local sound = mgef[type..'Sound']
        if not sound or sound == '' then sound = school[type..'Sound'] end
        if sound and not addedSounds[sound] then
            addedSounds[sound] = true
            core.sendGlobalEvent('PlaySound3d', {sound = sound, position = target})
        end

        local static = mgef[type..'Static']
        local loop = mgef.continuousVfx and allowEffectLoop

        if not static or static == '' then static = 'vfx_default'..type end
        if not addedStatic[static] then
            addedStatic[static] = true
            local eventParams = {
                model = types.Static.record(static).model,
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

return {
    playMagicEffects = playMagicEffects,
    getMagicRecord = getMagicRecord,
    findActiveSpell = findActiveSpell,
}
