local self = require('openmw.self')
local types = require('openmw.types')
local util = require('openmw.util')
local core = require('openmw.core')
local animation = require('openmw.animation')
local I = require('openmw.interfaces')
local auxUtil = require('openmw_aux.util')
local common = require('scripts.omw.spellcasting.common')
local Actor = types.Actor
local Item = types.Item
local isPlayer = types.Player.objectIsInstance(self)
local actorSpells = Actor.spells(self)

local interface = auxUtil.shallowCopy(I.SpellCasting)

I.SpellCasting.addApplyMagicEffectsHandler(function(options)
    if Actor.isDead(self) then return end
    local id = Actor.activeSpells(self):add(options)
    if id then
        -- It takes a frame for a spell to actually be applied, meaning we cannot yet tell
        -- what effects have been resisted, or did not apply, and therefore what VFX need to be rendered/skipped.
        -- so instead of invoking the function directly, we wait a frame using an event
        self:sendEvent('PlayOnHitEffects', {activeSpellId = id})
    end
end)

local function onPlayOnHitEffects(options)
    local aSpell = common.findActiveSpell(self, options.activeSpellId)
    if aSpell then
        common.playMagicEffects(self, 'hit', aSpell.effects, true)
    end
end

return {
    interfaceName = 'SpellCasting',
    interface = interface,

    eventHandlers = {
        PlayOnHitEffects = onPlayOnHitEffects,
    },
}
