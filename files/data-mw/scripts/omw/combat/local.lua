local core = require('openmw.core')
local I = require('openmw.interfaces')
local self = require('openmw.self')
local types = require('openmw.types')
local Actor = types.Actor
local Player = types.Player
local isPlayer = Player.objectIsInstance(self)

local godMode = function() return false end
if isPlayer then
    -- openmw.debug is only allowed on player scripts
    godMode = function() return require('openmw.debug').isGodMode() end
end

local function getDamage(attack, what)
    if attack.damage then
        return attack.damage[what] or 0
    end
end

local function onHit(data)
    if data.successful and not godMode() then
        I.Combat.applyArmor(data)
        I.Combat.adjustDamageForDifficulty(data)
        if getDamage(data, 'health') > 0 then
            core.sound.playSound3d('Health Damage', self)
            if data.hitPos then
                I.Combat.spawnBloodEffect(data.hitPos)
            end
        end
    elseif data.attacker and Player.objectIsInstance(data.attacker) then
        core.sound.playSound3d('miss', self)
    end
    Actor._onHit(self, data)
end

I.Combat.addOnHitHandler(onHit)
