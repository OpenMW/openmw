local content = require('openmw.content')

local function generateDefaultStatics()
    local statics = {
        -- Total conversions from SureAI lack marker records
        divinemarker = 'meshes/marker_divine.nif',
        doormarker = 'meshes/marker_arrow.nif',
        northmarker = 'meshes/marker_north.nif',
        templemarker = 'meshes/marker_temple.nif',
        travelmarker = 'meshes/marker_travel.nif',
    }
    for id, model in pairs(statics) do
        if content.statics.records[id] == nil then
            content.statics.records[id] = { model = model }
        end
    end
end

local function generateDefaultDoors()
    local doors = {
        prisonmarker = 'meshes/marker_prison.nif'
    }
    for id, model in pairs(doors) do
        if content.doors.records[id] == nil then
            content.doors.records[id] = { model = model }
        end
    end
end

return {
    engineHandlers = {
        onContentFilesLoaded = function()
            generateDefaultDoors()
            generateDefaultStatics()
        end
    }
}
