local testing = require('testing_util')
local menu = require('openmw.menu')

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
