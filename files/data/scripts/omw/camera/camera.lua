local camera = require('openmw.camera')
local core = require('openmw.core')
local input = require('openmw.input')
local util = require('openmw.util')
local self = require('openmw.self')
local nearby = require('openmw.nearby')
local async = require('openmw.async')

local Actor = require('openmw.types').Actor

local settings = require('scripts.omw.camera.settings').thirdPerson
local head_bobbing = require('scripts.omw.camera.head_bobbing')
local third_person = require('scripts.omw.camera.third_person')
local pov_auto_switch = require('scripts.omw.camera.first_person_auto_switch')
local move360 = require('scripts.omw.camera.move360')

local MODE = camera.MODE

local previewIfStandStill = false
local showCrosshairInThirdPerson = false
local slowViewChange = false

local function updateSettings()
    previewIfStandStill = settings:get('previewIfStandStill')
    showCrosshairInThirdPerson = settings:get('viewOverShoulder')
    camera.allowCharacterDeferredRotation(settings:get('deferredPreviewRotation'))
    local collisionType = util.bitAnd(nearby.COLLISION_TYPE.Default, util.bitNot(nearby.COLLISION_TYPE.Actor))
    collisionType = util.bitOr(collisionType, nearby.COLLISION_TYPE.Camera)
    if settings:get('ignoreNC') then
        collisionType = util.bitOr(collisionType, nearby.COLLISION_TYPE.VisualOnly)
    end
    camera.setCollisionType(collisionType)
    move360.enabled = settings:get('move360')
    move360.turnSpeed = settings:get('move360TurnSpeed')
    pov_auto_switch.enabled = settings:get('povAutoSwitch')
    slowViewChange = settings:get('slowViewChange')
end

local primaryMode

local noModeControl = 0
local noStandingPreview = 0
local noHeadBobbing = 0
local noZoom = 0

local function init()
    camera.setFieldOfView(camera.getBaseFieldOfView())
    if camera.getMode() == MODE.FirstPerson then
        primaryMode = MODE.FirstPerson
    else
        primaryMode = MODE.ThirdPerson
        camera.setMode(MODE.ThirdPerson)
    end
    updateSettings()
end

settings:subscribe(async:callback(updateSettings))

local smoothedSpeed = 0
local previewTimer = 0

local function updatePOV(dt)
    local switchLimit = 0.25
    if input.isActionPressed(input.ACTION.TogglePOV) and input.getControlSwitch(input.CONTROL_SWITCH.ViewMode) then
        previewTimer = previewTimer + dt
        if primaryMode == MODE.ThirdPerson or previewTimer >= switchLimit then
            third_person.standingPreview = false
            camera.setMode(MODE.Preview)
        end
    elseif previewTimer > 0 then
        if previewTimer <= switchLimit then
            if primaryMode == MODE.FirstPerson then
                primaryMode = MODE.ThirdPerson
            else
                primaryMode = MODE.FirstPerson
            end
        end
        camera.setMode(primaryMode)
        previewTimer = 0
    end
end

local idleTimer = 0
local vanityDelay = core.getGMST('fVanityDelay')

local function updateVanity(dt)
    if input.isIdle() then
        idleTimer = idleTimer + dt
    else
        idleTimer = 0
    end
    local vanityAllowed = input.getControlSwitch(input.CONTROL_SWITCH.VanityMode)
    if vanityAllowed and idleTimer > vanityDelay and camera.getMode() ~= MODE.Vanity then
        camera.setMode(MODE.Vanity)
    end
    if camera.getMode() == MODE.Vanity then
        if not vanityAllowed or idleTimer == 0 then
            camera.setMode(primaryMode)
        else
            camera.setYaw(camera.getYaw() + math.rad(3) * dt)
        end
    end
end

local function updateSmoothedSpeed(dt)
    local speed = Actor.currentSpeed(self)
    speed = speed / (1 + speed / 500)
    local maxDelta = 300 * dt
    smoothedSpeed = smoothedSpeed + util.clamp(speed - smoothedSpeed, -maxDelta, maxDelta)
end

local minDistance = 30
local maxDistance = 800

local function zoom(delta)
    if not input.getControlSwitch(input.CONTROL_SWITCH.ViewMode) or
       not input.getControlSwitch(input.CONTROL_SWITCH.Controls) or
       camera.getMode() == MODE.Static or noZoom > 0 then
        return
    end
    if camera.getMode() ~= MODE.FirstPerson then
        local obstacleDelta = third_person.preferredDistance - camera.getThirdPersonDistance()
        if delta > 0 and third_person.baseDistance == minDistance and
           (camera.getMode() ~= MODE.Preview or third_person.standingPreview) and noModeControl == 0 then
            primaryMode = MODE.FirstPerson
            camera.setMode(primaryMode)
        elseif delta > 0 or obstacleDelta < -delta then
            third_person.baseDistance = util.clamp(third_person.baseDistance - delta - obstacleDelta, minDistance, maxDistance)
        end
    elseif delta < 0 and noModeControl == 0 then
        primaryMode = MODE.ThirdPerson
        camera.setMode(primaryMode)
        third_person.baseDistance = minDistance
    end
end

local function applyControllerZoom(dt)
    if input.isActionPressed(input.ACTION.TogglePOV) then
        local triggerLeft = input.getAxisValue(input.CONTROLLER_AXIS.TriggerLeft)
        local triggerRight = input.getAxisValue(input.CONTROLLER_AXIS.TriggerRight)
        local controllerZoom = (triggerRight - triggerLeft) * 100 * dt
        if controllerZoom ~= 0 then
            zoom(controllerZoom)
        end
    end
end

