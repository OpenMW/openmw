local content = require('openmw.content')

return {
    engineHandlers = {
        onContentFilesLoaded = function()
            content.statics.records.OMW_Generated_Static = { model = 'meshes/generatedonload.nif' }
        end
    }
}
