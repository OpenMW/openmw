local testing = require('testing_util')
local util = require('openmw.util')
local world = require('openmw.world')
local core = require('openmw.core')
local types = require('openmw.types')

testing.registerGlobalTest('[issues] Player should be able to walk up stairs in Ebonheart docks (#4247)', function()
    world.players[1]:teleport('', util.vector3(19867, -102180, -79), util.transform.rotateZ(math.rad(91)))
    coroutine.yield()
    testing.runLocalTest(world.players[1], 'Player should be able to walk up stairs in Ebonheart docks (#4247)')
end)

testing.registerGlobalTest('[issues] Guard in Imperial Prison Ship should find path (#7241)', function()
    world.players[1]:teleport('Imperial Prison Ship', util.vector3(61, -135, -105), util.transform.rotateZ(math.rad(-20)))
    coroutine.yield()
    testing.runLocalTest(world.players[1], 'Guard in Imperial Prison Ship should find path (#7241)')
end)

testing.registerGlobalTest('[issues] Should keep reference to an object moved into container (#7663)', function()
    world.players[1]:teleport('ToddTest', util.vector3(2176, 3648, -191), util.transform.rotateZ(math.rad(0)))
    coroutine.yield()
    local barrel = world.createObject('barrel_01', 1)
    local fargothRing = world.createObject('ring_keley', 1)
    coroutine.yield()
    testing.expectEqual(types.Container.inventory(barrel):find('ring_keley'), nil)
    fargothRing:moveInto(types.Container.inventory(barrel))
    coroutine.yield()
    testing.expectEqual(fargothRing.recordId, 'ring_keley')
    local isFargothRing = function(actual)
        if actual == nil then
            return 'ring_keley is not found'
        end
        if actual.id ~= fargothRing.id then
            return 'found ring_keley id does not match expected: actual=' .. tostring(actual.id)
                    .. ', expected=' .. tostring(fargothRing.id)
        end
        return ''
    end
    testing.expectThat(types.Container.inventory(barrel):find('ring_keley'), isFargothRing)
end)
