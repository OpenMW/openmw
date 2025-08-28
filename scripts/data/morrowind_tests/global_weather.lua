local testing = require('testing_util')
local core = require('openmw.core')
local util = require('openmw.util')

testing.registerGlobalTest('[weather] Should be able to change weather records', function()
    local record = core.weather.records['clear']
    testing.expect(record ~= nil, 'Clear weather to exist')
    record.skyColor.sunrise = util.color.hex('2227FF')
    testing.expectEqual(record.skyColor.sunrise, util.color.hex('2227FF'))
end)
