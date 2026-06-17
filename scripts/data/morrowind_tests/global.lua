local testing = require('testing_util')
local util = require('openmw.util')
local world = require('openmw.world')
local core = require('openmw.core')
local types = require('openmw.types')

if not core.contentFiles.has('Morrowind.esm') then
    error('This test requires Morrowind.esm')
end

require('global_issues')
require('global_dialogues')
require('global_mwscript')
require('global_regions')
require('global_weather')

return {
    engineHandlers = {
        onUpdate = testing.makeUpdateGlobal(),
    },
    eventHandlers = testing.globalEventHandlers,
}
