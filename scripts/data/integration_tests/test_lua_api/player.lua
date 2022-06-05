local testing = require('testing_util')
local self = require('openmw.self')
local util = require('openmw.util')
local core = require('openmw.core')
local input = require('openmw.input')
local types = require('openmw.types')

input.setControlSwitch(input.CONTROL_SWITCH.Fighting, false)
input.setControlSwitch(input.CONTROL_SWITCH.Jumping, false)
input.setControlSwitch(input.CONTROL_SWITCH.Looking, false)
input.setControlSwitch(input.CONTROL_SWITCH.Magic, false)
input.setControlSwitch(input.CONTROL_SWITCH.VanityMode, false)
input.setControlSwitch(input.CONTROL_SWITCH.ViewMode, false)

testing.registerLocalTest('playerRotation',
    function()
        local endTime = core.getSimulationTime() + 1
        while core.getSimulationTime() < endTime do
            self.controls.jump = false
            self.controls.run = true
            self.controls.movement = 0
            self.controls.sideMovement = 0
            self.controls.yawChange = util.normalizeAngle(math.rad(90) - self.rotation.z) * 0.5
            coroutine.yield()
        end
        testing.expectEqualWithDelta(self.rotation.z, math.rad(90), 0.05, 'Incorrect rotation')
    end)

testing.registerLocalTest('playerForwardRunning',
    function()
        local startPos = self.position
        local endTime = core.getSimulationTime() + 1
        while core.getSimulationTime() < endTime do
            self.controls.jump = false
            self.controls.run = true
            self.controls.movement = 1
            self.controls.sideMovement = 0
            self.controls.yawChange = 0
            coroutine.yield()
        end
        local direction, distance = (self.position - startPos):normalize()
        local normalizedDistance = distance / types.Actor.runSpeed(self)
        testing.expectEqualWithDelta(normalizedDistance, 1, 0.2, 'Normalized forward runned distance')
        testing.expectEqualWithDelta(direction.x, 0, 0.1, 'Run forward, X coord')
        testing.expectEqualWithDelta(direction.y, 1, 0.1, 'Run forward, Y coord')
    end)

testing.registerLocalTest('playerDiagonalWalking',
    function()
        local startPos = self.position
        local endTime = core.getSimulationTime() + 1
        while core.getSimulationTime() < endTime do
            self.controls.jump = false
            self.controls.run = false
            self.controls.movement = -1
            self.controls.sideMovement = -1
            self.controls.yawChange = 0
            coroutine.yield()
        end
        local direction, distance = (self.position - startPos):normalize()
        local normalizedDistance = distance / types.Actor.walkSpeed(self)
        testing.expectEqualWithDelta(normalizedDistance, 1, 0.2, 'Normalized diagonally walked distance')
        testing.expectEqualWithDelta(direction.x, -0.707, 0.1, 'Walk diagonally, X coord')
        testing.expectEqualWithDelta(direction.y, -0.707, 0.1, 'Walk diagonally, Y coord')
    end)

return {
    engineHandlers = {
        onUpdate = testing.updateLocal,
    },
    eventHandlers = testing.eventHandlers
}
