local testing = require('testing_util')
local matchers = require('matchers')
local menu = require('openmw.menu')

testing.setSetupGlobalTest(function()
    menu.newGame({bypass = true})
    coroutine.yield()
end)

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
            'mwscript.omwaddon',
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

return {
    engineHandlers = {
        onFrame = testing.makeUpdateMenu(),
    },
    eventHandlers = testing.menuEventHandlers,
}
