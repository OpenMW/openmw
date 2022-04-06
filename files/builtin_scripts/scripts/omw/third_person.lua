local camera = require('openmw.camera')
local settings = require('openmw.settings')
local util = require('openmw.util')
local self = require('openmw.self')
local nearby = require('openmw.nearby')

local Actor = require('openmw.types').Actor

local MODE = camera.MODE
local STATE = { RightShoulder = 0, LeftShoulder = 1, Combat = 2, Swimming = 3 }

local M = {
    baseDistance = settings._getFloatFromSettingsCfg('Camera', 'third person camera distance'),
    preferredDistance = 0,
    standingPreview = false,
    noOffsetControl = 0,
}

local viewOverShoulder = settings._getBoolFromSettingsCfg('Camera', 'view over shoulder')
local autoSwitchShoulder = settings._getBoolFromSettingsCfg('Camera', 'auto switch shoulder')
local shoulderOffset = settings._getVector2FromSettingsCfg('Camera', 'view over shoulder offset')
local zoomOutWhenMoveCoef = settings._getFloatFromSettingsCfg('Camera', 'zoom out when move coef')

local defaultShoulder = (shoulderOffset.x > 0 and STATE.RightShoulder) or STATE.LeftShoulder
local rightShoulderOffset = util.vector2(math.abs(shoulderOffset.x), shoulderOffset.y)
local leftShoulderOffset = util.vector2(-math.abs(shoulderOffset.x), shoulderOffset.y)
local combatOffset = util.vector2(0, 15)

local state = defaultShoulder

local rayOptions = {collisionType = nearby.COLLISION_TYPE.Default - nearby.COLLISION_TYPE.Actor}
local function ray(from, angle, limit)
    local to = from + util.transform.rotateZ(angle) * util.vector3(0, limit, 0)
    local res = nearby.castRay(from, to, rayOptions)
    if res.hit then
        return (res.hitPos - from):length()
    else
        return limit
    end
end

local function trySwitchShoulder()
    local limitToSwitch = 120  -- switch to other shoulder if wall is closer than this limit
    local limitToSwitchBack = 300  -- switch back to default shoulder if there is no walls at this distance

    local pos = camera.getTrackedPosition()
    local rayRight = ray(pos, camera.getYaw() + math.rad(90), limitToSwitchBack + 1)
    local rayLeft = ray(pos, camera.getYaw() - math.rad(90), limitToSwitchBack + 1)
    local rayRightForward = ray(pos, camera.getYaw() + math.rad(30), limitToSwitchBack + 1)
    local rayLeftForward = ray(pos, camera.getYaw() - math.rad(30), limitToSwitchBack + 1)

    local distRight = math.min(rayRight, rayRightForward)
    local distLeft = math.min(rayLeft, rayLeftForward)

    if distLeft < limitToSwitch and distRight > limitToSwitchBack then
        state = STATE.RightShoulder
    elseif distRight < limitToSwitch and distLeft > limitToSwitchBack then
        state = STATE.LeftShoulder
    elseif distRight > limitToSwitchBack and distLeft > limitToSwitchBack then
        state = defaultShoulder
    end
end

local function calculateDistance(smoothedSpeed)
    local smoothedSpeedSqr = smoothedSpeed * smoothedSpeed
    return (M.baseDistance + math.max(camera.getPitch(), 0) * 50
            + smoothedSpeedSqr / (smoothedSpeedSqr + 300*300) * zoomOutWhenMoveCoef)
end

local noThirdPersonLastFrame = true

local function updateState()
    local mode = camera.getMode()
    local oldState = state
    if Actor.stance(self) ~= Actor.STANCE.Nothing and mode == MODE.ThirdPerson then
        state = STATE.Combat
    elseif Actor.isSwimming(self) then
        state = STATE.Swimming
    elseif oldState == STATE.Combat or oldState == STATE.Swimming then
        state = defaultShoulder
    elseif not state then
        state = defaultShoulder
    end
    if autoSwitchShoulder and (mode == MODE.ThirdPerson or state ~= oldState or noThirdPersonLastFrame)
       and (state == STATE.LeftShoulder or state == STATE.RightShoulder) then
        trySwitchShoulder()
    end
    if oldState ~= state or noThirdPersonLastFrame then
        -- State was changed, start focal point transition.
        if mode == MODE.Vanity then
            -- Player doesn't touch controls for a long time. Transition should be very slow.
            camera.setFocalTransitionSpeed(0.2)
        elseif (oldState == STATE.Combat or state == STATE.Combat) and
               (mode ~= MODE.Preview or M.standingPreview) then
            -- Transition to/from combat mode and we are not in preview mode. Should be fast.
            camera.setFocalTransitionSpeed(5.0)
        else
            camera.setFocalTransitionSpeed(1.0)  -- Default transition speed.
        end

        if state == STATE.RightShoulder then
            camera.setFocalPreferredOffset(rightShoulderOffset)
        elseif state == STATE.LeftShoulder then
            camera.setFocalPreferredOffset(leftShoulderOffset)
        else
            camera.setFocalPreferredOffset(combatOffset)
        end
    end
end

function M.update(dt, smoothedSpeed)
    local mode = camera.getMode()
    if mode == MODE.FirstPerson or mode == MODE.Static then
        noThirdPersonLastFrame = true
        return
    end
    if not viewOverShoulder then
        M.preferredDistance = M.baseDistance
        camera.setPreferredThirdPersonDistance(M.baseDistance)
        noThirdPersonLastFrame = false
        return
    end

    if M.noOffsetControl == 0 then
        updateState()
    else
        state = nil
    end

    M.preferredDistance = calculateDistance(smoothedSpeed)
    if noThirdPersonLastFrame then  -- just switched to third person view
        camera.setPreferredThirdPersonDistance(M.preferredDistance)
        camera.instantTransition()
        noThirdPersonLastFrame = false
    else
        local maxIncrease = dt * (100 + M.baseDistance)
        camera.setPreferredThirdPersonDistance(math.min(
            M.preferredDistance, camera.getThirdPersonDistance() + maxIncrease))
    end
end

return M

