local testing = require('testing_util')
local core = require('openmw.core')
local async = require('openmw.async')
local util = require('openmw.util')
local types = require('openmw.types')
local vfs = require('openmw.vfs')
local world = require('openmw.world')
local I = require('openmw.interfaces')

testing.registerGlobalTest('timers', function()
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

    while not (ts1 and ts2 and th1 and th2) do
        coroutine.yield()
    end

    testing.expectGreaterOrEqual(th1, 36, 'async:newGameTimer failed')
    testing.expectGreaterOrEqual(ts1, 0.5, 'async:newSimulationTimer failed')
    testing.expectGreaterOrEqual(th2, 72, 'async:newUnsavableGameTimer failed')
    testing.expectGreaterOrEqual(ts2, 1, 'async:newUnsavableSimulationTimer failed')
end)

testing.registerGlobalTest('teleport', function()
    local player = world.players[1]
    player:teleport('', util.vector3(100, 50, 500), util.transform.rotateZ(math.rad(90)))
    coroutine.yield()
    testing.expect(player.cell.isExterior, 'teleport to exterior failed')
    testing.expectEqualWithDelta(player.position.x, 100, 1, 'incorrect position after teleporting')
    testing.expectEqualWithDelta(player.position.y, 50, 1, 'incorrect position after teleporting')
    testing.expectEqualWithDelta(player.position.z, 500, 1, 'incorrect position after teleporting')
    testing.expectEqualWithDelta(player.rotation:getYaw(), math.rad(90), 0.05, 'incorrect yaw rotation after teleporting')
    testing.expectEqualWithDelta(player.rotation:getPitch(), math.rad(0), 0.05, 'incorrect pitch rotation after teleporting')

    local rotationX1, rotationZ1 = player.rotation:getAnglesXZ()
    testing.expectEqualWithDelta(rotationX1, math.rad(0), 0.05, 'incorrect x rotation from getAnglesXZ after teleporting')
    testing.expectEqualWithDelta(rotationZ1, math.rad(90), 0.05, 'incorrect z rotation from getAnglesXZ after teleporting')

    local rotationZ2, rotationY2, rotationX2 = player.rotation:getAnglesZYX()
    testing.expectEqualWithDelta(rotationZ2, math.rad(90), 0.05, 'incorrect z rotation from getAnglesZYX after teleporting')
    testing.expectEqualWithDelta(rotationY2, math.rad(0), 0.05, 'incorrect y rotation from getAnglesZYX after teleporting')
    testing.expectEqualWithDelta(rotationX2, math.rad(0), 0.05, 'incorrect x rotation from getAnglesZYX after teleporting')

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
end)

testing.registerGlobalTest('getGMST', function()
    testing.expectEqual(core.getGMST('non-existed gmst'), nil)
    testing.expectEqual(core.getGMST('Water_RippleFrameCount'), 4)
    testing.expectEqual(core.getGMST('Inventory_DirectionalDiffuseR'), 0.5)
    testing.expectEqual(core.getGMST('Level_Up_Level2'), 'something')
end)

testing.registerGlobalTest('MWScript', function()
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
end)

local function testRecordStore(store, storeName, skipPairs)
    testing.expect(store.records)
    local firstRecord = store.records[1]
    if not firstRecord then
        return
    end
    testing.expectEqual(firstRecord.id, store.records[firstRecord.id].id)
    local status, _ = pcall(function()
        for index, value in ipairs(store.records) do
            if value.id == firstRecord.id then
                testing.expectEqual(index, 1, storeName)
                break
            end
        end
    end)

    testing.expectEqual(status, true, storeName)
end

testing.registerGlobalTest('record stores', function()
    for key, type in pairs(types) do
        if type.records then
            testRecordStore(type, key)
        end
    end
    testRecordStore(core.magic.enchantments, "enchantments")
    testRecordStore(core.magic.effects, "effects", true)
    testRecordStore(core.magic.spells, "spells")

    testRecordStore(core.stats.Attribute, "Attribute")
    testRecordStore(core.stats.Skill, "Skill")

    testRecordStore(core.sound, "sound")
    testRecordStore(core.factions, "factions")

    testRecordStore(types.NPC.classes, "classes")
    testRecordStore(types.NPC.races, "races")
    testRecordStore(types.Player.birthSigns, "birthSigns")
end)

