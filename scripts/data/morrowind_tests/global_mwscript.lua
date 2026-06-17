local testing = require('testing_util')
local core = require('openmw.core')
local world = require('openmw.world')

function iterateOverVariables(variables)
    local first = nil
    local last = nil
    local count = 0
    for k, _ in pairs(variables) do
        first = first or k
        last = k
        count = count + 1
    end
    return first, last, count
end

testing.registerGlobalTest('[mwscript] Should support iteration over an empty set of script variables', function()
    local mainVars = world.mwscript.getGlobalScript('main').variables
    local first, last, count = iterateOverVariables(mainVars)
    testing.expectEqual(first, nil)
    testing.expectEqual(last, nil)
    testing.expectEqual(count, 0)
    testing.expectEqual(count, #mainVars)
end)

testing.registerGlobalTest('[mwscript] Should support iteration of script variables', function()
    local jiub = world.getObjectByFormId(core.getFormId('Morrowind.esm', 172867))
    local jiubVars = world.mwscript.getLocalScript(jiub).variables
    local first, last, count = iterateOverVariables(jiubVars)

    testing.expectEqual(first, 'state')
    testing.expectEqual(last, 'timer')
    testing.expectEqual(count, 3)
    testing.expectEqual(count, #jiubVars)
end)

testing.registerGlobalTest('[mwscript] Should support numeric and string indices for getting and setting', function()
    local jiub = world.getObjectByFormId(core.getFormId('Morrowind.esm', 172867))
    local jiubVars = world.mwscript.getLocalScript(jiub).variables

    testing.expectEqual(jiubVars[1], jiubVars.state)
    testing.expectEqual(jiubVars[2], jiubVars.wandering)
    testing.expectEqual(jiubVars[3], jiubVars.timer)

    jiubVars[1] = 123;
    testing.expectEqual(jiubVars.state, 123)
    jiubVars.wandering = 42;
    testing.expectEqual(jiubVars[2], 42)
    jiubVars[3] = 1.25;
    testing.expectEqual(jiubVars.timer, 1.25)
end)
