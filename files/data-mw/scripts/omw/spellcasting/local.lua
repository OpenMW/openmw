local async = require('openmw.async')
local core = require('openmw.core')
local self = require('openmw.self')
local types = require('openmw.types')
local I = require('openmw.interfaces')
local auxUtil = require('openmw_aux.util')
local common = require('scripts.omw.spellcasting.common')
local Actor = types.Actor

local interface = auxUtil.shallowCopy(I.SpellCasting)
interface.inflict = common.inflict

I.SpellCasting.addApplyMagicEffectsHandler(function(options)
    if Actor.isDeathFinished(self) then return end
    local id = Actor.activeSpells(self):add(options)
    if id then
        -- It takes a frame for a spell to actually be applied, meaning we cannot yet tell
        -- what effects have been resisted, or did not apply, and therefore what VFX need to be rendered/skipped.
        -- so instead of invoking the function directly, we wait a frame using an event
        self:sendEvent('PlayOnHitEffects', {activeSpellId = id})
    end
end)

-- These effects wait for non-zero dt in MWMechanics::applyMagicEffect.
local deferredEffects = {
    [core.magic.EFFECT_TYPE.Invisibility] = true,
    [core.magic.EFFECT_TYPE.Recall] = true,
    [core.magic.EFFECT_TYPE.DivineIntervention] = true,
    [core.magic.EFFECT_TYPE.AlmsiviIntervention] = true,
}
local playOnHitEffectsLater

local function onPlayOnHitEffects(options)
    local paused = core.isWorldPaused()
    local aSpell = Actor.activeSpells(self):getByActiveSpellId(options.activeSpellId)

    if aSpell then
        local effects = auxUtil.mapFilter(aSpell.effects, function(effect)
            if deferredEffects[effect.id] then return not paused end
            return not options.deferredOnly
        end)
        common.playMagicEffects(self, 'hit', effects, true)
    end

    if paused then
        -- Some effects may still be waiting for a mechanics update.
        -- Simulation timers only fire once the world is unpaused.
        async:newSimulationTimer(0, playOnHitEffectsLater, {
            activeSpellId = options.activeSpellId,
            deferredOnly = aSpell ~= nil,
        })
    end
end

playOnHitEffectsLater = async:registerTimerCallback('PlayOnHitEffects', onPlayOnHitEffects)

return {
    interfaceName = 'SpellCasting',
    interface = interface,

    eventHandlers = {
        PlayOnHitEffects = onPlayOnHitEffects,
    },
}
