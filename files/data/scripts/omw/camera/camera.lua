local camera = require('openmw.camera')
local core = require('openmw.core')
local debug = require('openmw.debug')
local input = require('openmw.input')
local util = require('openmw.util')
local self = require('openmw.self')
local nearby = require('openmw.nearby')
local async = require('openmw.async')
local storage = require('openmw.storage')
local I = require('openmw.interfaces')

local Actor = require('openmw.types').Actor
local Player = require('openmw.types').Player

input.registerAction {
    key = 'TogglePOV',
    l10n = 'OMWControls',
    name = 'TogglePOV_name',
    description = 'TogglePOV_description',
    type = input.ACTION_TYPE.Boolean,
    defaultValue = false,
}

input.registerAction {
    key = 'Zoom3rdPerson',
    l10n = 'OMWControls',
    name = 'Zoom3rdPerson_name',
    description = 'Zoom3rdPerson_description',
    type = input.ACTION_TYPE.Number,
    defaultValue = 0,
}

local settings = storage.playerSection('SettingsOMWCameraThirdPerson')
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

local noModeControl = {}
local noStandingPreview = {}
local noHeadBobbing = {}
local noZoom = {}

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
    if input.getBooleanActionValue('TogglePOV') and Player.getControlSwitch(self, Player.CONTROL_SWITCH.ViewMode) then
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
        if camera.getMode() == MODE.Preview then
            -- If Preview -> FirstPerson change is queued (because of 3rd person animation),
            -- then first exit Preview by switching to ThirdPerson, and then queue the switch to FirstPerson.
            camera.setMode(MODE.ThirdPerson)
            camera.setMode(MODE.FirstPerson)
        end
        previewTimer = 0
    end
end

local idleTimer = 0
local vanityDelay = core.getGMST('fVanityDelay')

local function updateVanity(dt)
    local vanityAllowed = Player.getControlSwitch(self, Player.CONTROL_SWITCH.VanityMode)
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
    local speed = Actor.getCurrentSpeed(self)
    speed = speed / (1 + speed / 500)
    local maxDelta = 300 * dt
    smoothedSpeed = smoothedSpeed + util.clamp(speed - smoothedSpeed, -maxDelta, maxDelta)
end

local minDistance = 30
local maxDistance = 800

local function zoom(delta)
    if not Player.getControlSwitch(self, Player.CONTROL_SWITCH.ViewMode) or
        not Player.getControlSwitch(self, Player.CONTROL_SWITCH.Controls) or
        camera.getMode() == MODE.Static or next(noZoom) then
        return
    end
    if camera.getMode() ~= MODE.FirstPerson then
        local obstacleDelta = third_person.preferredDistance - camera.getThirdPersonDistance()
        if delta > 0 and third_person.baseDistance == minDistance and
            (camera.getMode() ~= MODE.Preview or third_person.standingPreview) and not next(noModeControl) then
            primaryMode = MODE.FirstPerson
            camera.setMode(primaryMode)
        elseif delta > 0 or obstacleDelta < -delta then
            third_person.baseDistance = util.clamp(third_person.baseDistance - delta - obstacleDelta, minDistance,
                maxDistance)
        end
    elseif delta < 0 and not next(noModeControl) then
        primaryMode = MODE.ThirdPerson
        camera.setMode(primaryMode)
        third_person.baseDistance = minDistance
    end
end

local function updateStandingPreview()
    local mode = camera.getMode()
    if not previewIfStandStill or next(noStandingPreview)
        or mode == MODE.FirstPerson or mode == MODE.Static or mode == MODE.Vanity then
        third_person.standingPreview = false
        return
    end
    local standingStill = Actor.getCurrentSpeed(self) == 0 and Actor.getStance(self) == Actor.STANCE.Nothing
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

local function updateIdleTimer(dt)
    if not input.isIdle() then
        idleTimer = 0
    elseif self.controls.movement ~= 0 or self.controls.sideMovement ~= 0 or self.controls.jump or self.controls.use ~= 0 then
        idleTimer = 0 -- also reset the timer in case of a scripted movement
    else
        idleTimer = idleTimer + dt
    end
end

