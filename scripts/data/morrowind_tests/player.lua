local core = require('openmw.core')
local input = require('openmw.input')
local self = require('openmw.self')
local testing = require('testing_util')
local util = require('openmw.util')
local types = require('openmw.types')
local nearby = require('openmw.nearby')

input.setControlSwitch(input.CONTROL_SWITCH.Fighting, false)
input.setControlSwitch(input.CONTROL_SWITCH.Jumping, false)
input.setControlSwitch(input.CONTROL_SWITCH.Looking, false)
input.setControlSwitch(input.CONTROL_SWITCH.Magic, false)
input.setControlSwitch(input.CONTROL_SWITCH.VanityMode, false)
input.setControlSwitch(input.CONTROL_SWITCH.ViewMode, false)

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
        testing.expectLessOrEqual((util.vector2(path[path:size()].x, path[path:size()].y) - util.vector2(dst.x, dst.y)):length(), 1, 'Last path point x, y')
        testing.expectLessOrEqual(path[path:size()].z - dst.z, 20, 'Last path point z')
        if agentBounds.shapeType == nearby.COLLISION_SHAPE_TYPE.Aabb then
            testing.expectThat(path, testing.elementsAreArray({
                testing.closeToVector(util.vector3(34.29737091064453125, 806.3817138671875, 112.76610565185546875), 1e-1),
                testing.closeToVector(util.vector3(30.4828090667724609375, 864.81732177734375, 112.76610565185546875), 1e-1),
                testing.closeToVector(util.vector3(26.6682491302490234375, 923.25299072265625, 112.2945709228515625), 1e-1),
                testing.closeToVector(util.vector3(22.85369110107421875, 981.6885986328125, 112.2945709228515625), 1e-1),
                testing.closeToVector(util.vector3(19.03913116455078125, 1040.124267578125, 112.2945709228515625), 1e-1),
                testing.closeToVector(util.vector3(15.22457122802734375, 1098.559814453125, 112.2945709228515625), 1e-1),
                testing.closeToVector(util.vector3(15, 1102, 112.2945709228515625), 1e-1),
                testing.closeToVector(util.vector3(15, 1102, 112.2945709228515625), 1e-1),
                testing.closeToVector(util.vector3(15, 1102, 112.2945709228515625), 1e-1),
                testing.closeToVector(util.vector3(-67.99993896484375, 1108.4000244140625, 112.2945709228515625), 1e-1),
                testing.closeToVector(util.vector3(-112, 1110, 112.2945709228515625), 1e-1),
                testing.closeToVector(util.vector3(-112, 1110, 112.2945709228515625), 1e-1),
                testing.closeToVector(util.vector3(-112, 1110, 112.2945709228515625), 1e-1),
                testing.closeToVector(util.vector3(-115.59993743896484375, 1360.0001220703125, 112.2945709228515625), 1e-1),
                testing.closeToVector(util.vector3(-101.39704132080078125, 1416.811767578125, 112.2945709228515625), 1e-1),
                testing.closeToVector(util.vector3(-43.336151123046875, 1424.44091796875, 112.2945709228515625), 1e-1),
                testing.closeToVector(util.vector3(-3.460088253021240234375, 1381.5552978515625, 92.34075927734375), 1e-1),
                testing.closeToVector(util.vector3(3.82649898529052734375, 1323.4503173828125, 43.302886962890625), 1e-1),
                testing.closeToVector(util.vector3(11.11308765411376953125, 1265.345458984375, -7.3479709625244140625), 1e-1),
                testing.closeToVector(util.vector3(18.399677276611328125, 1207.240478515625, -54.67620849609375), 1e-1),
                testing.closeToVector(util.vector3(25.6862640380859375, 1149.135498046875, -91.845550537109375), 1e-1),
                testing.closeToVector(util.vector3(32.9728546142578125, 1091.030517578125, -97.08281707763671875), 1e-1),
                testing.closeToVector(util.vector3(40.2594451904296875, 1032.9256591796875, -98.50542449951171875), 1e-1),
                testing.closeToVector(util.vector3(47.546039581298828125, 974.82080078125, -98.50542449951171875), 1e-1),
                testing.closeToVector(util.vector3(54.832630157470703125, 916.71588134765625, -98.50542449951171875), 1e-1),
                testing.closeToVector(util.vector3(62.119220733642578125, 858.61102294921875, -104.83390045166015625), 1e-1),
                testing.closeToVector(util.vector3(69.40581512451171875, 800.50616455078125, -104.83390045166015625), 1e-1),
                testing.closeToVector(util.vector3(76.692413330078125, 742.4012451171875, -104.83390045166015625), 1e-1),
                testing.closeToVector(util.vector3(79, 724, -104.83390045166015625), 1e-1),
                testing.closeToVector(util.vector3(79, 724, -104.83390045166015625), 1e-1),
                testing.closeToVector(util.vector3(79, 724, -104.83390045166015625), 1e-1),
                testing.closeToVector(util.vector3(40.80001068115234375, 353.600006103515625, -104.83390045166015625), 1e-1),
                testing.closeToVector(util.vector3(73.7038726806640625, 305.158203125, -104.83390045166015625), 1e-1),
                testing.closeToVector(util.vector3(84, 290.000030517578125, -104.83390045166015625), 1e-1),
                testing.closeToVector(util.vector3(84, 290.000030517578125, -104.58989715576171875), 1e-1),
                testing.closeToVector(util.vector3(136.0000152587890625, 81.60001373291015625, -104.58989715576171875), 1e-1),
                testing.closeToVector(util.vector3(89.15203094482421875, 46.464019775390625, -104.58989715576171875), 1e-1),
                testing.closeToVector(util.vector3(83.552001953125, 42.26399993896484375, -104.58989715576171875), 1e-1),
                testing.closeToVector(util.vector3(83.552001953125, 42.26399993896484375, -98.72841644287109375), 1e-1),
                testing.closeToVector(util.vector3(115.60001373291015625, -27.1999359130859375, -98.72841644287109375), 1e-1),
                testing.closeToVector(util.vector3(93.4945526123046875, -81.42742156982421875, -100.4057159423828125), 1e-1),
                testing.closeToVector(util.vector3(90, -90, -99.7056884765625), 1e-1),
            }))
        elseif agentBounds.shapeType == nearby.COLLISION_SHAPE_TYPE.Cylinder then
            testing.expectThat(path, testing.elementsAreArray({
                testing.closeToVector(util.vector3(34.29737091064453125, 806.3817138671875, 112.76610565185546875), 1e-1),
                testing.closeToVector(util.vector3(23.4630756378173828125, 863.9307861328125, 112.76610565185546875), 1e-1),
                testing.closeToVector(util.vector3(12.628780364990234375, 921.4798583984375, 112.2945709228515625), 1e-1),
                testing.closeToVector(util.vector3(1.79448258876800537109375, 979.0289306640625, 112.2945709228515625), 1e-1),
                testing.closeToVector(util.vector3(-9.0398197174072265625, 1036.5780029296875, 112.2945709228515625), 1e-1),
                testing.closeToVector(util.vector3(-19.8741359710693359375, 1094.126953125, 112.2945709228515625), 1e-1),
                testing.closeToVector(util.vector3(-70.930450439453125, 1122.8067626953125, 112.2945709228515625), 1e-1),
                testing.closeToVector(util.vector3(-121.98685455322265625, 1151.486328125, 112.2945709228515625), 1e-1),
                testing.closeToVector(util.vector3(-121.020294189453125, 1210.038330078125, 112.2945709228515625), 1e-1),
                testing.closeToVector(util.vector3(-120.0537261962890625, 1268.59033203125, 112.2945709228515625), 1e-1),
                testing.closeToVector(util.vector3(-119.08716583251953125, 1327.142333984375, 112.2945709228515625), 1e-1),
                testing.closeToVector(util.vector3(-118.12059783935546875, 1385.6944580078125, 112.2945709228515625), 1e-1),
                testing.closeToVector(util.vector3(-118, 1393, 112.2945709228515625), 1e-1),
                testing.closeToVector(util.vector3(-118, 1393, 112.2945709228515625), 1e-1),
                testing.closeToVector(util.vector3(-118, 1393, 114.73973846435546875), 1e-1),
                testing.closeToVector(util.vector3(27.200008392333984375, 1380.4000244140625, 114.73973846435546875), 1e-1),
                testing.closeToVector(util.vector3(29.74369049072265625, 1321.8953857421875, 114.73973846435546875), 1e-1),
                testing.closeToVector(util.vector3(32.287372589111328125, 1263.3907470703125, 114.73973846435546875), 1e-1),
                testing.closeToVector(util.vector3(34.831058502197265625, 1204.885986328125, -57.1894378662109375), 1e-1),
                testing.closeToVector(util.vector3(40.18719482421875, 1146.571533203125, -90.156890869140625), 1e-1),
                testing.closeToVector(util.vector3(45.543331146240234375, 1088.2569580078125, -97.2764434814453125), 1e-1),
                testing.closeToVector(util.vector3(50.89946746826171875, 1029.9423828125, -98.50542449951171875), 1e-1),
                testing.closeToVector(util.vector3(56.255603790283203125, 971.62786865234375, -98.50542449951171875), 1e-1),
                testing.closeToVector(util.vector3(61.6117401123046875, 913.31329345703125, -98.50542449951171875), 1e-1),
                testing.closeToVector(util.vector3(66.9678802490234375, 854.998779296875, -104.83390045166015625), 1e-1),
                testing.closeToVector(util.vector3(72.3240203857421875, 796.6842041015625, -104.83390045166015625), 1e-1),
                testing.closeToVector(util.vector3(77.68015289306640625, 738.36968994140625, -104.83390045166015625), 1e-1),
                testing.closeToVector(util.vector3(79, 724, -104.83390045166015625), 1e-1),
                testing.closeToVector(util.vector3(79, 724, -104.83390045166015625), 1e-1),
                testing.closeToVector(util.vector3(108.80001068115234375, 299.20001220703125, -104.83390045166015625), 1e-1),
                testing.closeToVector(util.vector3(84, 290.000030517578125, -104.83390045166015625), 1e-1),
                testing.closeToVector(util.vector3(84, 290.000030517578125, -104.83390045166015625), 1e-1),
                testing.closeToVector(util.vector3(84, 290.000030517578125, -104.83390045166015625), 1e-1),
                testing.closeToVector(util.vector3(108.80001068115234375, 54.4000091552734375, -104.83390045166015625), 1e-1),
                testing.closeToVector(util.vector3(101.23966217041015625, -3.6698896884918212890625, -104.83390045166015625), 1e-1),
                testing.closeToVector(util.vector3(93.6793060302734375, -61.7397918701171875, -104.83390045166015625), 1e-1),
                testing.closeToVector(util.vector3(90, -90, -104.83390045166015625), 1e-1),
            }))
        end
    end)

return {
    engineHandlers = {
        onUpdate = testing.updateLocal,
    },
    eventHandlers = testing.eventHandlers
}
