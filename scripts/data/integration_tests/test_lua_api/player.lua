local testing = require('testing_util')
local self = require('openmw.self')
local util = require('openmw.util')
local core = require('openmw.core')
local input = require('openmw.input')
local types = require('openmw.types')
local nearby = require('openmw.nearby')
local camera = require('openmw.camera')

types.Player.setControlSwitch(self, types.Player.CONTROL_SWITCH.Controls, false)
types.Player.setControlSwitch(self, types.Player.CONTROL_SWITCH.Fighting, false)
types.Player.setControlSwitch(self, types.Player.CONTROL_SWITCH.Jumping, false)
types.Player.setControlSwitch(self, types.Player.CONTROL_SWITCH.Looking, false)
types.Player.setControlSwitch(self, types.Player.CONTROL_SWITCH.Magic, false)
types.Player.setControlSwitch(self, types.Player.CONTROL_SWITCH.VanityMode, false)
types.Player.setControlSwitch(self, types.Player.CONTROL_SWITCH.ViewMode, false)

local function rotate(object, targetPitch, targetYaw)
    local endTime = core.getSimulationTime() + 1
    while core.getSimulationTime() < endTime do
        object.controls.jump = false
        object.controls.run = true
        object.controls.movement = 0
        object.controls.sideMovement = 0
        if targetPitch ~= nil then
            object.controls.pitchChange = util.normalizeAngle(targetPitch - object.rotation:getPitch()) * 0.5
        end
        if targetYaw ~= nil then
            object.controls.yawChange = util.normalizeAngle(targetYaw - object.rotation:getYaw()) * 0.5
        end
        coroutine.yield()
    end
end

local function rotateByYaw(object, target)
    rotate(object, nil, target)
end

local function rotateByPitch(object, target)
    rotate(object, target, nil)
end

testing.registerLocalTest('player yaw rotation',
    function()
        local initialAlphaXZ, initialGammaXZ = self.rotation:getAnglesXZ()
        local initialAlphaZYX, initialBetaZYX, initialGammaZYX = self.rotation:getAnglesZYX()

        local targetYaw = math.rad(90)
        rotateByYaw(self, targetYaw)

        testing.expectEqualWithDelta(self.rotation:getYaw(), targetYaw, 0.05, 'Incorrect yaw rotation')

        local alpha1, gamma1 = self.rotation:getAnglesXZ()
        testing.expectEqualWithDelta(alpha1, initialAlphaXZ, 0.05, 'Alpha rotation in XZ convention should not change')
        testing.expectEqualWithDelta(gamma1, targetYaw, 0.05, 'Incorrect gamma rotation in XZ convention')

        local alpha2, beta2, gamma2 = self.rotation:getAnglesZYX()
        testing.expectEqualWithDelta(alpha2, targetYaw, 0.05, 'Incorrect alpha rotation in ZYX convention')
        testing.expectEqualWithDelta(beta2, initialBetaZYX, 0.05, 'Beta rotation in ZYX convention should not change')
        testing.expectEqualWithDelta(gamma2, initialGammaZYX, 0.05, 'Gamma rotation in ZYX convention should not change')
    end)

testing.registerLocalTest('player pitch rotation',
    function()
        local initialAlphaXZ, initialGammaXZ = self.rotation:getAnglesXZ()
        local initialAlphaZYX, initialBetaZYX, initialGammaZYX = self.rotation:getAnglesZYX()

        local targetPitch = math.rad(90)
        rotateByPitch(self, targetPitch)

        testing.expectEqualWithDelta(self.rotation:getPitch(), targetPitch, 0.05, 'Incorrect pitch rotation')

        local alpha1, gamma1 = self.rotation:getAnglesXZ()
        testing.expectEqualWithDelta(alpha1, targetPitch, 0.05, 'Incorrect alpha rotation in XZ convention')
        testing.expectEqualWithDelta(gamma1, initialGammaXZ, 0.05, 'Gamma rotation in XZ convention should not change')

        local alpha2, beta2, gamma2 = self.rotation:getAnglesZYX()
        testing.expectEqualWithDelta(alpha2, initialAlphaZYX, 0.05, 'Alpha rotation in ZYX convention should not change')
        testing.expectEqualWithDelta(beta2, initialBetaZYX, 0.05, 'Beta rotation in ZYX convention should not change')
        testing.expectEqualWithDelta(gamma2, targetPitch, 0.05, 'Incorrect gamma rotation in ZYX convention')
    end)

