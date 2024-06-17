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

return {
    {'Should support iteration over script variables', function()
        local mainVars = world.mwscript.getGlobalScript('main').variables
        local first, last, count = iterateOverVariables(mainVars)
        testing.expectEqual(first, nil)
        testing.expectEqual(last, nil)
        testing.expectEqual(count, 0)
        testing.expectEqual(count, #mainVars)

        local jiub = world.getObjectByFormId(core.getFormId('Morrowind.esm', 172867))
        local jiubVars = world.mwscript.getLocalScript(jiub).variables
        first, last, count = iterateOverVariables(jiubVars)
        print(first, last, count)
        testing.expectEqual(first, 'state')
        testing.expectEqual(last, 'timer')
        testing.expectEqual(count, 3)
        testing.expectEqual(count, #jiubVars)
        testing.expectEqual(jiubVars[1], jiubVars.state)
        testing.expectEqual(jiubVars[2], jiubVars.wandering)
        testing.expectEqual(jiubVars[3], jiubVars.timer)
    end},
}
