local testing = require('testing_util')
local core = require('openmw.core')
local async = require('openmw.async')
local util = require('openmw.util')
local types = require('openmw.types')
local world = require('openmw.world')

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

    testing.expectGreaterOrEqual(th1, 36, 'async:newGameTimer failed')
    testing.expectGreaterOrEqual(ts1, 0.5, 'async:newSimulationTimer failed')
    testing.expectGreaterOrEqual(th2, 72, 'async:newUnsavableGameTimer failed')
    testing.expectGreaterOrEqual(ts2, 1, 'async:newUnsavableSimulationTimer failed')
end

local function testTeleport()
    player:teleport('', util.vector3(100, 50, 500), util.transform.rotateZ(math.rad(90)))
    coroutine.yield()
    testing.expect(player.cell.isExterior, 'teleport to exterior failed')
    testing.expectEqualWithDelta(player.position.x, 100, 1, 'incorrect position after teleporting')
    testing.expectEqualWithDelta(player.position.y, 50, 1, 'incorrect position after teleporting')
    testing.expectEqualWithDelta(player.position.z, 500, 1, 'incorrect position after teleporting')
    testing.expectEqualWithDelta(player.rotation:getYaw(), math.rad(90), 0.05, 'incorrect rotation after teleporting')

    player:teleport('', player.position, {rotation=util.transform.rotateZ(math.rad(-90)), onGround=true})
    coroutine.yield()
    testing.expectEqualWithDelta(player.rotation:getYaw(), math.rad(-90), 0.05, 'options.rotation is not working')
    testing.expectLessOrEqual(player.position.z, 400, 'options.onGround is not working')

    player:teleport('', util.vector3(50, -100, 0))
    coroutine.yield()
    testing.expect(player.cell.isExterior, 'teleport to exterior failed')
    testing.expectEqualWithDelta(player.position.x, 50, 1, 'incorrect position after teleporting')
    testing.expectEqualWithDelta(player.position.y, -100, 1, 'incorrect position after teleporting')
    testing.expectEqualWithDelta(player.rotation:getYaw(), math.rad(-90), 0.05, 'teleporting changes rotation')
end

local function testGetGMST()
    testing.expectEqual(core.getGMST('non-existed gmst'), nil)
    testing.expectEqual(core.getGMST('Water_RippleFrameCount'), 4)
    testing.expectEqual(core.getGMST('Inventory_DirectionalDiffuseR'), 0.5)
    testing.expectEqual(core.getGMST('Level_Up_Level2'), 'something')
end

local function testMWScript()
    local variableStoreCount = 18
    local variableStore = world.mwscript.getGlobalVariables(player)
    testing.expectEqual(variableStoreCount, #variableStore)
    
    variableStore.year = 5
    testing.expectEqual(5, variableStore.year)
    variableStore.year = 1
    local indexCheck = 0
    for index, value in ipairs(variableStore) do
        testing.expectEqual(variableStore[index], value)
        indexCheck = indexCheck + 1
    end
    testing.expectEqual(variableStoreCount, indexCheck)
    indexCheck = 0
    for index, value in pairs(variableStore) do
        testing.expectEqual(variableStore[index], value)
        indexCheck = indexCheck + 1
    end
    testing.expectEqual(variableStoreCount, indexCheck)
end

local function testRecordStore(store,storeName,skipPairs)
    testing.expect(store.records)
    local firstRecord = store.records[1]
    if not firstRecord then return end
    testing.expectEqual(firstRecord.id,store.records[firstRecord.id].id)
    local status, _ = pcall(function()
            for index, value in ipairs(store.records) do
                if value.id == firstRecord.id then
                    testing.expectEqual(index,1,storeName)
                    break
                end
            end 
    end)

    testing.expectEqual(status,true,storeName)
    
end

local function testRecordStores()
    for key, type in pairs(types) do
        if type.records then
            testRecordStore(type,key)
        end
    end
    testRecordStore(core.magic.enchantments,"enchantments")
    testRecordStore(core.magic.effects,"effects",true)
    testRecordStore(core.magic.spells,"spells")

    testRecordStore(core.stats.Attribute,"Attribute")
    testRecordStore(core.stats.Skill,"Skill")

    testRecordStore(core.sound,"sound")
    testRecordStore(core.factions,"factions")

    testRecordStore(types.NPC.classes,"classes")
    testRecordStore(types.NPC.races,"races")
    testRecordStore(types.Player.birthSigns,"birthSigns")
end

local function initPlayer()
    player:teleport('', util.vector3(4096, 4096, 867.237), util.transform.identity)
    coroutine.yield()
end

tests = {
    {'timers', testTimers},
    {'playerRotation', function()
        initPlayer()
        testing.runLocalTest(player, 'playerRotation')
    end},
    {'playerForwardRunning', function()
        initPlayer()
        testing.runLocalTest(player, 'playerForwardRunning')
    end},
    {'playerDiagonalWalking', function()
        initPlayer()
        testing.runLocalTest(player, 'playerDiagonalWalking')
    end},
    {'findPath', function()
        initPlayer()
        testing.runLocalTest(player, 'findPath')
    end},
    {'findRandomPointAroundCircle', function()
        initPlayer()
        testing.runLocalTest(player, 'findRandomPointAroundCircle')
    end},
    {'castNavigationRay', function()
        initPlayer()
        testing.runLocalTest(player, 'castNavigationRay')
    end},
    {'findNearestNavMeshPosition', function()
        initPlayer()
        testing.runLocalTest(player, 'findNearestNavMeshPosition')
    end},
    {'teleport', testTeleport},
    {'getGMST', testGetGMST},
    {'recordStores', testRecordStores},
    {'mwscript', testMWScript},
}

return {
    engineHandlers = {
        onUpdate = testing.testRunner(tests),
        onPlayerAdded = function(p) player = p end,
    },
    eventHandlers = testing.eventHandlers,
}
