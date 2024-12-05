local core = require('openmw.core')
local util = require('openmw.util')

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
    while currentLocalTest do
        coroutine.yield()
    end
    if currentLocalTestError then
        error(currentLocalTestError, 2)
    end
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

function M.expectNotEqual(v1, v2, msg)
    if v1 == v2 then
        error(string.format('%s: %s == %s', msg or '', v1, v2), 2)
    end
end

function M.closeToVector(expected, maxDistance)
    return function(actual)
        local distance = (expected - actual):length()
        if distance <= maxDistance then
            return ''
        end
        return string.format('%s is too far from expected %s: %s > %s', actual, expected, distance, maxDistance)
    end
end

---
-- Matcher verifying that given value is an array each element of which matches elements of expected.
-- @function elementsAreArray
-- @param expected#array of values or matcher functions.
-- @usage
-- local t = {42, 13}
-- local matcher = function(actual)
--     if actual ~= 42 then
--         return string.format('%s is not 42', actual)
--     end
--     return ''
-- end
-- expectThat({42, 13}, elementsAreArray({matcher, 13}))
function M.elementsAreArray(expected)
    local expected_matchers = {}
    for i, v in ipairs(expected) do
        if type(v) == 'function' then
            expected_matchers[i] = v
        else
            expected_matchers[i] = function (other)
                if expected[i].__eq(expected[i], other) then
                    return ''
                end
                return string.format('%s element %s does no match expected: %s', i, other, expected[i])
            end
        end
    end
    return function(actual)
        if #actual < #expected_matchers then
            return string.format('number of elements is less than expected: %s < %s', #actual, #expected_matchers)
        end
        local message = ''
        for i, v in ipairs(actual) do
            if i > #expected_matchers then
                message = string.format('%s\n%s element is out of expected range: %s', message, i, #expected_matchers)
                break
            end
            local match_message = expected_matchers[i](v)
            if match_message ~= '' then
                message = string.format('%s\n%s', message, match_message)
            end
        end
        return message
    end
end

---
-- Matcher verifying that given number is not a nan.
-- @function isNotNan
-- @usage
-- expectThat(value, isNotNan())
function M.isNotNan(expected)
    return function(actual)
        if actual ~= actual then
            return 'actual value is nan, expected to be not nan'
        end
        return ''
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

local localTests = {}
local localTestRunner = nil

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

M.eventHandlers = {
    runLocalTest = function(name)  -- used only in local scripts
        fn = localTests[name]
        if not fn then
            core.sendGlobalEvent('localTestFinished', {name=name, errMsg='Test not found'})
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
    localTestFinished = function(data)  -- used only in global scripts
        if data.name ~= currentLocalTest then
            error(string.format('localTestFinished with incorrect name %s, expected %s', data.name, currentLocalTest))
        end
        currentLocalTest = nil
        currentLocalTestError = data.errMsg
    end,
}

return M
