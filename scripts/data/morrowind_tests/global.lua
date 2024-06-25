local testing = require('testing_util')
local util = require('openmw.util')
local world = require('openmw.world')
local core = require('openmw.core')
local types = require('openmw.types')

if not core.contentFiles.has('Morrowind.esm') then
    error('This test requires Morrowind.esm')
end

function makeTests(modules)
    local tests = {}

    for _, moduleName in ipairs(modules) do
        local module = require(moduleName)
        for _, v in ipairs(module) do
            table.insert(tests, {string.format('[%s] %s', moduleName, v[1]), v[2]})
        end
    end

    return tests
end

local testModules = {
    'global_issues',
    'global_dialogues',
    'global_mwscript',
}

return {
    engineHandlers = {
        onUpdate = testing.testRunner(makeTests(testModules)),
    },
    eventHandlers = testing.eventHandlers,
}
