local types = require('openmw.types')
local animation = require('openmw.animation')
local Lockable = types.Lockable
local Player = types.Player
local core = require('openmw.core')
local self = require('openmw.self')
local common = require('scripts.omw.spellcasting.common')
local nearby = require('openmw.nearby')
local auxUtil = require('openmw_aux.util')
local l10n = core.l10n('Mechanics')

local isOrganic = common.isOrganicContainer(self)

local function unlockAttempted(actor)
    if Player.objectIsInstance(actor) then
        core.sendGlobalEvent('UnlockAttempted',{
            player = actor,
            lockable = self.object,
        })
    end
end

local function onApplyMagicEffects(options)
    assert(options.id)
    local record = common.getMagicRecord(options.id)
    assert(record)
    local effects = common.filterByIndex(record.effects, options.effects)

    -- Lockables are only affected by Lock/Unlock spells, so we ignore all other effects.
    -- Lock state can only be changed from global scripts, so we need to use events to affect lock state.
    -- This gives a 1 frame delay, so for spells with multiple lock/unlock effects we need to track the state
    -- locally for later effects to see the correct lock state.
    local lockLevel = Lockable.getLockLevel(self)
    local isLocked = Lockable.isLocked(self)
    local playEffects = {}
    local unlockAttempt = false
    for _, effect in ipairs(effects) do
        if effect.id == core.magic.EFFECT_TYPE.Lock then
            playEffects[#playEffects + 1] = effect
            local magnitude = auxUtil.random(effect.magnitudeMin, effect.magnitudeMax)
            if magnitude > lockLevel or not isLocked then
                lockLevel = magnitude
                isLocked = true
                core.sendGlobalEvent('Lock', {target = self, magnitude = magnitude})
                core.sound.playSound3d('Open Lock', self)
                if options.caster then
                    options.caster:sendEvent('ShowMessage', { message = l10n'MagicLockSuccess' })
                end
            end
        end
        if effect.id == core.magic.EFFECT_TYPE.Open then
            playEffects[#playEffects + 1] = effect
            unlockAttempt = true
            local magnitude = auxUtil.random(effect.magnitudeMin, effect.magnitudeMax)
            if magnitude >= lockLevel and isLocked then
                lockLevel = 0
                isLocked = false
                core.sendGlobalEvent('Unlock', {target = self})
                core.sound.playSound3d('Open Lock', self)
                if options.caster then
                    options.caster:sendEvent('ShowMessage', { message = l10n'MagicOpenSuccess' })
                end
            elseif isLocked then
                core.sound.playSound3d('Open Lock Fail', self)
            end
        end
    end

    local crimeActor = options.caster
    if not options.noVanillaCompatibleCrime then
        crimeActor = nearby.players[1]
    end
    if unlockAttempt and crimeActor then
        unlockAttempted(crimeActor)
    end

    if #playEffects > 0 then
        common.playMagicEffects(self, 'hit', playEffects)
    end
end

local ApplyMagicEffectsHandler = nil
if not isOrganic then
    ApplyMagicEffectsHandler = onApplyMagicEffects
end

return {
    eventHandlers = {
        ApplyMagicEffects = ApplyMagicEffectsHandler,
        AddGlow = function(options) animation.addGlow(self, options) end,
    },
}
