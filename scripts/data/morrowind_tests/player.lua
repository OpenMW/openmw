local core = require('openmw.core')
local input = require('openmw.input')
local self = require('openmw.self')
local testing = require('testing_util')
local util = require('openmw.util')
local types = require('openmw.types')
local nearby = require('openmw.nearby')

types.Player.setControlSwitch(self, types.Player.CONTROL_SWITCH.Fighting, false)
types.Player.setControlSwitch(self, types.Player.CONTROL_SWITCH.Jumping, false)
types.Player.setControlSwitch(self, types.Player.CONTROL_SWITCH.Looking, false)
types.Player.setControlSwitch(self, types.Player.CONTROL_SWITCH.Magic, false)
types.Player.setControlSwitch(self, types.Player.CONTROL_SWITCH.VanityMode, false)
types.Player.setControlSwitch(self, types.Player.CONTROL_SWITCH.ViewMode, false)

testing.registerLocalTest('Player should be able to walk up stairs in Ebonheart docks (#4247)',
    function()
        local startPos = self.position
        local dest = util.vector3(20296, -102194, 73)
        local endTime = core.getSimulationTime() + 3
        while (self.position.x <= dest.x or self.position.z <= dest.z) and core.getSimulationTime() < endTime do
            self.controls.jump = false
            self.controls.run = true
            self.controls.movement = 1
            self.controls.sideMovement = 0
            self.controls.yawChange = 0
            coroutine.yield()
        end
        testing.expectGreaterThan(self.position.x, dest.x, 'Final position, X')
        testing.expectEqualWithDelta(self.position.y, dest.y, 10, 'Final position, Y')
        testing.expectGreaterThan(self.position.z, dest.z, 'Final position, Z')
    end)

testing.registerLocalTest('Guard in Imperial Prison Ship should find path (#7241)',
    function()
        local src = util.vector3(34.297367095947265625, 806.3817138671875, 109.278961181640625)
        local dst = util.vector3(90, -90, -88)
        local agentBounds = types.Actor.getPathfindingAgentBounds(self)
        local options = {
            agentBounds = agentBounds,
            destinationTolerance = 0,
        }
        local status, path = nearby.findPath(src, dst, options)
        testing.expectEqual(status, nearby.FIND_PATH_STATUS.Success, 'Status')
        testing.expectLessOrEqual((util.vector2(path[#path].x, path[#path]) - util.vector2(dst.x, dst.y)):length(), 1, 'Last path point x, y')
        testing.expectLessOrEqual(path[#path].z - dst.z, 20, 'Last path point z')
        if agentBounds.shapeType == nearby.COLLISION_SHAPE_TYPE.Aabb then
            testing.expectThat(path, testing.elementsAreArray({
                testing.closeToVector(util.vector3(34.29737091064453125, 806.3817138671875, 112.76610565185546875), 1e-1),
                testing.closeToVector(util.vector3(15, 1102, 112.2945709228515625), 1e-1),
                testing.closeToVector(util.vector3(-112, 1110, 112.2945709228515625), 1e-1),
                testing.closeToVector(util.vector3(-118, 1393, 112.2945709228515625), 1e-1),
                testing.closeToVector(util.vector3(-67.99993896484375, 1421.2000732421875, 112.2945709228515625), 1e-1),
                testing.closeToVector(util.vector3(-33.999935150146484375, 1414.4000244140625, 112.2945709228515625), 1e-1),
                testing.closeToVector(util.vector3(-6.79993534088134765625, 1380.4000244140625, 85.094573974609375), 1e-1),
                testing.closeToVector(util.vector3(79, 724, -104.83390045166015625), 1e-1),
                testing.closeToVector(util.vector3(84, 290.000030517578125, -104.83390045166015625), 1e-1),
                testing.closeToVector(util.vector3(83.552001953125, 42.26399993896484375, -104.58989715576171875), 1e-1),
                testing.closeToVector(util.vector3(89, -105, -98.72841644287109375), 1e-1),
                testing.closeToVector(util.vector3(90, -90, -99.7056884765625), 1e-1),
            }))
        elseif agentBounds.shapeType == nearby.COLLISION_SHAPE_TYPE.Cylinder then
            testing.expectThat(path, testing.elementsAreArray({
                testing.closeToVector(util.vector3(34.29737091064453125, 806.3817138671875, 112.76610565185546875), 1e-1),
                testing.closeToVector(util.vector3(-13.5999355316162109375, 1060.800048828125, 112.2945709228515625), 1e-1),
                testing.closeToVector(util.vector3(-27.1999359130859375, 1081.2000732421875, 112.2945709228515625), 1e-1),
                testing.closeToVector(util.vector3(-81.59993743896484375, 1128.800048828125, 112.2945709228515625), 1e-1),
                testing.closeToVector(util.vector3(-101.99993896484375, 1156.0001220703125, 112.2945709228515625), 1e-1),
                testing.closeToVector(util.vector3(-118, 1393, 112.2945709228515625), 1e-1),
                testing.closeToVector(util.vector3(7, 1470, 114.73973846435546875), 1e-1),
                testing.closeToVector(util.vector3(79, 724, -104.83390045166015625), 1e-1),
                testing.closeToVector(util.vector3(84, 290.000030517578125, -104.83390045166015625), 1e-1),
                testing.closeToVector(util.vector3(95, 27, -104.83390045166015625), 1e-1),
                testing.closeToVector(util.vector3(90, -90, -104.83390045166015625), 1e-1),
            }))
        end
    end)

return {
    engineHandlers = {
        onFrame = testing.updateLocal,
    },
    eventHandlers = testing.eventHandlers
}
