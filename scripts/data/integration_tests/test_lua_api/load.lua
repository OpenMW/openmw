local content = require('openmw.content')

local function testApparatusContent()
    local apparatuses = content.apparatuses.records
    apparatuses.OMW_Generated_Apparatus = {
        name = 'Generated apparatus',
        model = 'meshes/a/generated.nif',
        icon = 'icons/a/generated.dds',
        mwscript = 'generated_script',
        type = content.apparatuses.TYPE.Alembic,
        quality = 2.5,
        weight = 4,
        value = 300,
    }
    local apparatus = apparatuses.OMW_Generated_Apparatus
    apparatus.name = 'Modified apparatus'
    apparatus.model = 'meshes/a/modified.nif'
    apparatus.icon = 'icons/a/modified.dds'
    apparatus.mwscript = nil
    apparatus.type = content.apparatuses.TYPE.Calcinator
    apparatus.quality = 10
    apparatus.weight = 5.5
    apparatus.value = 450

    apparatuses.OMW_Templated_Apparatus = { template = apparatus, name = 'Templated apparatus' }
    apparatuses.OMW_Copied_Apparatus = apparatus
    apparatuses.OMW_Temporary_Apparatus = { type = content.apparatuses.TYPE.Retort }
    apparatuses.OMW_Temporary_Apparatus = nil

    local ok = pcall(function()
        apparatuses.OMW_Invalid_Apparatus = { type = 4 }
    end)
    assert(not ok, 'Invalid apparatus type should be rejected during creation')
    ok = pcall(function()
        apparatus.type = -1
    end)
    assert(not ok, 'Invalid apparatus type should be rejected during mutation')
end

return {
    engineHandlers = {
        onContentFilesLoaded = function()
            content.statics.records.OMW_Generated_Static = { model = 'meshes/generatedonload.nif' }
            testApparatusContent()
        end
    }
}