testing.registerLocalTest('player pitch and yaw rotation',
    function()
        local targetPitch = math.rad(-30)
        local targetYaw = math.rad(-60)
        rotate(self, targetPitch, targetYaw)

        testing.expectEqualWithDelta(self.rotation:getPitch(), targetPitch, 0.05, 'Incorrect pitch rotation')
        testing.expectEqualWithDelta(self.rotation:getYaw(), targetYaw, 0.05, 'Incorrect yaw rotation')

        local alpha1, gamma1 = self.rotation:getAnglesXZ()
        testing.expectEqualWithDelta(alpha1, targetPitch, 0.05, 'Incorrect alpha rotation in XZ convention')
        testing.expectEqualWithDelta(gamma1, targetYaw, 0.05, 'Incorrect gamma rotation in XZ convention')

        local alpha2, beta2, gamma2 = self.rotation:getAnglesZYX()
        testing.expectEqualWithDelta(alpha2, math.rad(-56), 0.05, 'Incorrect alpha rotation in ZYX convention')
        testing.expectEqualWithDelta(beta2, math.rad(-25), 0.05, 'Incorrect beta rotation in ZYX convention')
        testing.expectEqualWithDelta(gamma2, math.rad(-16), 0.05, 'Incorrect gamma rotation in ZYX convention')
    end)

testing.registerLocalTest('player rotation',
    function()
        local rotation = math.sqrt(2)
        local endTime = core.getSimulationTime() + 3
        while core.getSimulationTime() < endTime do
            self.controls.jump = false
            self.controls.run = true
            self.controls.movement = 0
            self.controls.sideMovement = 0
            self.controls.pitchChange = rotation
            self.controls.yawChange = rotation
            coroutine.yield()

            local alpha1, gamma1 = self.rotation:getAnglesXZ()
            testing.expectThat(alpha1, testing.isNotNan(), 'Alpha rotation in XZ convention is nan')
            testing.expectThat(gamma1, testing.isNotNan(), 'Gamma rotation in XZ convention is nan')

            local alpha2, beta2, gamma2 = self.rotation:getAnglesZYX()
            testing.expectThat(alpha2, testing.isNotNan(), 'Alpha rotation in ZYX convention is nan')
            testing.expectThat(beta2, testing.isNotNan(), 'Beta rotation in ZYX convention is nan')
            testing.expectThat(gamma2, testing.isNotNan(), 'Gamma rotation in ZYX convention is nan')
        end
    end)

testing.registerLocalTest('player forward running',
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
        testing.expectGreaterThan(distance, 0, 'Run forward, distance')
        testing.expectEqualWithDelta(direction.x, 0, 0.1, 'Run forward, X coord')
        testing.expectEqualWithDelta(direction.y, 1, 0.1, 'Run forward, Y coord')
    end)

testing.registerLocalTest('player diagonal walking',
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
        testing.expectGreaterThan(distance, 0, 'Walk diagonally, distance')
        testing.expectEqualWithDelta(direction.x, -0.707, 0.1, 'Walk diagonally, X coord')
        testing.expectEqualWithDelta(direction.y, -0.707, 0.1, 'Walk diagonally, Y coord')
    end)