testing.registerGlobalTest('record creation', function()
    local newLight = {
        isCarriable = true,
        isDynamic = true,
        isFire =false,
        isFlicker = false,
        isFlickerSlow = false,
        isNegative = false,
        isOffByDefault = false,
        isPulse = false,
        weight = 1,
        value = 10,
        duration = 12,
        radius = 30,
        color = util.color.hex('123456'),
        name = "TestLight",
        model = "meshes/marker_door.dae"
    }
    local draft = types.Light.createRecordDraft(newLight)
    local record = world.createRecord(draft)
    for key, value in pairs(newLight) do
        testing.expectEqual(record[key], value)
    end
end)

testing.registerGlobalTest('UTF-8 characters', function()
    testing.expectEqual(utf8.codepoint("😀"), 0x1F600)

    local chars = {}

    for codepoint = 0, 0x10FFFF do
        local char = utf8.char(codepoint)
        local charSize = string.len(char)

        testing.expect(not chars[char], nil, "Duplicate UTF-8 character: " .. char)
        chars[char] = true

        if codepoint <= 0x7F then
            testing.expectEqual(charSize, 1)
        elseif codepoint <= 0x7FF then
            testing.expectEqual(charSize, 2)
        elseif codepoint <= 0xFFFF then
            testing.expectEqual(charSize, 3)
        elseif codepoint <= 0x10FFFF then
            testing.expectEqual(charSize, 4)
        end

        testing.expectEqual(utf8.codepoint(char), codepoint)
        testing.expectEqual(utf8.len(char), 1)
    end
end)

testing.registerGlobalTest('UTF-8 strings', function()
    local utf8str = "Hello, 你好, 🌎!"

    local str = ""
    for utf_char in utf8str:gmatch(utf8.charpattern) do
        str = str .. utf_char
    end
    testing.expectEqual(str, utf8str)

    testing.expectEqual(utf8.len(utf8str), 13)
    testing.expectEqual(utf8.offset(utf8str, 9), 11)
end)

testing.registerGlobalTest('memory limit', function()
    local ok, err = pcall(function()
        local t = {}
        local n = 1
        while true do
            t[n] = n
            n = n + 1
        end
    end)
    testing.expectEqual(ok, false, 'Script reaching memory limit should fail')
    testing.expectEqual(err, 'not enough memory')
end)

local function initPlayer()
    local player = world.players[1]
    player:teleport('', util.vector3(4096, 4096, 1745), util.transform.identity)
    coroutine.yield()
    return player
end

testing.registerGlobalTest('vfs', function()
    local file = 'test_vfs_dir/lines.txt'
    local nosuchfile = 'test_vfs_dir/nosuchfile'
    testing.expectEqual(vfs.fileExists(file), true, 'lines.txt should exist')
    testing.expectEqual(vfs.fileExists(nosuchfile), false, 'nosuchfile should not exist')

    local expectedLines = { '1', '2', '', '4' }
    local getLine = vfs.lines(file)
    for _,v in pairs(expectedLines) do
        testing.expectEqual(getLine(), v)
    end
    testing.expectEqual(getLine(), nil, 'All lines should have been read')
    local ok = pcall(function()
        vfs.lines(nosuchfile)
    end)
    testing.expectEqual(ok, false, 'Should not be able to read lines from nonexistent file')

    local getPath = vfs.pathsWithPrefix('test_vfs_dir/')
    testing.expectEqual(getPath(), file)
    testing.expectEqual(getPath(), nil, 'All paths should have been read')

    local handle = vfs.open(file)
    testing.expectEqual(vfs.type(handle), 'file', 'File should be open')
    testing.expectEqual(handle.fileName, file)

    local n1, n2, _, l3, l4 = handle:read("*n", "*number", "*l", "*line", "*l")
    testing.expectEqual(n1, 1)
    testing.expectEqual(n2, 2)
    testing.expectEqual(l3, '')
    testing.expectEqual(l4, '4')

    testing.expectEqual(handle:seek('set', 0), 0, 'Reading should happen from the start of the file')
    testing.expectEqual(handle:read("*a"), '1\n2\n\n4')

    testing.expectEqual(handle:close(), true, 'File should be closeable')
    testing.expectEqual(vfs.type(handle), 'closed file', 'File should be closed')

    handle = vfs.open(nosuchfile)
    testing.expectEqual(handle, nil, 'vfs.open should return nil on nonexistent files')

    getLine = vfs.open(file):lines()
    for _,v in pairs(expectedLines) do
        testing.expectEqual(getLine(), v)
    end
end)