local function onFrame(dt)
    if core.isWorldPaused() or I.UI.getMode() then return end
    updateIdleTimer(dt)
    local mode = camera.getMode()
    if (mode == MODE.FirstPerson or mode == MODE.ThirdPerson) and not camera.getQueuedMode() then
        primaryMode = mode
    end
    if mode ~= MODE.Static then
        local paralysis = Actor.activeEffects(self):getEffect(core.magic.EFFECT_TYPE.Paralyze)
        local paralyzed = not debug.isGodMode() and paralysis.magnitude > 0
        if not next(noModeControl) and not paralyzed then
            updatePOV(dt)
            updateVanity(dt)
        end
        updateStandingPreview()
        updateCrosshair()
    end

    do
        local Zoom3rdPerson = input.getNumberActionValue('Zoom3rdPerson')
        if Zoom3rdPerson ~= 0 then
            zoom(Zoom3rdPerson)
        end
    end

    third_person.update(dt, smoothedSpeed)
    if not next(noHeadBobbing) then head_bobbing.update(dt, smoothedSpeed) end
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
    -- @context player
    -- @usage require('openmw.interfaces').Camera
    interface = {
        --- Interface version is 1
        -- @field [parent=#Camera] #number version
        version = 1,

        --- Return primary mode (MODE.FirstPerson or MODE.ThirdPerson).
        -- @function [parent=#Camera] getPrimaryMode
        -- @return #number @{openmw.camera#MODE}
        getPrimaryMode = function() return primaryMode end,

        --- Get base third person distance (without applying angle and speed modifiers).
        -- @function [parent=#Camera] getBaseThirdPersonDistance
        -- @return #number
        getBaseThirdPersonDistance = function() return third_person.baseDistance end,
        --- Set base third person distance
        -- @function [parent=#Camera] setBaseThirdPersonDistance
        -- @param #number value
        setBaseThirdPersonDistance = function(v) third_person.baseDistance = v end,
        --- Get the desired third person distance if there would be no obstacles (with angle and speed modifiers)
        -- @function [parent=#Camera] getTargetThirdPersonDistance
        -- @return #number
        getTargetThirdPersonDistance = function() return third_person.preferredDistance end,

        --- Whether the built-in mode control logic is enabled.
        -- @function [parent=#Camera] isModeControlEnabled
        -- @return #boolean
        isModeControlEnabled = function() return not next(noModeControl) end,
        --- Disable with (optional) tag until the corresponding enable function is called with the same tag.
        -- @function [parent=#Camera] disableModeControl
        -- @param #string tag (optional, empty string by default) Will be disabled until the enabling function is called with the same tag
        disableModeControl = function(tag) noModeControl[tag or ''] = true end,
        --- Undo disableModeControl
        -- @function [parent=#Camera] enableModeControl
        -- @param #string tag (optional, empty string by default)
        enableModeControl = function(tag) noModeControl[tag or ''] = nil end,

        --- Whether the built-in standing preview logic is enabled.
        -- @function [parent=#Camera] isStandingPreviewEnabled
        -- @return #boolean
        isStandingPreviewEnabled = function() return previewIfStandStill and not next(noStandingPreview) end,
        --- Disable with (optional) tag until the corresponding enable function is called with the same tag.
        -- @function [parent=#Camera] disableStandingPreview
        -- @param #string tag (optional, empty string by default) Will be disabled until the enabling function is called with the same tag
        disableStandingPreview = function(tag) noStandingPreview[tag or ''] = true end,
        --- Undo disableStandingPreview
        -- @function [parent=#Camera] enableStandingPreview
        -- @param #string tag (optional, empty string by default)
        enableStandingPreview = function(tag) noStandingPreview[tag or ''] = nil end,

        --- Whether head bobbing is enabled.
        -- @function [parent=#Camera] isHeadBobbingEnabled
        -- @return #boolean
        isHeadBobbingEnabled = function() return head_bobbing.enabled and not next(noHeadBobbing) end,
        --- Disable with (optional) tag until the corresponding enable function is called with the same tag.
        -- @function [parent=#Camera] disableHeadBobbing
        -- @param #string tag (optional, empty string by default) Will be disabled until the enabling function is called with the same tag
        disableHeadBobbing = function(tag) noHeadBobbing[tag or ''] = true end,
        --- Undo disableHeadBobbing
        -- @function [parent=#Camera] enableHeadBobbing
        -- @param #string tag (optional, empty string by default)
        enableHeadBobbing = function(tag) noHeadBobbing[tag or ''] = nil end,

        --- Whether the built-in zooming is enabled.
        -- @function [parent=#Camera] isZoomEnabled
        -- @return #boolean
        isZoomEnabled = function() return not next(noZoom) end,
        --- Disable with (optional) tag until the corresponding enable function is called with the same tag.
        -- @function [parent=#Camera] disableZoom
        -- @param #string tag (optional, empty string by default) Will be disabled until the enabling function is called with the same tag
        disableZoom = function(tag) noZoom[tag or ''] = true end,
        --- Undo disableZoom
        -- @function [parent=#Camera] enableZoom
        -- @param #string tag (optional, empty string by default)
        enableZoom = function(tag) noZoom[tag or ''] = nil end,

        --- Whether the the third person offset can be changed by the built-in camera script.
        -- @function [parent=#Camera] isThirdPersonOffsetControlEnabled
        -- @return #boolean
        isThirdPersonOffsetControlEnabled = function() return not next(third_person.noOffsetControl) end,
        --- Disable with (optional) tag until the corresponding enable function is called with the same tag.
        -- @function [parent=#Camera] disableThirdPersonOffsetControl
        -- @param #string tag (optional, empty string by default) Will be disabled until the enabling function is called with the same tag
        disableThirdPersonOffsetControl = function(tag) third_person.noOffsetControl[tag or ''] = true end,
        --- Undo disableThirdPersonOffsetControl
        -- @function [parent=#Camera] enableThirdPersonOffsetControl
        -- @param #string tag (optional, empty string by default)
        enableThirdPersonOffsetControl = function(tag) third_person.noOffsetControl[tag or ''] = nil end,
    },
    engineHandlers = {
        onUpdate = onUpdate,
        onFrame = onFrame,
        onTeleported = function()
            camera.instantTransition()
        end,
        onActive = init,
        onLoad = function(data)
            if data and data.distance then third_person.baseDistance = data.distance end
        end,
        onSave = function()
            return { version = 0, distance = third_person.baseDistance }
        end,
    },
}
