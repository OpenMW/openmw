local types = require('openmw.types')
local self = require('openmw.self')

local common = require('scripts.omw.settings.common')

local function registerPage(options)
    types.Player.sendMenuEvent(self, common.registerPageEvent, options)
end

---
-- @type PageOptions
-- @field #string key A unique key
-- @field #string l10n A localization context (an argument of core.l10n)
-- @field #string name A key from the localization context
-- @field #string description A key from the localization context (optional, can be `nil`)

---
-- @type GroupOptions
-- @field #string key A unique key, starts with "Settings" by convention
-- @field #string l10n A localization context (an argument of core.l10n)
-- @field #string name A key from the localization context
-- @field #string description A key from the localization context (optional, can be `nil`)
-- @field #string page Key of a page which will contain this group
-- @field #number order Groups within the same page are sorted by this number, or their key for equal values.
--   Defaults to 0.
-- @field #boolean permanentStorage Whether the group should be stored in permanent storage, or in the save file
-- @field #list<#SettingOptions> settings A [iterables#List](iterables.html#List) table of #SettingOptions

---
-- Table of setting options
-- @type SettingOptions
-- @field #string key A unique key
-- @field #string name A key from the localization context
-- @field #string description A key from the localization context (optional, can be `nil`)
-- @field default A default value
-- @field #string renderer A renderer key (see the "Setting Renderers" page)
-- @field argument An argument for the renderer

return {
    interfaceName = 'Settings',
    ---
    -- @module Settings
    -- @usage
    -- -- In a player script
    -- local storage = require('openmw.storage')
    -- local I = require('openmw.interfaces')
    -- I.Settings.registerPage {
    --     key = 'MyModPage',
    --     l10n = 'MyMod',
    --     name = 'My Mod Name',
    --     description = 'My Mod Description',
    -- }
    -- I.Settings.registerGroup {
    --     key = 'SettingsPlayerMyMod',
    --     page = 'MyModPage',
    --     l10n = 'MyMod',
    --     name = 'My Group Name',
    --     description = 'My Group Description',
    --     permanentStorage = false,
    --     settings = {
    --         {
    --             key = 'Greeting',
    --             renderer = 'textLine',
    --             name = 'Greeting',
    --             description = 'Text to display when the game starts',
    --             default = 'Hello, world!',
    --         },
    --     },
    -- }
    -- local playerSettings = storage.playerSection('SettingsPlayerMyMod')
    -- ...
    -- ui.showMessage(playerSettings:get('Greeting'))
    -- -- ...
    -- -- access a setting page registered by a global script
    -- local globalSettings = storage.globalSection('SettingsGlobalMyMod')
    interface = {
        ---
        -- @field [parent=#Settings] #number version
        version = 1,
        ---
        -- @function [parent=#Settings] registerPage Register a page to be displayed in the settings menu,
        --   available in player and menu scripts
        -- @param #PageOptions options
        -- @usage
        -- I.Settings.registerPage({
        --   key = 'MyModName',
        --   l10n = 'MyModName',
        --   name = 'MyModName',
        --   description = 'MyModDescription',
        -- })---
        registerPage = registerPage,
        ---
        -- @function [parent=#Settings] registerRenderer Register a renderer,
        --   only available in menu scripts (DEPRECATED in player scripts)
        -- @param #string key
        -- @param #function renderer A renderer function, receives setting's value,
        --   a function to change it and an argument from the setting options
        -- @usage
        -- I.Settings.registerRenderer('text', function(value, set, arg)
        --   return {
        --     type = ui.TYPE.TextEdit,
        --     props = {
        --       size = util.vector2(arg and arg.size or 150, 30),
        --       text = value,
        --       textColor = util.color.rgb(1, 1, 1),
        --       textSize = 15,
        --       textAlignV = ui.ALIGNMENT.End,
        --     },
        --     events = {
        --       textChanged = async:callback(function(s) set(s) end),
        --     },
        --   }
        -- end)
        registerRenderer = function(name)
            print(([[Can't register setting renderer "%s". registerRenderer and moved to Menu context Settings interface]])
                :format(name))
        end,
        ---
        -- @function [parent=#Settings] registerGroup Register a group to be attached to a page,
        --   available in player, menu and global scripts
        --   Note: menu scripts only allow group with permanentStorage = true, but can render the page before a game is loaded!
        -- @param #GroupOptions options
        -- @usage
        -- I.Settings.registerGroup {
        --     key = 'SettingsTest',
        --     page = 'test',
        --     l10n = 'test',
        --     name = 'Player',
        --     description = 'Player settings group',
        --     permanentStorage = false,
        --     settings = {
        --         {
        --             key = 'Greeting',
        --             default = 'Hi',
        --             renderer = 'textLine',
        --             name = 'Text Input',
        --             description = 'Short text input',
        --         },
        --         {
        --             key = 'Flag',
        --             default = false,
        --             renderer = 'checkbox',
        --             name = 'Flag',
        --             description = 'Flag toggle',
        --         },
        --     }
        -- }
        registerGroup = common.registerGroup,
        ---
        -- @function [parent=#Settings] updateRendererArgument Change the renderer argument of a setting
        --   available both in player, menu and global scripts
        -- @param #string groupKey A settings group key
        -- @param #string settingKey A setting key
        -- @param argument A renderer argument
        updateRendererArgument = common.updateRendererArgument,
    },
    engineHandlers = {
        onLoad = common.onLoad,
        onSave = common.onSave,
    },
}