testing.registerLocalTest('findPath',
    function()
        local src = util.vector3(4096, 4096, 1745)
        local dst = util.vector3(4500, 4500, 1745.95263671875)
        local options = {
            agentBounds = types.Actor.getPathfindingAgentBounds(self),
            includeFlags = nearby.NAVIGATOR_FLAGS.Walk + nearby.NAVIGATOR_FLAGS.Swim,
            areaCosts = {
                water = 1,
                door = 2,
                ground = 1,
                pathgrid = 1,
            },
            destinationTolerance = 1,
        }
        local status, path = nearby.findPath(src, dst, options)
        testing.expectEqual(status, nearby.FIND_PATH_STATUS.Success, 'Status')
        testing.expectLessOrEqual((path[#path] - dst):length(), 1,
            'Last path point '  .. testing.formatActualExpected(path[#path], dst))
    end)

testing.registerLocalTest('findRandomPointAroundCircle',
    function()
        local position = util.vector3(4096, 4096, 1745.95263671875)
        local maxRadius = 100
        local options = {
            agentBounds = types.Actor.getPathfindingAgentBounds(self),
            includeFlags = nearby.NAVIGATOR_FLAGS.Walk,
        }
        local result = nearby.findRandomPointAroundCircle(position, maxRadius, options)
        testing.expectGreaterThan((result - position):length(), 1,
            'Random point ' .. testing.formatActualExpected(result, position))
    end)

testing.registerLocalTest('castNavigationRay',
    function()
        local src = util.vector3(4096, 4096, 1745)
        local dst = util.vector3(4500, 4500, 1745.95263671875)
        local options = {
            agentBounds = types.Actor.getPathfindingAgentBounds(self),
            includeFlags = nearby.NAVIGATOR_FLAGS.Walk + nearby.NAVIGATOR_FLAGS.Swim,
        }
        local result = nearby.castNavigationRay(src, dst, options)
        testing.expectLessOrEqual((result - dst):length(), 1,
            'Navigation hit point ' .. testing.formatActualExpected(result, dst))
    end)

testing.registerLocalTest('findNearestNavMeshPosition',
    function()
        local position = util.vector3(4096, 4096, 2000)
        local options = {
            agentBounds = types.Actor.getPathfindingAgentBounds(self),
            includeFlags = nearby.NAVIGATOR_FLAGS.Walk + nearby.NAVIGATOR_FLAGS.Swim,
            searchAreaHalfExtents = util.vector3(1000, 1000, 1000),
        }
        local result = nearby.findNearestNavMeshPosition(position, options)
        local expected = util.vector3(4096, 4096, 1746.27099609375)
        testing.expectLessOrEqual((result - expected):length(), 1,
            'Navigation mesh position ' .. testing.formatActualExpected(result, expected))
    end)

testing.registerLocalTest('player memory limit',
    function()
        local ok, err = pcall(function()
            local str = 'a'
            while true do
                str = str .. str
            end
        end)
        testing.expectEqual(ok, false, 'Script reaching memory limit should fail')
        testing.expectEqual(err, 'not enough memory')
    end)

testing.registerLocalTest('player weapon attack',
    function()
        camera.setMode(camera.MODE.ThirdPerson)

        local options = {
            agentBounds = types.Actor.getPathfindingAgentBounds(self),
        }
        local duration = 10
        local endTime = core.getSimulationTime() + duration
        local nextTime = 0
        local use = self.ATTACK_TYPE.NoAttack

        local attributes = {}
        for k, v in pairs(types.Actor.stats.attributes) do
            attributes[k] = v(self).base
        end

        types.Actor.stats.attributes.speed(self).base = 100
        types.Actor.stats.attributes.strength(self).base = 1000
        types.Actor.stats.attributes.agility(self).base = 1000

        local weaponId = 'basic_dagger1h'
        local weapon = types.Actor.inventory(self):find(weaponId)

        local isWeapon = function(actual)
            if actual == nil then
                return weaponId .. ' is not found'
            end
            if actual.recordId ~= weaponId then
                return 'found weapon recordId does not match expected: actual=' .. tostring(actual.id)
                       .. ', expected=' .. weaponId
            end
            return ''
        end
        testing.expectThat(weapon, isWeapon)

        types.Actor.setEquipment(self, {[types.Actor.EQUIPMENT_SLOT.CarriedRight] = weapon})

        coroutine.yield()

        testing.expectThat(types.Actor.getEquipment(self, types.Actor.EQUIPMENT_SLOT.CarriedRight), isWeapon)

        types.Actor.setStance(self, types.Actor.STANCE.Weapon)

        local previousHealth = nil
        local targetActor = nil

        while true do
            local time = core.getSimulationTime()
            testing.expectLessOrEqual(time, endTime, 'Did not damage any targets in ' .. duration .. ' seconds')

            if targetActor ~= nil and types.Actor.stats.dynamic.health(targetActor).current < previousHealth then
                print('Dealt ' .. (previousHealth - types.Actor.stats.dynamic.health(targetActor).current) .. ' damage to ' .. tostring(targetActor))
                break
            end

            local minDistance = nil
            for i, actor in ipairs(nearby.actors) do
                if actor.id ~= self.id then
                    local distance = (actor.position - self.position):length()
                    if minDistance == nil or minDistance > distance then
                        minDistance = distance
                        targetActor = actor
                    end
                end
            end
            testing.expectNotEqual(targetActor, nil, 'No attack targets found')

            previousHealth = types.Actor.stats.dynamic.health(targetActor).current

            local destination = nil
            if minDistance > 100 then
                local status, path = nearby.findPath(self.position, targetActor.position, options)

                testing.expectEqual(status, nearby.FIND_PATH_STATUS.Success,
                    'Failed to find path from ' .. tostring(self.position) .. ' to ' .. tostring(targetActor.position))

                destination = path[2]
                use = self.ATTACK_TYPE.NoAttack

                self.controls.run = true
                self.controls.movement = 1
            else
                local halfExtents = types.Actor.getPathfindingAgentBounds(targetActor).halfExtents
                destination = targetActor.position - util.vector3(0, 0, halfExtents.z)

                if nextTime < time then
                    if use == 0 then
                        use = self.ATTACK_TYPE.Any
                        nextTime = time + 0.5
                    else
                        use = self.ATTACK_TYPE.NoAttack
                    end
                end
            end
            self.controls.use = use

            local direction = destination - self.position
            direction = direction:normalize()
            self.controls.yawChange = util.normalizeAngle(math.atan2(direction.x, direction.y) - self.rotation:getYaw())
            self.controls.pitchChange = util.normalizeAngle(math.asin(util.clamp(-direction.z, -1, 1)) - self.rotation:getPitch())

            coroutine.yield()
        end

        for k, v in pairs(types.Actor.stats.attributes) do
            v(self).base = attributes[k]
        end
    end)

return {
    engineHandlers = {
        onFrame = testing.updateLocal,
    },
    eventHandlers = testing.localEventHandlers,
}
