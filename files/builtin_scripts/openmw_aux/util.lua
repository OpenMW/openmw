-------------------------------------------------------------------------------
-- `openmw_aux.util` defines utility functions that are implemented in Lua rather than in C++.
-- Implementation can be found in `resources/vfs/openmw_aux/util.lua`.
-- @module util
-- @usage local aux_util = require('openmw_aux.util')

local aux_util = {}

-------------------------------------------------------------------------------
-- Finds the nearest object to the given point in the given list.
-- Ignores cells, uses only coordinates. Returns the nearest object,
-- and the distance to it. If objectList is empty, returns nil.
-- @function [parent=#util] findNearestTo
-- @param openmw.util#Vector3 point
-- @param openmw.core#ObjectList objectList
-- @return openmw.core#GameObject, #number the nearest object and the distance
function aux_util.findNearestTo(point, objectList)
    local res = nil
    local resDist = nil
    for i = 1, #objectList do
        local obj = objectList[i]
        local dist = (obj.position - point):length()
        if i == 1 or dist < resDist then
            res = obj
            resDist = dist
        end
    end
    return res, resDist
end

-------------------------------------------------------------------------------
-- Runs given function every N game seconds (seconds when the game is not paused).
-- Note that loading a save stops the evaluation. If it should work always, call it in 2 places --
-- when a script starts and in the engine handler `onLoad`.
-- @function [parent=#util] runEveryNSeconds
-- @param #number N interval in seconds
-- @param #function fn the function that should be called every N seconds
-- @param #number initialDelay optional argument -- delay in seconds before the first call. If missed then the delay is a random number in range [0, N]. Randomization is used for performance reasons -- to prevent all scripts from doing time consuming operations at the same time.
-- @return #function a function without arguments that can be used to stop the periodical evaluation.
-- @usage
-- local stopFn = aux_util.runEveryNSeconds(5, function() print('Test') end)  -- print 'Test' every 5 seconds
-- stopFn()  -- stop printing 'Test'
-- aux_util.runEveryNSeconds(5, function() print('Test2') end, 1)  -- print 'Test' every 5 seconds starting from the next second
function aux_util.runEveryNSeconds(N, fn, initialDelay)
    if N <= 0 then
        error('Interval must be positive. If you want it to be as small '..
              'as possible, use the engine handler `onUpdate` instead', 2)
    end
    local async = require('openmw.async')
    local core = require('openmw.core')
    local breakFlag = false
    local initialDelay = initialDelay or math.random() * N
    local baseTime = core.getGameTimeInSeconds() + initialDelay
    local wrappedFn
    wrappedFn = function()
        if breakFlag then return end
        fn()
        local nextDelay = 1.5 * N - math.fmod(core.getGameTimeInSeconds() - baseTime + N / 2, N)
        async:newUnsavableTimerInSeconds(nextDelay, wrappedFn)
    end
    async:newUnsavableTimerInSeconds(initialDelay, wrappedFn)
    return function() breakFlag = true end
end

-------------------------------------------------------------------------------
-- Runs given function every N game hours.
-- Note that loading a save stops the evaluation. If it should work always, call it in 2 places --
-- when a script starts and in the engine handler `onLoad`.
-- @function [parent=#util] runEveryNHours
-- @param #number N interval in game hours
-- @param #function fn the function that should be called every N game hours
-- @param #number initialDelay optional argument -- delay in game hours before the first call. If missed then the delay is a random number in range [0, N]. Randomization is used for performance reasons -- to prevent all scripts from doing time consuming operations at the same time.
-- @return #function a function without arguments that can be used to stop the periodical evaluation.
-- @usage
-- local timeBeforeMidnight = 24 - math.fmod(core.getGameTimeInHours(), 24)
-- aux_util.runEveryNHours(24, doSomething, timeBeforeMidnight)  -- call `doSomething` at the end of every game day.
function aux_util.runEveryNHours(N, fn, initialDelay)
    if N <= 0 then
        error('Interval must be positive. If you want it to be as small '..
              'as possible, use the engine handler `onUpdate` instead', 2)
    end
    local async = require('openmw.async')
    local core = require('openmw.core')
    local breakFlag = false
    local initialDelay = initialDelay or math.random() * N
    local baseTime = core.getGameTimeInHours() + initialDelay
    local wrappedFn
    wrappedFn = function()
        if breakFlag then return end
        fn()
        local nextDelay = 1.5 * N - math.fmod(core.getGameTimeInHours() - baseTime + N / 2, N)
        async:newUnsavableTimerInHours(nextDelay, wrappedFn)
    end
    async:newUnsavableTimerInHours(initialDelay, wrappedFn)
    return function() breakFlag = true end
end

return aux_util

