---
-- `openmw_aux.time` defines utility functions for timers.
-- Implementation can be found in `resources/vfs/openmw_aux/time.lua`.
-- @module time
-- @context global|menu|local
-- @usage local time = require('openmw_aux.time')

local time = {
    second = 1,
    minute = 60,
    hour = 3600,
    day = 3600 * 24,
    GameTime = 'GameTime',
    SimulationTime = 'SimulationTime',
}

---
-- Alias of async:registerTimerCallback ; register a function as a timer callback.
-- @function [parent=#time] registerTimerCallback
-- @param #string name
-- @param #function func
-- @return openmw.async#TimerCallback
function time.registerTimerCallback(name, fn)
    local async = require('openmw.async')
    return async:registerTimerCallback(name, fn)
end

---
-- Alias of async:newSimulationTimer ; call callback(arg) in `delay` game seconds.
-- Callback must be registered in advance.
-- @function [parent=#time] newGameTimer
-- @param #number delay
-- @param openmw.async#TimerCallback callback A callback returned by `registerTimerCallback`
-- @param arg An argument for `callback`; can be `nil`.
function time.newGameTimer(delay, callback, callbackArg)
    local async = require('openmw.async')
    return async:newGameTimer(delay, callback, callbackArg)
end

---
-- Alias of async:newSimulationTimer ; call callback(arg) in `delay` simulation seconds.
-- Callback must be registered in advance.
-- @function [parent=#time] newSimulationTimer
-- @param #number delay
-- @param openmw.async#TimerCallback callback A callback returned by `registerTimerCallback`
-- @param arg An argument for `callback`; can be `nil`.
function time.newSimulationTimer(delay, callback, callbackArg)
    local async = require('openmw.async')
    return async:newSimulationTimer(delay, callback, callbackArg)
end

---
-- Run given function repeatedly.
-- Note that loading a save stops the evaluation. If it should always work, call it during the initialization of the script (i.e. not in a handler)
-- @function [parent=#time] runRepeatedly
-- @param #function fn the function that should be called
-- @param #number period interval
-- @param #table options additional options `initialDelay` and `type`.
-- `initialDelay` - delay before the first call. If missed then the delay is a random number in range [0, N]. Randomization is used for performance reasons -- to prevent all scripts from doing time consuming operations at the same time.
-- `type` - either `time.SimulationTime` (by default, timer uses simulation time) or `time.GameTime` (timer uses game time).
-- @return #function a function without arguments that can be used to stop the periodical evaluation.
-- @usage
-- local stopFn = time.runRepeatedly(function() print('Test') end,
--                                   5 * time.second)  -- print 'Test' every 5 seconds
-- stopFn()  -- stop printing 'Test'
-- time.runRepeatedly(  -- print 'Test' every 5 minutes with initial 30 second delay
--     function() print('Test2') end, 5 * time.minute,
--     { initialDelay = 30 * time.second })
-- @usage
-- local timeBeforeMidnight = time.day - core.getGameTime() % time.day
-- time.runRepeatedly(doSomething, time.day, {
--     initialDelay = timeBeforeMidnight,
--     type = time.GameTime,
-- })  -- call `doSomething` at the end of every game day.
function time.runRepeatedly(fn, period, options)
    if period <= 0 then
        error('Period must be positive. If you want it to be as small '..
              'as possible, use the engine handler `onUpdate` instead', 2)
    end
    local async = require('openmw.async')
    local core = require('openmw.core')
    local initialDelay = (options and options.initialDelay) or math.random() * period
    local getTimeFn, newTimerFn
    if (options and options.type) == time.GameTime then
        getTimeFn = core.getGameTime
        newTimerFn = async.newUnsavableGameTimer
    else
        getTimeFn = core.getSimulationTime
        newTimerFn = async.newUnsavableSimulationTimer
    end
    local baseTime = getTimeFn() + initialDelay
    local breakFlag = false
    local wrappedFn
    wrappedFn = function()
        if breakFlag then return end
        fn()
        local nextDelay = 1.5 * period - math.fmod(getTimeFn() - baseTime + period / 2, period)
        newTimerFn(async, nextDelay, wrappedFn)
    end
    newTimerFn(async, initialDelay, wrappedFn)
    return function() breakFlag = true end
end

return time

