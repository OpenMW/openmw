local types = require('openmw.types')
local animation = require('openmw.animation')
local Lockable = types.Lockable
local NPC = types.NPC
local Player = types.Player
local core = require('openmw.core')
local self = require('openmw.self')
local common = require('scripts.omw.spellcasting.common')
local nearby = require('openmw.nearby')
local l10n = core.l10n('OMWEngine')

local function isAllowedToOpen(player)
    local owner = self.object.owner
    local isOwned = false
    local isFactionOwned = false
    if owner.recordId and owner.recordId ~= player.recordId then
        isOwned = true
    end

    local faction = owner.factionId
    if faction then
        local rank = NPC.getFactionRank(player, owner.factionId)
        if rank == 0 then
            -- Not in faction.
            isFactionOwned = true
        else
            isFactionOwned = rank < (owner.factionRank or 0)
        end
    end

    -- TODO: global variables

    return not(isFactionOwned or isOwned)
end

local function findActor(recordId)
    for _, actor in pairs(nearby.actors) do
        if actor.recordId == recordId then
            return actor
        end
    end
end

local function unlockAttempted(actor)
    if Player.objectIsInstance(actor) and not isAllowedToOpen(actor) then
        core.sendGlobalEvent('CommitCrime',{
            player = actor,
            type = Player.OFFENSE_TYPE.Trespassing,
            faction = self.object.owner.factionId,
            victim = findActor(self.object.owner.recordId),
        })
    end
end

local function onApplyMagicEffects(options)
    -- Non-actors do not have an active spell store so we have to manually
    -- apply the effects of open/lock spells here.
    local effects = common.getMagicRecord(options.id).effects

    -- Lockables are only affected by Lock/Unlock spells
    -- Lock state from global scripts, so we need to use events to affect lock state.
    -- This gives a 1 frame delay, so to keep track of the current lock state we have to track it locally
    local lockLevel = Lockable.getLockLevel(self)
    local isLocked = Lockable.isLocked(self)
    local playEffects = {}
    local unlockAttempt = false
    for _, effect in ipairs(effects) do
        if effect.id == core.magic.EFFECT_TYPE.Lock then
            playEffects[#playEffects + 1] = effect
            local magnitude = math.random(effect.magnitudeMin, effect.magnitudeMax)
            if magnitude >= lockLevel or not isLocked then
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
            local magnitude = math.random(effect.magnitudeMin, effect.magnitudeMax)
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

    for _, effect in pairs(playEffects) do
        local mgef = core.magic.effects.records[effect.id]
        animation.addGlow(self, {color = mgef.color, duration = 1.5})
    end

    if #playEffects > 0 then
        common.playMagicEffects(self, 'hit', playEffects)
    end

    local crimeActor = options.caster
    if not options.noVanillaCompatibleCrime then
        crimeActor = nearby.players[1]
    end
    if unlockAttempt and crimeActor then
        unlockAttempted(crimeActor)
    end
end

return {
    eventHandlers = {
        ApplyMagicEffects =  onApplyMagicEffects,
        AddGlow = function(options) animation.addGlow(self, options) end,
    },
}
