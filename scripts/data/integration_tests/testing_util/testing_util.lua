local core = require('openmw.core')
local util = require('openmw.util')

local M = {}

local menuTestsOrder = {}
local menuTests = {}
local discoveredTests = {}
local setupGlobalTest = function() end

local globalTestsOrder = {}
local globalTests = {}
local globalTestRunner = nil
local currentGlobalTest = {}
local setupLocalTest = function() end
local discoveredLocalTests = {}

local localTestsOrder = {}
local localTests = {}
local localTestRunner = nil
local currentLocalTest = {}

local localDiscoveryTimeout = 10
local globalDiscoveryTimeout = localDiscoveryTimeout + 10

local function resumeChecked(co)
    local ok, err = coroutine.resume(co)
    if not ok then
        print('TEST_ERROR', tostring(err))
        core.quit()
    end
end

local function makeTestCoroutine(fn)
    local co = coroutine.create(fn)
    return function()
        if coroutine.status(co) ~= 'dead' then
            resumeChecked(co)
        end
    end
end

local function waitForDiscovery(mailbox, timeout, request, errMsg)
    mailbox.value = nil
    request()
    while mailbox.value == nil do
        timeout = timeout - 1
        if timeout == 0 then
            error(errMsg)
        end
        coroutine.yield()
    end
    return mailbox.value
end

local function runRemoteTest(state, name, request)
    state.name = name
    state.error = nil
    request()
    while state.name do
        coroutine.yield()
    end
    if state.error then
        error(state.error, 0)
    end
end

local function finishRemoteTest(state, data, eventName)
    if data.name ~= state.name then
        error(string.format('%s with incorrect name %s, expected %s', eventName, data.name, state.name), 2)
    end
    state.name = nil
    state.error = data.errMsg
end

local function loadTestConfig()
    local hasTestConfig, testConfig = pcall(require, 'test_config')
    if not hasTestConfig then
        if not tostring(testConfig):find('module not found') then
            error(testConfig)
        end
        return {}
    end
    return testConfig
end

local testConfig = loadTestConfig()

local function testNameMatchesFilter(name)
    return testConfig.filter == nil or name:find(testConfig.filter) ~= nil
end

local function runTests(tests)
    for i, test in ipairs(tests) do
        if testNameMatchesFilter(test.name) then
            print('TEST_START', i, test.name)
            local status, err = pcall(test.fn)
            if status then
                print('TEST_OK', i, test.name)
            else
                print('TEST_FAILED', i, test.name, err)
            end
        end
    end
    core.quit()
end

function M.makeUpdateMenu()
    return makeTestCoroutine(function()
        local menu = require('openmw.menu')
        print('Discovering tests...')
        menu.newGame({bypass = true})
        coroutine.yield()
        local tests = waitForDiscovery(discoveredTests, globalDiscoveryTimeout,
            function() core.sendGlobalEvent('discoverTests') end,
            'test discovery timed out: no testsDiscovered event from the global script')
        print('Discovered', #tests, 'tests')
        for _, name in ipairs(tests) do
            if menuTests[name] then
                error('discovered global/local test name collides with a menu test: ' .. name)
            end
            table.insert(menuTestsOrder, {
                name = name,
                fn = function()
                    setupGlobalTest()
                    M.runTest(name)
                end,
            })
        end
        if testConfig.list == true then
            for i, t in ipairs(menuTestsOrder) do
                if testNameMatchesFilter(t.name) then
                    print('TEST_FOUND', i, t.name)
                end
            end
            core.quit()
            return
        end
        print('Running tests...')
        runTests(menuTestsOrder)
    end)
end

function M.registerMenuTest(name, fn)
    menuTests[name] = fn
    table.insert(menuTestsOrder, {name = name, fn = fn})
end

function M.setSetupGlobalTest(fn)
    setupGlobalTest = fn
end

function M.runGlobalTest(name)
    runRemoteTest(currentGlobalTest, name, function() core.sendGlobalEvent('runGlobalTest', name) end)
end

function M.runTest(name)
    runRemoteTest(currentGlobalTest, name, function() core.sendGlobalEvent('runTest', name) end)
end

function M.registerGlobalTest(name, fn)
    globalTests[name] = fn
    table.insert(globalTestsOrder, name)
end

function M.registerGlobalTestStep(name, fn)
    globalTests[name] = fn
end

function M.setSetupLocalTest(fn)
    setupLocalTest = fn
end

function M.updateGlobal()
    if globalTestRunner and coroutine.status(globalTestRunner) ~= 'dead' then
        resumeChecked(globalTestRunner)
    else
        globalTestRunner = nil
    end
end

function M.runLocalTest(obj, name)
    runRemoteTest(currentLocalTest, name, function() obj:sendEvent('runLocalTest', name) end)
end

function M.registerLocalTest(name, fn)
    localTests[name] = fn
    table.insert(localTestsOrder, name)
end

function M.registerLocalTestStep(name, fn)
    localTests[name] = fn
end

function M.updateLocal()
    if localTestRunner and coroutine.status(localTestRunner) ~= 'dead' then
        if not core.isWorldPaused() then
            resumeChecked(localTestRunner)
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
        finishRemoteTest(currentGlobalTest, data, 'globalTestFinished')
    end,
    testsDiscovered = function(data)
        discoveredTests.value = data.tests
    end,
}

-- used only in global scripts
M.globalEventHandlers = {
    runTest = function(name)
        local types = require('openmw.types')
        local world = require('openmw.world')
        local fn = globalTests[name]
        if fn then
            globalTestRunner = coroutine.create(function()
                local status, err = pcall(fn)
                if status then
                    err = nil
                end
                types.Player.sendMenuEvent(world.players[1], 'globalTestFinished', {name=name, errMsg=err})
            end)
        else
            globalTestRunner = coroutine.create(function()
                local player = world.players[1]
                local status, err = pcall(function()
                    setupLocalTest(player)
                    M.runLocalTest(player, name)
                end)
                if status then
                    err = nil
                end
                types.Player.sendMenuEvent(world.players[1], 'globalTestFinished', {name=name, errMsg=err})
            end)
        end
    end,
    runGlobalTest = function(name)
        local fn = globalTests[name]
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
        finishRemoteTest(currentLocalTest, data, 'localTestFinished')
    end,
    discoverTests = function()
        globalTestRunner = coroutine.create(function()
            local types = require('openmw.types')
            local world = require('openmw.world')
            local localTestNames = waitForDiscovery(discoveredLocalTests, localDiscoveryTimeout,
                function() world.players[1]:sendEvent('discoverLocalTests') end,
                'local test discovery timed out: no localTestsDiscovered event from the local script')
            for _, name in ipairs(localTestNames) do
                if globalTests[name] then
                    error('local test name collides with a global test/step: ' .. name)
                end
            end
            local tests = {}
            for _, name in ipairs(globalTestsOrder) do
                table.insert(tests, name)
            end
            for _, name in ipairs(localTestNames) do
                table.insert(tests, name)
            end
            types.Player.sendMenuEvent(world.players[1], 'testsDiscovered', {tests = tests})
        end)
    end,
    localTestsDiscovered = function(data)
        discoveredLocalTests.value = data.tests
    end,
}

-- used only in local scripts
M.localEventHandlers = {
    runLocalTest = function(name)
        local fn = localTests[name]
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
    discoverLocalTests = function()
        core.sendGlobalEvent('localTestsDiscovered', {tests = localTestsOrder})
    end,
}

return M
