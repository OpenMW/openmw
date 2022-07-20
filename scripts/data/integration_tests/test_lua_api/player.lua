local testing = require('testing_util')
local self = require('openmw.self')
local util = require('openmw.util')
local core = require('openmw.core')
local input = require('openmw.input')
local types = require('openmw.types')
local nearby = require('openmw.nearby')

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

testing.registerLocalTest('findPath',
    function()
        local src = util.vector3(4096, 4096, 867.237)
        local dst = util.vector3(4500, 4500, 700.216)
        local options = {
            agentBounds = types.Actor.getPathfindingAgentBounds(self),
            stepSize = 50,
            includeFlags = nearby.NAVIGATOR_FLAGS.Walk + nearby.NAVIGATOR_FLAGS.Swim,
            areaCosts = {
                water = 1,
                door = 2,
                ground = 1,
                pathgrid = 1,
            },
            destinationTolerance = 1,
        }
        local status, path = nearby.findPath(src, dst)
        testing.expectEqual(status, nearby.FIND_PATH_STATUS.Success, 'Status')
        testing.expectLessOrEqual((path[path:size()] - dst):length(), 1, 'Last path point')
    end)

testing.registerLocalTest('findRandomPointAroundCircle',
    function()
        local position = util.vector3(4096, 4096, 867.237)
        local maxRadius = 100
        local options = {
            agentBounds = types.Actor.getPathfindingAgentBounds(self),
            includeFlags = nearby.NAVIGATOR_FLAGS.Walk,
        }
        local result = nearby.findRandomPointAroundCircle(position, maxRadius, options)
        testing.expectGreaterThan((result - position):length(), 1, 'Random point')
    end)

testing.registerLocalTest('castNavigationRay',
    function()
        local src = util.vector3(4096, 4096, 867.237)
        local dst = util.vector3(4500, 4500, 700.216)
        local options = {
            agentBounds = types.Actor.getPathfindingAgentBounds(self),
            includeFlags = nearby.NAVIGATOR_FLAGS.Walk + nearby.NAVIGATOR_FLAGS.Swim,
        }
        local result = nearby.castNavigationRay(src, dst, options)
        testing.expectLessOrEqual((result - dst):length(), 1, 'Navigation hit point')
    end)

return {
    engineHandlers = {
        onUpdate = testing.updateLocal,
    },
    eventHandlers = testing.eventHandlers
}
