local menu = require('openmw.menu')
local ui = require('openmw.ui')
local util = require('openmw.util')

local menuModeName = 'Lua[Menu]'

local function printHelp()
    local msg = [[
This is the built-in Lua interpreter.
help() - print this message
exit() - exit Lua mode
view(_G) - print content of the table `_G` (current environment)
    standard libraries (math, string, etc.) are loaded by default but not visible in `_G`
view(menu, 2) - print table `menu` (i.e. `openmw.menu`) and its subtables (2 - traversal depth)]]
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

local function exitLuaMenuMode()
    ui.setConsoleMode('')
    ui.printToConsole('Lua mode OFF', ui.CONSOLE_COLOR.Success)
end

local function enterLuaMenuMode()
    ui.printToConsole('Lua mode ON, use exit() to return, help() for more info', ui.CONSOLE_COLOR.Success)
    ui.printToConsole('Context: Menu', ui.CONSOLE_COLOR.Success)
    ui.setConsoleMode(menuModeName)
end

local env = {
    I = require('openmw.interfaces'),
    menu = require('openmw.menu'),
    util = require('openmw.util'),
    core = require('openmw.core'),
    storage = require('openmw.storage'),
    vfs = require('openmw.vfs'),
    markup = require('openmw.markup'),
    ambient = require('openmw.ambient'),
    async = require('openmw.async'),
    ui = require('openmw.ui'),
    input = require('openmw.input'),
    aux_util = require('openmw_aux.util'),
    calendar = require('openmw_aux.calendar'),
    time = require('openmw_aux.time'),
    view = require('openmw_aux.util').deepToString,
    print = printToConsole,
    exit = exitLuaMenuMode,
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

local usageInfo = [[
Usage: 'lua menu' or 'luam' - enter menu context
Other contexts are available only when the game is started:
       'lua player' or 'luap' - enter player context
       'lua global' or 'luag' - enter global context
       'lua selected' or 'luas' - enter local context on the selected object]]

local function onConsoleCommand(mode, cmd)
    if mode == '' then
        cmd, arg = cmd:lower():match('(%w+) *(%w*)')
        if (cmd == 'lua' and arg == 'menu') or cmd == 'luam' then
            enterLuaMenuMode()
        elseif menu.getState() == menu.STATE.NoGame and (cmd == 'lua' or cmd == 'luap' or cmd == 'luas' or cmd == 'luag') then
            ui.printToConsole(usageInfo, ui.CONSOLE_COLOR.Info)
        end
    elseif mode == menuModeName then
        if cmd == 'exit()' then
            exitLuaMenuMode()
        else
            executeLuaCode(cmd)
        end
    end
end

local function onStateChanged()
    local mode = ui.getConsoleMode()
    if menu.getState() ~= menu.STATE.Ended and mode ~= menuModeName then
        -- When a new game started or loaded reset console mode (except of `luam`) because
        -- other modes become invalid after restarting Lua scripts.
        ui.setConsoleMode('')
    end
end

return {
    engineHandlers = {
        onConsoleCommand = onConsoleCommand,
        onStateChanged = onStateChanged,
    },
}
