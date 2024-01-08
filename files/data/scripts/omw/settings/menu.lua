local common = require('scripts.omw.settings.common')
local render = require('scripts.omw.settings.render')

require('scripts.omw.settings.renderers')(render.registerRenderer)

return {
    interfaceName = 'Settings',
    interface = {
        version = 0,
        registerPage = render.registerPage,
        registerRenderer = render.registerRenderer,
        registerGroup = common.registerGroup,
        updateRendererArgument = common.updateRendererArgument,
    },
    engineHandlers = {
        onLoad = common.onLoad,
        onSave = common.onSave,
    },
}
