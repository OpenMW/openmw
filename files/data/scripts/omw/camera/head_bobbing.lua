local camera = require('openmw.camera')
local self = require('openmw.self')
local util = require('openmw.util')
local async = require('openmw.async')
local storage = require('openmw.storage')

local Actor = require('openmw.types').Actor

local M = {}

local settings = storage.playerSection('SettingsOMWCameraHeadBobbing')

local doubleStepLength, stepHeight, maxRoll

local function updateSettings()
    M.enabled = settings:get('enabled')
    doubleStepLength = settings:get('step') * 2
    stepHeight = settings:get('height')
    maxRoll = math.rad(settings:get('roll'))
end

updateSettings()
settings:subscribe(async:callback(updateSettings))

local effectWeight = 0
local totalMovement = 0

-- Trajectory of each step is a scaled arc of 60 degrees.
local halfArc = math.rad(30)
local sampleArc = function(x) return 1 - math.cos(x * halfArc) end
local arcHeight = sampleArc(1)

function M.update(dt, smoothedSpeed)
    local speed = Actor.getCurrentSpeed(self)
    speed = speed / (1 + speed / 500) -- limit bobbing frequency if the speed is very high
    totalMovement = totalMovement + speed * dt
    if not M.enabled or camera.getMode() ~= camera.MODE.FirstPerson then
        effectWeight = 0
        return
    end
    if Actor.isOnGround(self) then
        effectWeight = math.min(1, effectWeight + dt * 5)
    else
        effectWeight = math.max(0, effectWeight - dt * 5)
    end

    local doubleStepState = totalMovement / doubleStepLength
    doubleStepState = doubleStepState - math.floor(doubleStepState) -- from 0 to 1 during 2 steps
    local stepState = math.abs(doubleStepState * 4 - 2) - 1         -- from -1 to 1 on even steps and from 1 to -1 on odd steps
    local effect = sampleArc(stepState) / arcHeight                 -- range from 0 to 1

    -- Smoothly reduce the effect to zero when the player stops
    local coef = math.min(smoothedSpeed / 300, 1) * effectWeight

    local zOffset = (0.5 - effect) * coef * stepHeight                   -- range from -stepHeight/2 to stepHeight/2
    local roll = ((stepState > 0 and 1) or -1) * effect * coef * maxRoll -- range from -maxRoll to maxRoll
    camera.setFirstPersonOffset(camera.getFirstPersonOffset() + util.vector3(0, 0, zOffset))
    camera.setExtraRoll(camera.getExtraRoll() + roll)
end

return M
