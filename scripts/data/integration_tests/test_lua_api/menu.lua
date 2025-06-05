local testing = require('testing_util')
local matchers = require('matchers')
local menu = require('openmw.menu')

testing.registerMenuTest('save and load', function()
    menu.newGame()
    coroutine.yield()
    menu.saveGame('save and load')
    coroutine.yield()

    local directorySaves = {}
    directorySaves['save_and_load.omwsave'] = {
        playerName = '',
        playerLevel = 1,
        timePlayed = 0,
        description = 'save and load',
        contentFiles = {
            'builtin.omwscripts',
            'template.omwgame',
            'landracer.omwaddon',
            'the_hub.omwaddon',
            'test_lua_api.omwscripts',
        },
        creationTime = matchers.isAny(),
    }
    local expectedAllSaves = {}
    expectedAllSaves[' - 1'] = directorySaves

    testing.expectThat(menu.getAllSaves(), matchers.equalTo(expectedAllSaves))

    menu.loadGame(' - 1', 'save_and_load.omwsave')
    coroutine.yield()

    menu.deleteGame(' - 1', 'save_and_load.omwsave')
    testing.expectThat(menu.getAllSaves(), matchers.equalTo({}))
end)

testing.registerMenuTest('load while teleporting', function()
    menu.newGame()
    coroutine.yield()

    testing.runGlobalTest('load while teleporting - init player')

    menu.saveGame('load while teleporting')
    coroutine.yield()

    testing.runGlobalTest('load while teleporting - teleport')

    menu.loadGame(' - 1', 'load_while_teleporting.omwsave')
    coroutine.yield()

    menu.deleteGame(' - 1', 'load_while_teleporting.omwsave')
end)

local function registerGlobalTest(name, description)
   testing.registerMenuTest(description or name, function()
       menu.newGame()
       coroutine.yield()
       testing.runGlobalTest(name)
   end)
end

registerGlobalTest('timers')
registerGlobalTest('teleport')
registerGlobalTest('getGMST')
registerGlobalTest('MWScript')
registerGlobalTest('record stores')
registerGlobalTest('record creation')
registerGlobalTest('UTF-8 characters')
registerGlobalTest('UTF-8 strings')
registerGlobalTest('memory limit')
registerGlobalTest('vfs')
registerGlobalTest('commit crime')
registerGlobalTest('record model property')
registerGlobalTest('nan', 'world.setGameTimeScale should not accept nan')

registerGlobalTest('player yaw rotation', 'rotating player with controls.yawChange should change rotation')
registerGlobalTest('player pitch rotation', 'rotating player with controls.pitchChange should change rotation')
registerGlobalTest('player pitch and yaw rotation', 'rotating player with controls.pitchChange and controls.yawChange should change rotation')
registerGlobalTest('player rotation', 'rotating player should not lead to nan rotation')
registerGlobalTest('player forward running')
registerGlobalTest('player diagonal walking')
registerGlobalTest('findPath')
registerGlobalTest('findRandomPointAroundCircle')
registerGlobalTest('castNavigationRay')
registerGlobalTest('findNearestNavMeshPosition')
registerGlobalTest('player memory limit')
registerGlobalTest('player weapon attack', 'player with equipped weapon on attack should damage health of other actors')

return {
    engineHandlers = {
        onFrame = testing.makeUpdateMenu(),
    },
    eventHandlers = testing.menuEventHandlers,
}
