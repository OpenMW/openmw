local world = require('openmw.world')

return {
    eventHandlers = {
        Pause = function(tag) world.pause(tag) end,
        Unpause = function(tag) world.unpause(tag) end,
        SetGameTimeScale = function(scale) world.setGameTimeScale(scale) end,
        SetSimulationTimeScale = function(scale) world.setSimulationTimeScale(scale) end,
        SpawnVfx = function(data)
            if data.options then
                world.vfx.spawn(data.model, data.position, data.options)
            else
                world.vfx.spawn(data.model, data.position)
            end
        end,
    },
}
