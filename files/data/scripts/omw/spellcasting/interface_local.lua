local auxUtil = require('openmw_aux.util')
local core = require('openmw.core')
local I = require('openmw.interfaces')

---
-- Table describing a spell cast
-- @type SpellCastInfo
-- @field #string id Record ID of a spell, enchantment, enchanted item, potion, or ingredient
-- @field openmw.core#GameObject caster (Optional) The caster
-- @field openmw.core#GameObject target (Optional) The spell target (Normally nil, can be set to force a spell to hit the given target)
-- @field openmw.types#Item item (Optional) The enchanted item

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

        --- (Local only) Apply new magic effects on this actor.
        -- Invokes handlers added via addApplyMagicEffectHandler. The default handler adds effects as a new active spell using Actor.activeSpells(self):add(options),
        -- and handles spawning appropriate VFX. Including looping effects for magic effects like shield.
        -- @function [parent=#SpellCasting] applyMagicEffects
        -- @param #function options The options. Will be passed as-is to Actor.activeSpells(self):add(options) by the built-in handler.
        applyMagicEffects = function(options)
            auxUtil.callEventHandlers(applyMagicEffectsHandlers, options)
        end,

        --- (Local only) Add a new applyMagicEffects handler for this actor. Receives as its options the parameters that were passed to @{#applyMagicEffects}.
        -- If `applyMagicEffects` is invoked as the result of spell reflection, an `isReflect` parameter will be added to the options and set to true.
        -- @function [parent=#SpellCasting] addApplyMagicEffectsHandler
        -- @param #function handler The handler.
        addApplyMagicEffectsHandler = function(handler)
            applyMagicEffectsHandlers[#applyMagicEffectsHandlers+1] = handler
        end,

        --- (Local and Global) Explodes a spell at the given location
        -- @function [parent=#SpellCasting] explodeSpell
        -- @param #SpellCastInfo spellCast The spell info
        -- @param #table options Explosion options
        --
        --   * `position` - @{openmw.util#vector3} world position of the explosion
        --   * `range` - @{openmw.core#SpellRange} Which effects (self, touch, target) to consider. If not set, all effects with aoe will explode.
        --   * `ignore` - @{#set<#string>} Set of unique ids (see @{openmw.core#GameObject.id}) of objects to be ignored by aoe.
        explodeSpell = function(spellCast, options)
            core.sendGlobalEvent('ExplodeSpell', {spellCast = spellCast, options = options})
        end,

        --- (Local and Global) Inflicts a spell on the target following vanilla rules, such as filtering by range, and policing recastable effects. Note that this only
        -- inflicts effects on the target, and does not explode the spell even if it has AOE effects.
        -- @function [parent=#SpellCasting] inflict
        -- @param #SpellCastInfo spellCast
        -- @param openmw.core#GameObject target Must be either a @{openmw.types#Actor} or @{openmw.types#Lockable}
        -- @param openmw.core#SpellRange range
        inflict = function(spellCast, target, range)
        end,
    },

    eventHandlers = {
        ApplyMagicEffects = function(options) I.SpellCasting.applyMagicEffects(options) end,
    },
}
