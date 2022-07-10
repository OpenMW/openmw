local testing = require('testing_util')
local util = require('openmw.util')
local world = require('openmw.world')
local core = require('openmw.core')

if not core.contentFiles.has('Morrowind.esm') then
    error('This test requires Morrowind.esm')
end

local tests = {
    {'Player should be able to walk up stairs in Ebonheart docks (#4247)', function()
        world.players[1]:teleport('', util.vector3(19867, -102180, -79), util.transform.rotateZ(math.rad(91)))
        coroutine.yield()
        testing.runLocalTest(world.players[1], 'Player should be able to walk up stairs in Ebonheart docks (#4247)')
    end},
    {'Guard in Imperial Prison Ship should find path (#7241)', function()
        world.players[1]:teleport('Imperial Prison Ship', util.vector3(61, -135, -105), util.transform.rotateZ(math.rad(-20)))
        coroutine.yield()
        testing.runLocalTest(world.players[1], 'Guard in Imperial Prison Ship should find path (#7241)')
    end},
}

return {
    engineHandlers = {
        onUpdate = testing.testRunner(tests),
    },
    eventHandlers = testing.eventHandlers,
}
