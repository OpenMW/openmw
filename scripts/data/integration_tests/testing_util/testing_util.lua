local core = require('openmw.core')

local M = {}
local currentLocalTest = nil
local currentLocalTestError = nil

function M.testRunner(tests)
    local fn = function()
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
    local co = coroutine.create(fn)
    return function()
        if coroutine.status(co) ~= 'dead' then
            coroutine.resume(co)
        end
    end
end

function M.runLocalTest(obj, name)
    currentLocalTest = name
    currentLocalTestError = nil
    obj:sendEvent('runLocalTest', name)
    while currentLocalTest do coroutine.yield() end
    if currentLocalTestError then error(currentLocalTestError, 2) end
end

function M.expect(cond, delta, msg)
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

local localTests = {}
local localTestRunner = nil

function M.registerLocalTest(name, fn)
    localTests[name] = fn
end

function M.updateLocal()
    if localTestRunner and coroutine.status(localTestRunner) ~= 'dead' then
        coroutine.resume(localTestRunner)
    else
        localTestRunner = nil
    end
end

M.eventHandlers = {
    runLocalTest = function(name)  -- used only in local scripts
        fn = localTests[name]
        if not fn then
            core.sendGlobalEvent('localTestFinished', {name=name, errMsg='Test not found'})
            return
        end
        localTestRunner = coroutine.create(function()
            local status, err = pcall(fn)
            if status then err = nil end
            core.sendGlobalEvent('localTestFinished', {name=name, errMsg=err})
        end)
    end,
    localTestFinished = function(data)  -- used only in global scripts
        if data.name ~= currentLocalTest then
            error(string.format('localTestFinished with incorrect name %s, expected %s', data.name, currentLocalTest))
        end
        currentLocalTest = nil
        currentLocalTestError = data.errMsg
    end,
}

return M
