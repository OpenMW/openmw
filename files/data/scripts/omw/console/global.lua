local util = require('openmw.util')

local player = nil

local function printToConsole(...)
    local strs = {}
    for i = 1, select('#', ...) do
        strs[i] = tostring(select(i, ...))
    end
    player:sendEvent('OMWConsolePrint', table.concat(strs, '\t'))
end

local function printRes(...)
    if select('#', ...) >= 0 then
        printToConsole(...)
    end
end

local env = {
    I = require('openmw.interfaces'),
    util = require('openmw.util'),
    storage = require('openmw.storage'),
    core = require('openmw.core'),
    types = require('openmw.types'),
    vfs = require('openmw.vfs'),
    markup = require('openmw.markup'),
    async = require('openmw.async'),
    world = require('openmw.world'),
    aux_util = require('openmw_aux.util'),
    calendar = require('openmw_aux.calendar'),
    time = require('openmw_aux.time'),
    view = require('openmw_aux.util').deepToString,
    print = printToConsole,
    exit = function() player:sendEvent('OMWConsoleExit') end,
    help = function() player:sendEvent('OMWConsoleHelp') end,
}
env._G = env
setmetatable(env, {__index = _G, __metatable = false})
_G = nil

local function executeLuaCode(code)
    local fn
    local ok, err = pcall(function() fn = util.loadCode('return ' .. code, env) end)
    if ok then
        ok, err = pcall(function() printRes(fn()) end)
    else
        ok, err = pcall(function() util.loadCode(code, env)() end)
    end
    if not ok then
        player:sendEvent('OMWConsoleError', err)
    end
end

return {
    eventHandlers = {
        OMWConsoleEval = function(data)
            player = data.player
            env.selected = data.selected
            executeLuaCode(data.code)
            if env.selected ~= data.selected then
                local ok, err = pcall(function() player:sendEvent('OMWConsoleSetSelected', env.selected) end)
                if not ok then player:sendEvent('OMWConsoleError', err) end
            end
        end,
        OMWConsoleStartLocal = function(data)
            player = data.player
            ok, err = pcall(function()
                if not data.selected:hasScript('scripts/omw/console/local.lua') then
                    data.selected:addScript('scripts/omw/console/local.lua')
                end
            end)
            if ok then
                player:sendEvent('OMWConsoleSetContext', data.selected)
            else
                player:sendEvent('OMWConsoleError', err)
            end
        end,
        OMWConsoleStopLocal = function(obj)
            if obj:hasScript('scripts/omw/console/local.lua') then
                obj:removeScript('scripts/omw/console/local.lua')
            end
        end,
    },
}

