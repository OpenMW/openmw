---
-- `openmw_aux.calendar` defines utility functions for formatting game time.
-- Implementation can be found in `resources/vfs/openmw_aux/calendar.lua`.
-- @module calendar
-- @context global|menu|local
-- @usage local calendar = require('openmw_aux.calendar')

local core = require('openmw.core')
local time = require('openmw_aux.time')
local conf = require('openmw_aux.calendarconfig')
local l10n = core.l10n('Calendar')

local monthsDuration = conf.monthsDuration
local daysInYear = 0
for _, d in ipairs(monthsDuration) do daysInYear = daysInYear + d end

local function gameTime(t)
    if not t then
        return core.getGameTime()
    else
        local days = (t.year or 0) * daysInYear + (t.day or 0)
        for i = 1, (t.month or 1)-1 do
            days = days + monthsDuration[i]
        end
        return days * time.day + (t.hour or 0) * time.hour +
               (t.min or 0) * time.minute + (t.sec or 0) * time.second
    end
end

local function defaultDateFormat(t)
    return l10n('dateFormat', {
        day = t.day,
        month = l10n('month' .. t.month),
        monthInGenitive = l10n('monthInGenitive' .. t.month),
        year = t.year,
    })
end

local function formatGameTime(formatStr, timestamp)
    timestamp = timestamp or core.getGameTime()

    local t = {}
    local day = math.floor(timestamp / time.day) + conf.startingYearDay - 1
    t.year = math.floor(day / daysInYear) + conf.startingYear
    t.yday = day % daysInYear + 1
    t.wday = (day + conf.startingWeekDay - conf.startingYearDay) % conf.daysInWeek + 1
    timestamp = timestamp % time.day
    t.hour = math.floor(timestamp / time.hour)
    timestamp = timestamp % time.hour
    t.min = math.floor(timestamp / time.minute)
    t.sec = math.floor(timestamp) % time.minute

    t.day = t.yday
    t.month = 1
    while t.day > monthsDuration[t.month] do
        t.day = t.day - monthsDuration[t.month]
        t.month = t.month + 1
    end

    if formatStr == '*t' then return t end

    local replFn = function(tag)
        if tag == '%a' or tag == '%A' then return l10n('weekday' .. t.wday) end
        if tag == '%b' or tag == '%B' then return l10n('monthInGenitive' .. t.month) end
        if tag == '%c' then
            return string.format('%02d:%02d %s', t.hour, t.min, defaultDateFormat(t))
        end
        if tag == '%d' then return string.format('%02d', t.day) end
        if tag == '%e' then return string.format('%2d', t.day) end
        if tag == '%H' then return string.format('%02d', t.hour) end
        if tag == '%I' then return string.format('%02d', (t.hour - 1) % 12 + 1) end
        if tag == '%M' then return string.format('%02d', t.min) end
        if tag == '%m' then return string.format('%02d', t.month) end
        if tag == '%p' then
            if t.hour >= 0 and t.hour < 12 then
                return l10n('am')
            else
                return l10n('pm')
            end
        end
        if tag == '%S' then return string.format('%02d', t.sec) end
        if tag == '%w' then return t.wday - 1 end
        if tag == '%x' then return defaultDateFormat(t) end
        if tag == '%X' then return string.format('%02d:%02d', t.hour, t.min) end
        if tag == '%Y' then return t.year end
        if tag == '%y' then return string.format('%02d', t.year % 100) end
        if tag == '%%' then return '%' end
        error('Unknown tag "'..tag..'"')
    end

    local res, _ = string.gsub(formatStr or '%c', '%%.', replFn)
    return res
end

return {
    --- An equivalent of `os.time` for game time.
    -- See [https://www.lua.org/pil/22.1.html](https://www.lua.org/pil/22.1.html)
    -- @function [parent=#calendar] gameTime
    -- @param #table table a table which describes a date (optional).
    -- @return #number a timestamp.
    gameTime = gameTime,

    --- An equivalent of `os.date` for game time.
    -- See [https://www.lua.org/pil/22.1.html](https://www.lua.org/pil/22.1.html).
    -- It is a slow function. Please try not to use it in every frame.
    -- @function [parent=#calendar] formatGameTime
    -- @param #string format format of date (optional)
    -- @param #number time time to format (default value is current time)
    -- @return #string a formatted string representation of `time`.
    formatGameTime = formatGameTime,

    --- The number of months in a year
    -- @field [parent=#calendar] #number monthCount
    monthCount = #monthsDuration,

    --- The number of days in a year
    -- @field [parent=#calendar] #number daysInYear
    daysInYear = daysInYear,

    --- The number of days in a week
    -- @field [parent=#calendar] #number daysInWeek
    daysInWeek = conf.daysInWeek,

    --- The number of days in a month
    -- @function [parent=#calendar] daysInMonth
    -- @param monthIndex
    -- @return #number
    daysInMonth = function(m)
        return monthsDuration[(m-1) % #monthsDuration + 1]
    end,

    --- The name of a month
    -- @function [parent=#calendar] monthName
    -- @param monthIndex
    -- @return #string
    monthName = function(m)
        return l10n('month' .. ((m-1) % #monthsDuration + 1))
    end,

    --- The name of a month in genitive (for English is the same as `monthName`, but in some languages the form can differ).
    -- @function [parent=#calendar] monthNameInGenitive
    -- @param monthIndex
    -- @return #string
    monthNameInGenitive = function(m)
        return l10n('monthInGenitive' .. ((m-1) % #monthsDuration + 1))
    end,

    --- The name of a weekday
    -- @function [parent=#calendar] weekdayName
    -- @param dayIndex
    -- @return #string
    weekdayName = function(d)
        return l10n('weekday' .. ((d-1) % conf.daysInWeek + 1))
    end,
}

