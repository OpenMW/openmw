local core = require('openmw.core')
local camera = require('openmw.camera')
local input = require('openmw.input')
local self = require('openmw.self')
local util = require('openmw.util')
local I = require('openmw.interfaces')

local Actor = require('openmw.types').Actor
local Player = require('openmw.types').Player

local MODE = camera.MODE

local active = false

local M = {
    enabled = false,
    turnSpeed = 5,
}

local function turnOn()
    I.Camera.disableStandingPreview()
    active = true
end

local function turnOff()
    I.Camera.enableStandingPreview()
    active = false
    if camera.getMode() == MODE.Preview then
        camera.setMode(MODE.ThirdPerson)
    end
end

local function processZoom3rdPerson()
    if
        not Player.getControlSwitch(self, Player.CONTROL_SWITCH.ViewMode) or
        not Player.getControlSwitch(self, Player.CONTROL_SWITCH.Controls) or
        input.getBooleanActionValue('TogglePOV') or
        not I.Camera.isModeControlEnabled() or
        not I.Camera.isZoomEnabled()
    then
        return
    end
    local Zoom3rdPerson = input.getNumberActionValue('Zoom3rdPerson')
    if Zoom3rdPerson > 0 and camera.getMode() == MODE.Preview
        and I.Camera.getBaseThirdPersonDistance() == 30 then
        self.controls.yawChange = camera.getYaw() - self.rotation:getYaw()
        camera.setMode(MODE.FirstPerson)
    elseif Zoom3rdPerson < 0 and camera.getMode() == MODE.FirstPerson then
        camera.setMode(MODE.Preview)
        I.Camera.setBaseThirdPersonDistance(30)
    end
end

function M.onFrame(dt)
    if core.isWorldPaused() then return end
    local newActive = M.enabled and Actor.getStance(self) == Actor.STANCE.Nothing
    if newActive and not active then
        turnOn()
    elseif not newActive and active then
        turnOff()
    end
    if not active then return end
    processZoom3rdPerson()
    if camera.getMode() == MODE.Static then return end
    if camera.getMode() == MODE.ThirdPerson then camera.setMode(MODE.Preview) end
    if camera.getMode() == MODE.Preview and not input.getBooleanActionValue('TogglePOV') then
        camera.showCrosshair(camera.getFocalPreferredOffset():length() > 5)
        local move = util.vector2(self.controls.sideMovement, self.controls.movement)
        local yawDelta = camera.getYaw() - self.rotation:getYaw()
        move = move:rotate(-yawDelta)
        self.controls.sideMovement = move.x
        self.controls.movement = move.y
        self.controls.pitchChange = camera.getPitch() * math.cos(yawDelta) - self.rotation:getPitch()
        if move:length() > 0.05 then
            local delta = math.atan2(move.x, move.y)
            local maxDelta = math.max(delta, 1) * M.turnSpeed * dt
            self.controls.yawChange = util.clamp(delta, -maxDelta, maxDelta)
        else
            self.controls.yawChange = 0
        end
    end
end

return M
