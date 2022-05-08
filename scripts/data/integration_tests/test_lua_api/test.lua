local testing = require('testing_util')
local core = require('openmw.core')
local async = require('openmw.async')
local util = require('openmw.util')

local function testTimers()
    testing.expectAlmostEqual(core.getGameTimeScale(), 30, 'incorrect getGameTimeScale() result')
    testing.expectAlmostEqual(core.getSimulationTimeScale(), 1, 'incorrect getSimulationTimeScale result')

    local startGameTime = core.getGameTime()
    local startSimulationTime = core.getSimulationTime()

    local ts1, ts2, th1, th2
    local cb = async:registerTimerCallback("tfunc", function(arg)
        if arg == 'g' then
            th1 = core.getGameTime() - startGameTime
        else
            ts1 = core.getSimulationTime() - startSimulationTime
        end
    end)
    async:newGameTimer(36, cb, 'g')
    async:newSimulationTimer(0.5, cb, 's')
    async:newUnsavableGameTimer(72, function()
        th2 = core.getGameTime() - startGameTime
    end)
    async:newUnsavableSimulationTimer(1, function()
        ts2 = core.getSimulationTime() - startSimulationTime
    end)

    while not (ts1 and ts2 and th1 and th2) do coroutine.yield() end

    testing.expectAlmostEqual(th1, 36, 'async:newGameTimer failed')
    testing.expectAlmostEqual(ts1, 0.5, 'async:newSimulationTimer failed')
    testing.expectAlmostEqual(th2, 72, 'async:newUnsavableGameTimer failed')
    testing.expectAlmostEqual(ts2, 1, 'async:newUnsavableSimulationTimer failed')
end

local function testTeleport()
    player:teleport('', util.vector3(100, 50, 0), util.vector3(0, 0, math.rad(-90)))
    coroutine.yield()
    testing.expect(player.cell.isExterior, 'teleport to exterior failed')
    testing.expectEqualWithDelta(player.position.x, 100, 1, 'incorrect position after teleporting')
    testing.expectEqualWithDelta(player.position.y, 50, 1, 'incorrect position after teleporting')
    testing.expectEqualWithDelta(player.rotation.z, math.rad(-90), 0.05, 'incorrect rotation after teleporting')

    player:teleport('', util.vector3(50, -100, 0))
    coroutine.yield()
    testing.expect(player.cell.isExterior, 'teleport to exterior failed')
    testing.expectEqualWithDelta(player.position.x, 50, 1, 'incorrect position after teleporting')
    testing.expectEqualWithDelta(player.position.y, -100, 1, 'incorrect position after teleporting')
    testing.expectEqualWithDelta(player.rotation.z, math.rad(-90), 0.05, 'teleporting changes rotation')
end

tests = {
    {'timers', testTimers},
    {'playerMovement', function() testing.runLocalTest(player, 'playerMovement') end},
    {'teleport', testTeleport},
}

return {
    engineHandlers = {
        onUpdate = testing.testRunner(tests),
        onPlayerAdded = function(p) player = p end,
    },
    eventHandlers = testing.eventHandlers,
}

