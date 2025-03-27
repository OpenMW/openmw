local core = require('openmw.core')
local util = require('openmw.util')

local M = {}

local menuTestsOrder = {}
local menuTests = {}

local globalTestsOrder = {}
local globalTests = {}
local globalTestRunner = nil
local currentGlobalTest = nil
local currentGlobalTestError = nil

local localTests = {}
local localTestRunner = nil
local currentLocalTest = nil
local currentLocalTestError = nil

local function makeTestCoroutine(fn)
    local co = coroutine.create(fn)
    return function()
        if coroutine.status(co) ~= 'dead' then
            coroutine.resume(co)
        end
    end
end

local function runTests(tests)
    for i, test in ipairs(tests) do
        local name, fn = unpack(test)
        print('TEST_START', i, name)
        local status, err = pcall(fn)
        if status then
            print('TEST_OK', i, name)
        else
            print('TEST_FAILED', i, name, err)
        end
    end
    core.quit()
end

function M.makeUpdateMenu()
    return makeTestCoroutine(function()
        print('Running menu tests...')
        runTests(menuTestsOrder)
    end)
end

function M.makeUpdateGlobal()
    return makeTestCoroutine(function()
        print('Running global tests...')
        runTests(globalTestsOrder)
    end)
end

function M.registerMenuTest(name, fn)
    menuTests[name] = fn
    table.insert(menuTestsOrder, {name, fn})
end

function M.runGlobalTest(name)
    currentGlobalTest = name
    currentGlobalTestError = nil
    core.sendGlobalEvent('runGlobalTest', name)
    while currentGlobalTest do
        coroutine.yield()
    end
    if currentGlobalTestError then
        error(currentGlobalTestError, 2)
    end
end

function M.registerGlobalTest(name, fn)
    globalTests[name] = fn
    table.insert(globalTestsOrder, {name, fn})
end

function M.updateGlobal()
    if globalTestRunner and coroutine.status(globalTestRunner) ~= 'dead' then
        coroutine.resume(globalTestRunner)
    else
        globalTestRunner = nil
    end
end

function M.runLocalTest(obj, name)
    currentLocalTest = name
    currentLocalTestError = nil
    obj:sendEvent('runLocalTest', name)
    while currentLocalTest do
        coroutine.yield()
    end
    if currentLocalTestError then
        error(currentLocalTestError, 2)
    end
end

function M.registerLocalTest(name, fn)
    localTests[name] = fn
end

function M.updateLocal()
    if localTestRunner and coroutine.status(localTestRunner) ~= 'dead' then
        if not core.isWorldPaused() then
            coroutine.resume(localTestRunner)
        end
    else
        localTestRunner = nil
    end
end

function M.expect(cond, msg)
    if not cond then
        error(msg or '"true" expected', 2)
    end
end

function M.expectEqualWithDelta(v1, v2, delta, msg)
    if math.abs(v1 - v2) > delta then
        error(string.format('%s: %f ~= %f', msg or '', v1, v2), 2)
    end
end

function M.expectAlmostEqual(v1, v2, msg)
    if math.abs(v1 - v2) / (math.abs(v1) + math.abs(v2)) > 0.05 then
        error(string.format('%s: %f ~= %f', msg or '', v1, v2), 2)
    end
end

function M.expectGreaterOrEqual(v1, v2, msg)
    if not (v1 >= v2) then
        error(string.format('%s: %f >= %f', msg or '', v1, v2), 2)
    end
end

function M.expectGreaterThan(v1, v2, msg)
    if not (v1 > v2) then
        error(string.format('%s: %s > %s', msg or '', v1, v2), 2)
    end
end

function M.expectLessOrEqual(v1, v2, msg)
    if not (v1 <= v2) then
        error(string.format('%s: %s <= %s', msg or '', v1, v2), 2)
    end
end

function M.expectEqual(v1, v2, msg)
    if not (v1 == v2) then
        error(string.format('%s: %s ~= %s', msg or '', v1, v2), 2)
    end
end

function M.expectNotEqual(v1, v2, msg)
    if v1 == v2 then
        error(string.format('%s: %s == %s', msg or '', v1, v2), 2)
    end
end

---
-- Verifies that given value matches provided matcher.
-- @function expectThat
-- @param value#any any value to match.
-- @param matcher#function a function returing empty string in the case of success or a message explaining the mismatch.
-- @param msg#string a message to prefix failure reason.
-- @usage
-- local matcher = function(actual)
--     if actual == 42 then
--         return ''
--     end
--     return string.format('%s is not 42', actual)
-- end
-- expectThat(42, matcher)
function M.expectThat(value, matcher, msg)
    local message = matcher(value)
    if message ~= '' then
        error(string.format('%s: actual does not match expected: %s', msg or 'Failure', message), 2)
    end
end

function M.formatActualExpected(actual, expected)
    return string.format('actual: %s, expected: %s', actual, expected)
end

-- used only in menu scripts
M.menuEventHandlers = {
    globalTestFinished = function(data)
        if data.name ~= currentGlobalTest then
            error(string.format('globalTestFinished with incorrect name %s, expected %s', data.name, currentGlobalTest), 2)
        end
        currentGlobalTest = nil
        currentGlobalTestError = data.errMsg
    end,
}

-- used only in global scripts
M.globalEventHandlers = {
    runGlobalTest = function(name)
        fn = globalTests[name]
        local types = require('openmw.types')
        local world = require('openmw.world')
        if not fn then
            types.Player.sendMenuEvent(world.players[1], 'globalTestFinished', {name=name, errMsg='Global test is not found'})
            return
        end
        globalTestRunner = coroutine.create(function()
            local status, err = pcall(fn)
            if status then
                err = nil
            end
            types.Player.sendMenuEvent(world.players[1], 'globalTestFinished', {name=name, errMsg=err})
        end)
    end,
    localTestFinished = function(data)
        if data.name ~= currentLocalTest then
            error(string.format('localTestFinished with incorrect name %s, expected %s', data.name, currentLocalTest), 2)
        end
        currentLocalTest = nil
        currentLocalTestError = data.errMsg
    end,
}

-- used only in local scripts
M.localEventHandlers = {
    runLocalTest = function(name)
        fn = localTests[name]
        if not fn then
            core.sendGlobalEvent('localTestFinished', {name=name, errMsg='Local test is not found'})
            return
        end
        localTestRunner = coroutine.create(function()
            local status, err = pcall(fn)
            if status then
                err = nil
            end
            core.sendGlobalEvent('localTestFinished', {name=name, errMsg=err})
        end)
    end,
}

return M
