local core = require('openmw.core')
local camera = require('openmw.camera')
local input = require('openmw.input')
local self = require('openmw.self')
local util = require('openmw.util')
local I = require('openmw.interfaces')

local Actor = require('openmw.types').Actor

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

function M.onFrame(dt)
    if core.isWorldPaused() then return end
    local newActive = M.enabled and Actor.stance(self) == Actor.STANCE.Nothing
    if newActive and not active then
        turnOn()
    elseif not newActive and active then
        turnOff()
    end
    if not active then return end
    if camera.getMode() == MODE.Static then return end
    if camera.getMode() == MODE.ThirdPerson then camera.setMode(MODE.Preview) end
    if camera.getMode() == MODE.Preview and not input.isActionPressed(input.ACTION.TogglePOV) then
        camera.showCrosshair(camera.getFocalPreferredOffset():length() > 5)
        local move = util.vector2(self.controls.sideMovement, self.controls.movement)
        local yawDelta = camera.getYaw() - self.rotation.z
        move = move:rotate(-yawDelta)
        self.controls.sideMovement = move.x
        self.controls.movement = move.y
        self.controls.pitchChange = camera.getPitch() * math.cos(yawDelta) - self.rotation.x
        if move:length() > 0.05 then
            local delta = math.atan2(move.x, move.y)
            local maxDelta = math.max(delta, 1) * M.turnSpeed * dt
            self.controls.yawChange = util.clamp(delta, -maxDelta, maxDelta)
        else
            self.controls.yawChange = 0
        end
    end
end

function M.onInputAction(action)
    if not active or core.isWorldPaused() then return end
    if action == input.ACTION.ZoomIn and camera.getMode() == MODE.Preview
       and I.Camera.getBaseThirdPersonDistance() == 30 then
        self.controls.yawChange = camera.getYaw() - self.rotation.z
        camera.setMode(MODE.FirstPerson)
    elseif action == input.ACTION.ZoomOut and camera.getMode() == MODE.FirstPerson then
        camera.setMode(MODE.Preview)
        I.Camera.setBaseThirdPersonDistance(30)
    end
end

return M
