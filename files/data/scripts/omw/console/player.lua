local ui = require('openmw.ui')
local util = require('openmw.util')
local self = require('openmw.self')
local core = require('openmw.core')

local function printHelp()
    local msg = [[
This is the built-in Lua interpreter.
help() - print this message
exit() - exit Lua mode
selected - currently selected object (click on any object to change)
view(_G) - print content of the table `_G` (current environment)
    standard libraries (math, string, etc.) are loaded by default but not visible in `_G`
view(types, 2) - print table `types` (i.e. `openmw.types`) and its subtables (2 - traversal depth)]]
    ui.printToConsole(msg, ui.CONSOLE_COLOR.Info)
end

local function printToConsole(...)
    local strs = {}
    for i = 1, select('#', ...) do
        strs[i] = tostring(select(i, ...))
    end
    return ui.printToConsole(table.concat(strs, '\t'), ui.CONSOLE_COLOR.Info)
end

local function printRes(...)
    if select('#', ...) >= 0 then
        printToConsole(...)
    end
end

local currentSelf = nil
local currentMode = ''

local function exitLuaMode()
    currentSelf = nil
    currentMode = ''
    ui.setConsoleMode('')
    ui.printToConsole('Lua mode OFF', ui.CONSOLE_COLOR.Success)
end

local function setContext(obj)
    ui.printToConsole('Lua mode ON, use exit() to return, help() for more info', ui.CONSOLE_COLOR.Success)
    if obj == self then
        currentMode = 'Lua[Player]'
        ui.printToConsole('Context: Player', ui.CONSOLE_COLOR.Success)
    elseif obj then
        if not obj:isValid() then error('Object not available') end
        currentMode = 'Lua['..obj.recordId..']'
        ui.printToConsole('Context: Local['..tostring(obj)..']', ui.CONSOLE_COLOR.Success)
    else
        currentMode = 'Lua[Global]'
        ui.printToConsole('Context: Global', ui.CONSOLE_COLOR.Success)
    end
    currentSelf = obj
    ui.setConsoleMode(currentMode)
end

local function setSelected(obj)
    local ok, err = pcall(function() ui.setConsoleSelectedObject(obj) end)
    if ok then
        ui.printToConsole('Selected object changed', ui.CONSOLE_COLOR.Success)
    else
        ui.printToConsole(err, ui.CONSOLE_COLOR.Error)
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
    ambient = require('openmw.ambient'),
    async = require('openmw.async'),
    nearby = require('openmw.nearby'),
    self = require('openmw.self'),
    input = require('openmw.input'),
    postprocessing = require('openmw.postprocessing'),
    anim = require('openmw.animation'),
    ui = require('openmw.ui'),
    camera = require('openmw.camera'),
    aux_util = require('openmw_aux.util'),
    debug = require('openmw.debug'),
    calendar = require('openmw_aux.calendar'),
    time = require('openmw_aux.time'),
    view = require('openmw_aux.util').deepToString,
    print = printToConsole,
    exit = exitLuaMode,
    help = printHelp,
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
        ui.printToConsole(err, ui.CONSOLE_COLOR.Error)
    end
end

local function onConsoleCommand(mode, cmd, selectedObject)
    env.selected = selectedObject
    if mode == '' then
        cmd, arg = cmd:lower():match('(%w+) *(%w*)')
        if cmd == 'lua' then
            if arg == 'player' then
                cmd = 'luap'
            elseif arg == 'global' then
                cmd = 'luag'
            elseif arg == 'selected' then
                cmd = 'luas'
            elseif arg == 'menu' then
                -- handled in menu.lua
            else
                local msg = [[
Usage: 'lua menu' or 'luam' - enter menu context
       'lua player' or 'luap' - enter player context
       'lua global' or 'luag' - enter global context
       'lua selected' or 'luas' - enter local context on the selected object]]
                ui.printToConsole(msg, ui.CONSOLE_COLOR.Info)
            end
        end
        if cmd == 'luap' or (cmd == 'luas' and selectedObject == self.object) then
            setContext(self)
        elseif cmd == 'luag' then
            setContext()
        elseif cmd == 'luas' then
            if selectedObject then
                core.sendGlobalEvent('OMWConsoleStartLocal', {player=self.object, selected=selectedObject})
            else
                ui.printToConsole('No selected object', ui.CONSOLE_COLOR.Error)
            end
        end
    elseif mode == currentMode then
        if cmd == 'exit()' then
            exitLuaMode()
        elseif currentSelf == self then
            executeLuaCode(cmd)
            if env.selected ~= selectedObject then setSelected(env.selected) end
        elseif currentSelf then
            currentSelf:sendEvent('OMWConsoleEval', {player=self.object, code=cmd, selected=selectedObject})
        else
            core.sendGlobalEvent('OMWConsoleEval', {player=self.object, code=cmd, selected=selectedObject})
        end
    end
end

return {
    engineHandlers = {onConsoleCommand = onConsoleCommand},
    eventHandlers = {
        OMWConsolePrint = function(msg) ui.printToConsole(tostring(msg), ui.CONSOLE_COLOR.Info) end,
        OMWConsoleError = function(msg) ui.printToConsole(tostring(msg), ui.CONSOLE_COLOR.Error) end,
        OMWConsoleSetContext = setContext,
        OMWConsoleSetSelected = setSelected,
        OMWConsoleExit = exitLuaMode,
        OMWConsoleHelp = printHelp,
    }
}
