local camera = require('openmw.camera')
local util = require('openmw.util')
local nearby = require('openmw.nearby')
local self = require('openmw.self')

local forcedFirstPerson = false
local limitSwitch = 40
local limitReturn = 65

local rayOptions = {collisionType = nearby.COLLISION_TYPE.Default - nearby.COLLISION_TYPE.Actor}
local function castRayBackward()
    local from = camera.getTrackedPosition()
    local orient = util.transform.rotateZ(camera.getYaw()) * util.transform.rotateX(camera.getPitch())
    local resLeft = nearby.castRay(from, from + orient * util.vector3(-30, -limitReturn, 0), rayOptions)
    local resRight = nearby.castRay(from, from + orient * util.vector3(30, -limitReturn, 0), rayOptions)
    local distLeft = limitReturn + 1
    local distRight = limitReturn + 1
    if resLeft.hit then distLeft = (resLeft.hitPos - from):length() end
    if resRight.hit then distRight = (resRight.hitPos - from):length() end
    return math.min(distLeft, distRight)
end

local M = {
    enabled = false,
}

function M.onUpdate(dt)
    if camera.getMode() ~= camera.MODE.FirstPerson then forcedFirstPerson = false end
    if not M.enabled then
        if forcedFirstPerson then
            camera.setMode(camera.MODE.ThirdPerson, false)
            forcedFirstPerson = false
        end
        return
    end
    if camera.getMode() == camera.MODE.ThirdPerson and camera.getThirdPersonDistance() < limitSwitch
            and math.abs(util.normalizeAngle(camera.getYaw() - self.rotation.z)) < math.rad(10) then
        if castRayBackward() <= limitSwitch then
            camera.setMode(camera.MODE.FirstPerson, true)
            forcedFirstPerson = true
        end
        return
    end
    if forcedFirstPerson then
        if castRayBackward() > limitReturn then
            camera.setMode(camera.MODE.ThirdPerson, false)
            forcedFirstPerson = false
        end
    end
end

return M