local function updateStandingPreview()
    local mode = camera.getMode()
    if not previewIfStandStill or noStandingPreview > 0
       or mode == MODE.FirstPerson or mode == MODE.Static or mode == MODE.Vanity then
        third_person.standingPreview = false
        return
    end
    local standingStill = Actor.currentSpeed(self) == 0 and Actor.stance(self) == Actor.STANCE.Nothing
    if standingStill and mode == MODE.ThirdPerson then
        third_person.standingPreview = true
        camera.setMode(MODE.Preview)
    elseif not standingStill and third_person.standingPreview then
        third_person.standingPreview = false
        camera.setMode(primaryMode)
    end
end

local function updateCrosshair()
    camera.showCrosshair(
        camera.getMode() == MODE.FirstPerson or
        (showCrosshairInThirdPerson and (camera.getMode() == MODE.ThirdPerson or third_person.standingPreview)))
end

local function onUpdate(dt)
    camera.setExtraPitch(0)
    camera.setExtraYaw(0)
    camera.setExtraRoll(0)
    camera.setFirstPersonOffset(util.vector3(0, 0, 0))
    updateSmoothedSpeed(dt)
    pov_auto_switch.onUpdate(dt)
end

local function onFrame(dt)
    if core.isWorldPaused() then return end
    local mode = camera.getMode()
    if mode == MODE.FirstPerson or mode == MODE.ThirdPerson then
        primaryMode = mode
    end
    if mode ~= MODE.Static then
        if not camera.getQueuedMode() or camera.getQueuedMode() == MODE.Preview then
            if noModeControl == 0 then
                updatePOV(dt)
                updateVanity(dt)
            end
            updateStandingPreview()
        end
        updateCrosshair()
    end
    applyControllerZoom(dt)
    third_person.update(dt, smoothedSpeed)
    if noHeadBobbing == 0 then head_bobbing.update(dt, smoothedSpeed) end
    if slowViewChange then
        local maxIncrease = dt * (100 + third_person.baseDistance)
        camera.setPreferredThirdPersonDistance(
            math.min(camera.getThirdPersonDistance() + maxIncrease, third_person.preferredDistance))
    end
    move360.onFrame(dt)
end

return {
    interfaceName = 'Camera',
    ---
    -- @module Camera
    -- @usage require('openmw.interfaces').Camera
    interface = {
        --- Interface version
        -- @field [parent=#Camera] #number version
        version = 0,

        --- Return primary mode (MODE.FirstPerson or MODE.ThirdPerson).
        -- @function [parent=#Camera] getPrimaryMode
        getPrimaryMode = function() return primaryMode end,
        --- @function [parent=#Camera] getBaseThirdPersonDistance
        getBaseThirdPersonDistance = function() return third_person.baseDistance end,
        --- @function [parent=#Camera] setBaseThirdPersonDistance
        setBaseThirdPersonDistance = function(v) third_person.baseDistance = v end,
        --- @function [parent=#Camera] getTargetThirdPersonDistance
        getTargetThirdPersonDistance = function() return third_person.preferredDistance end,

        --- @function [parent=#Camera] isModeControlEnabled
        isModeControlEnabled = function() return noModeControl == 0 end,
        --- @function [parent=#Camera] disableModeControl
        disableModeControl = function() noModeControl = noModeControl + 1 end,
        --- @function [parent=#Camera] enableModeControl
        enableModeControl = function() noModeControl = math.max(0, noModeControl - 1) end,

        --- @function [parent=#Camera] isStandingPreviewEnabled
        isStandingPreviewEnabled = function() return previewIfStandStill and noStandingPreview == 0 end,
        --- @function [parent=#Camera] disableStandingPreview
        disableStandingPreview = function() noStandingPreview = noStandingPreview + 1 end,
        --- @function [parent=#Camera] enableStandingPreview
        enableStandingPreview = function() noStandingPreview = math.max(0, noStandingPreview - 1) end,

        --- @function [parent=#Camera] isHeadBobbingEnabled
        isHeadBobbingEnabled = function() return head_bobbing.enabled and noHeadBobbing == 0 end,
        --- @function [parent=#Camera] disableHeadBobbing
        disableHeadBobbing = function() noHeadBobbing = noHeadBobbing + 1 end,
        --- @function [parent=#Camera] enableHeadBobbing
        enableHeadBobbing = function() noHeadBobbing = math.max(0, noHeadBobbing - 1) end,

        --- @function [parent=#Camera] isZoomEnabled
        isZoomEnabled = function() return noZoom == 0 end,
        --- @function [parent=#Camera] disableZoom
        disableZoom = function() noZoom = noZoom + 1 end,
        --- @function [parent=#Camera] enableZoom
        enableZoom = function() noZoom = math.max(0, noZoom - 1) end,

        --- @function [parent=#Camera] isThirdPersonOffsetControlEnabled
        isThirdPersonOffsetControlEnabled = function() return third_person.noOffsetControl == 0 end,
        --- @function [parent=#Camera] disableThirdPersonOffsetControl
        disableThirdPersonOffsetControl = function() third_person.noOffsetControl = third_person.noOffsetControl + 1 end,
        --- @function [parent=#Camera] enableThirdPersonOffsetControl
        enableThirdPersonOffsetControl = function() third_person.noOffsetControl = math.max(0, third_person.noOffsetControl - 1) end,
    },
    engineHandlers = {
        onUpdate = onUpdate,
        onFrame = onFrame,
        onInputAction = function(action)
            if core.isWorldPaused() then return end
            if action == input.ACTION.ZoomIn then
                zoom(10)
            elseif action == input.ACTION.ZoomOut then
                zoom(-10)
            end
            move360.onInputAction(action)
        end,
        onActive = init,
        onLoad = function(data)
            if data and data.distance then third_person.baseDistance = data.distance end
        end,
        onSave = function()
            return {version = 0, distance = third_person.baseDistance}
        end,
    },
}