testing.registerGlobalTest('commit crime', function()
    local player = initPlayer()
    testing.expectEqual(player == nil, false, 'A viable player reference should exist to run `commit crime`')
    testing.expectEqual(I.Crimes == nil, false, 'Crimes interface should be available in global contexts')

    -- Reset crime level to have a clean slate
    types.Player.setCrimeLevel(player, 0)
    testing.expectEqual(I.Crimes.commitCrime(player, { type = types.Player.OFFENSE_TYPE.Theft, victim = player, arg = 100}).wasCrimeSeen, false, "Running the crime with the player as the victim should not result in a seen crime")
    testing.expectEqual(I.Crimes.commitCrime(player, { type = types.Player.OFFENSE_TYPE.Theft, arg = 50 }).wasCrimeSeen, false, "Running the crime with no victim and a type shouldn't raise errors")
    testing.expectEqual(I.Crimes.commitCrime(player, { type = types.Player.OFFENSE_TYPE.Murder }).wasCrimeSeen, false, "Running a murder crime should work even without a victim")

    -- Create a mockup target for crimes
    local victim = world.createObject(types.NPC.record(player).id)
    victim:teleport(player.cell, player.position + util.vector3(0, 300, 0))
    coroutine.yield()

    -- Reset crime level for testing with a valid victim
    types.Player.setCrimeLevel(player, 0)
    testing.expectEqual(I.Crimes.commitCrime(player, { victim = victim, type = types.Player.OFFENSE_TYPE.Theft, arg = 50 }).wasCrimeSeen, true, "Running a crime with a valid victim should notify them when the player is not sneaking, even if it's not explicitly passed in")
    testing.expectEqual(types.Player.getCrimeLevel(player), 0, "Crime level should not change if the victim's alarm value is low and there's no other witnesses")
end)

testing.registerGlobalTest('record model property', function()
    local player = world.players[1]
    testing.expectEqual(types.NPC.record(player).model, 'meshes/basicplayer.dae')
end)

local function registerPlayerTest(name)
    testing.registerGlobalTest(name, function()
        local player = initPlayer()
        testing.runLocalTest(player, name)
    end)
end

registerPlayerTest('player yaw rotation')
registerPlayerTest('player pitch rotation')
registerPlayerTest('player pitch and yaw rotation')
registerPlayerTest('player rotation')
registerPlayerTest('player forward running')
registerPlayerTest('player diagonal walking')
registerPlayerTest('findPath')
registerPlayerTest('findRandomPointAroundCircle')
registerPlayerTest('castNavigationRay')
registerPlayerTest('findNearestNavMeshPosition')
registerPlayerTest('player memory limit')

testing.registerGlobalTest('player weapon attack', function()
    local player = initPlayer()
    world.createObject('basic_dagger1h', 1):moveInto(player)
    testing.runLocalTest(player, 'player weapon attack')
end)

testing.registerGlobalTest('load while teleporting - init player', function()
    local player = world.players[1]
    player:teleport('Museum of Wonders', util.vector3(0, -1500, 111), util.transform.rotateZ(math.rad(180)))
end)

testing.registerGlobalTest('load while teleporting - teleport', function()
    local player = world.players[1]
    local landracer = world.createObject('landracer')
    landracer:teleport(player.cell, player.position + util.vector3(0, 500, 0))
    coroutine.yield()

    local door = world.getObjectByFormId(core.getFormId('the_hub.omwaddon', 26))
    door:activateBy(player)
    coroutine.yield()

    landracer:teleport(player.cell, player.position)
end)

testing.registerGlobalTest('nan', function()
    local nan = 0.0 / 0.0
    local ok, err = pcall(function() world.setGameTimeScale(nan) end)
    testing.expectEqual(ok, false)
    testing.expectEqual(err, 'Value must be a finite number')
end)

return {
    engineHandlers = {
        onUpdate = testing.updateGlobal,
    },
    eventHandlers = testing.globalEventHandlers,
}
