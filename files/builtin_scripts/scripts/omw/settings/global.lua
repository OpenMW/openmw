local storage = require('openmw.storage')

local common = require('scripts.omw.settings.common')

return {
    interfaceName = 'Settings',
    interface = {
        registerGroup = common.registerGroup,
        updateRendererArgument = common.updateRendererArgument,
    },
    engineHandlers = {
        onLoad = common.onLoad,
        onSave = common.onSave,
    },
    eventHandlers = {
        [common.setGlobalEvent] = function(e)
            storage.globalSection(e.groupKey):set(e.settingKey, e.value)
        end,
    },
}