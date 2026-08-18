local types = require('openmw.types')
local Actor = types.Actor
local Lockable = types.Lockable
local I = require('openmw.interfaces')
local world = require('openmw.world')
local core = require('openmw.core')
local auxUtil = require('openmw_aux.util')
local common = require('scripts.omw.spellcasting.common')

I.Projectiles.addOnProjectileHitHandler(I.Projectiles.TYPES.Magic, function(projectile, hitResult)
    if not hitResult.hit then return end
    assert(projectile.spellCast)
    local spellCast = projectile.spellCast
    local target = hitResult.hitObject
    local haveValidTarget = common.targetIsValid(target)

    if haveValidTarget then
        I.SpellCasting.inflict(spellCast, target, core.magic.RANGE.Target)
    end

    local record = common.getMagicRecord(spellCast.id)
    local effects = common.filterByRange(record.effects, core.magic.RANGE.Target)
    for _, effect in pairs(effects) do
        if not (effect.area > 0 or haveValidTarget) then
            -- Non-aoe effects spawn a magical orb when the magic bolt misses
            local mgef = effect.effect
            local static = mgef.areaStatic
            if static == nil or static == "" then
                static = "VFX_DefaultArea"
            end
            core.sendGlobalEvent('SpawnVfx', {
                model = types.Static.record(static).model,
                position = hitResult.hitPos,
                options = { particleTextureOverride = mgef.particle }
            })
        end
    end

    if common.hasAoe(effects) then
        local options = {
            position = hitResult.hitPos,
            range = core.magic.RANGE.Target,
            ignore = {},
        }
        if haveValidTarget then
            options.ignore[target.id] = true
        end
        I.SpellCasting.explodeSpell(spellCast, options)
    end
end)

local function explodeEffect(position, effect)
    local mgef = effect.effect
    local areaStatic = mgef.areaStatic
    if areaStatic == nil or areaStatic == "" then
        areaStatic = "VFX_DefaultArea"
    end

    world.vfx.spawn(types.Static.record(areaStatic).model, position, {
        particleTextureOverride = mgef.particle,
        scale = effect.area * 2,
    })

    local areaSound = mgef.areaSound
    if areaSound == nil or areaSound == "" then
        local school = core.stats.Skill.record(mgef.school).school
        areaSound = school.areaSound
    end
    core.sendGlobalEvent('PlaySound3d', {sound = areaSound, position = position})
end

local function explodeSpell(spellCast, options)
    -- Generates a list of affected actors and sends events their way
    local record = common.getMagicRecord(spellCast.id)
    local effects = common.filterByRange(record.effects, options.range)
    if not effects or #effects == 0 then
        return
    end

    local position = options.position
    if not position then
        if spellCast.target then
            position = spellCast.target.position
        else
            print('Warning: Tried to explode a spell with no target or position')
            return
        end
    end

    local casterIsActor = spellCast.caster and Actor.objectIsInstance(spellCast.caster)

    -- Make a shallow copy to avoid modifying the input table
    local ignore = auxUtil.shallowCopy(options.ignore or {})
    if spellCast.caster then
        ignore[spellCast.caster.id] = true
    end

    local applicableEffects = {}
    local maxArea = 0
    -- First generate a list of applicable effects and compute maxArea
    for _, effect in pairs(effects) do
        if effect.area > 0 and (casterIsActor or not effect.casterLinked) then
            applicableEffects[#applicableEffects + 1] = effect
            maxArea = math.max(effect.area, maxArea)
            explodeEffect(position, effect)
        end
    end
    if #applicableEffects == 0 then
        return false
    end

    -- If maxArea == 0, this means there was no AOE so we can exit early.
    if maxArea <= 0 then return end

    -- Get all objects within maxArea
    local objectsInRange = world.getObjectsInRange(position, maxArea * common.UnitsPerFoot)
    local eventOptions = auxUtil.shallowCopy(spellCast)

    -- For each object compute each effect in range and apply
    for _, object in pairs(objectsInRange) do
        if common.targetIsValid(object) and not ignore[object.id] then
            local effectIndexes = {}
            local distance = (object.position - position):length() / common.UnitsPerFoot
            for _, effect in pairs(applicableEffects) do
                if distance < effect.area then
                    effectIndexes[#effectIndexes + 1] = effect.index
                end
            end
            effectIndexes = common.filterApplicableEffects(effectIndexes, record.effects, object)
            if #effectIndexes > 0 then
                eventOptions.effects = effectIndexes
                object:sendEvent('ApplyMagicEffects', eventOptions)
            end
        end
    end
end

local interface = auxUtil.shallowCopy(I.SpellCasting)
interface.explodeSpell = explodeSpell
interface.inflict = common.inflict

return {
    interfaceName = 'SpellCasting',
    interface = interface,
    eventHandlers = {
        ExplodeSpell = function(data)
            I.SpellCasting.explodeSpell(data.spellCast, data.options)
        end
    }
}
