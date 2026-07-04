local auxUtil = require('openmw_aux.util')
local I = require('openmw.interfaces')

--- Basic spell casting interface
-- @module SpellCasting
-- @usage require('openmw.interfaces').SpellCasting

local applyMagicEffectsHandlers = {}

return {
    interfaceName = 'SpellCasting',
    interface = {
        --- Interface version
        -- @field [parent=#SpellCasting] #number version
        version = 0,

        --- Apply new magic effects on this actor.
        -- Invokes handlers added via addApplyMagicEffectHandler. The default handler adds effects as a new active spell using Actor.activeSpells(self):add(options),
        -- and handles spawning appropriate VFX. Including looping effects for magic effects like shield.
        -- @function [parent=#SpellCasting] applyMagicEffects
        -- @param #function options The options. Will be passed as-is to Actor.activeSpells(self):add(options) by the built-in handler.
        applyMagicEffects = function(options)
            auxUtil.callEventHandlers(applyMagicEffectsHandlers, options)
        end,

        --- Add a new applyMagicEffects handler for this actor. Receives as its options the parameters that were passed to @{#applyMagicEffects}.
        -- If `applyMagicEffects` is invoked as the result of spell reflection, an `isReflect` parameter will be added to the options and set to true.
        -- @function [parent=#SpellCasting] addApplyMagicEffectsHandler
        -- @param #function handler The handler.
        addApplyMagicEffectsHandler = function(handler)
            applyMagicEffectsHandlers[#applyMagicEffectsHandlers+1] = handler
        end,
    },

    eventHandlers = {
        ApplyMagicEffects = function(options) I.SpellCasting.applyMagicEffects(options) end,
    },
}
