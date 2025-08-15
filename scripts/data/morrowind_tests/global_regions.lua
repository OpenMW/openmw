local testing = require('testing_util')
local core = require('openmw.core')
local util = require('openmw.util')

testing.registerGlobalTest('[regions] Should expose Bitter Coast', function()
    local record = core.regions.records['bitter coast region']
    testing.expect(record ~= nil, 'Bitter Coast Region to exist')
    testing.expectEqual(record.mapColor, util.color.hex('2227FF'))
    for id, probability in pairs(record.weatherProbabilities) do
        testing.expect(core.weather.records[id] ~= nil, 'weather ' .. id .. 'to exist')
    end
    testing.expectEqual(record.weatherProbabilities.cloudy, 60, 'cloudy weather to occur 60% of the time')
end)
