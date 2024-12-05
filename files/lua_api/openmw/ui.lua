---
-- `openmw.ui` controls user interface.
-- Can be used only by menu scripts and local scripts, that are attached to a player.
-- @module ui
-- @usage
-- local ui = require('openmw.ui')

---
-- Widget types
-- @field [parent=#ui] #TYPE TYPE

---
-- Alignment values (details depend on the specific property). For horizontal alignment the order is left to right, for vertical alignment the order is top to bottom.
-- @type ALIGNMENT
-- @field Start
-- @field Center
-- @field End

---
-- Alignment values (left to right, top to bottom)
-- @field [parent=#ui] #ALIGNMENT ALIGNMENT

---
-- Tools for working with layers
-- @field [parent=#ui] #Layers layers

---
-- All available widget types
-- @type TYPE
-- @field Widget Base widget type
-- @field Text Display text
-- @field TextEdit Accepts user text input
-- @field Window Can be moved and resized by the user
-- @field Image Displays an image
-- @field Flex Aligns widgets in a row or column
-- @field Container Automatically wraps around its contents

---
-- Shows given message at the bottom of the screen.
-- @function [parent=#ui] showMessage
-- @param #string msg
-- @param #table options An optional table with additional optional arguments. Can contain:
--
--   * `showInDialogue` - If true, this message will only be shown in the dialogue window. If false, it will always be shown in a message box.
--                        When omitted, the message will be displayed in the dialogue window if it is open and will be shown at the bottom of the screen otherwise.
-- @usage local params = {
--    showInDialogue=false
-- };
-- ui.showMessage("Hello world", params)

---
-- Predefined colors for console output
-- @field [parent=#ui] #CONSOLE_COLOR CONSOLE_COLOR

---
-- Predefined colors for console output
-- @type CONSOLE_COLOR
-- @field openmw.util#Color Default
-- @field openmw.util#Color Error
-- @field openmw.util#Color Success
-- @field openmw.util#Color Info

---
-- Print to the in-game console.
-- @function [parent=#ui] printToConsole
-- @param #string msg
-- @param openmw.util#Color color

---
-- Set mode of the in-game console.
-- The mode can be any string, by default is empty.
-- If not empty, then the console doesn't handle mwscript commands and
-- instead passes user input to Lua scripts via `onConsoleCommand` engine handler.
-- @function [parent=#ui] setConsoleMode
-- @param #string mode

---
-- Set selected object for console.
-- @function [parent=#ui] setConsoleSelectedObject
-- @param openmw.core#GameObject obj

---
-- Returns the size of the OpenMW window in pixels as a 2D vector.
-- @function [parent=#ui] screenSize
-- @return openmw.util#Vector2

---
-- Converts a given table of tables into an @{openmw.ui#Content}
-- @function [parent=#ui] content
-- @param #table table
-- @return #Content

---
-- Creates a UI element from the given layout table
-- @function [parent=#ui] create
-- @param #Layout layout
-- @return #Element

---
-- Adds a settings page to main menu setting's Scripts tab.
-- @function [parent=#ui] registerSettingsPage
-- @param #SettingsPageOptions page

---
-- Removes the settings page
-- @function [parent=#ui] removeSettingsPage
-- @param #SettingsPageOptions page must be the exact same table of options as the one passed to registerSettingsPage

---
-- Table with settings page options, passed as an argument to ui.registerSettingsPage
-- @type SettingsPageOptions
-- @field #string name Name of the page, displayed in the list, used for search
-- @field #string searchHints Additional keywords used in search, not displayed anywhere
-- @field #Element element The page's UI, which will be attached to the settings tab. The root widget has to have a fixed size. Set the `size` field in `props`, `relativeSize` is ignored.

---
-- Update all existing UI elements. Potentially extremely slow, so only call this when necessary, e. g. after overriding a template.
-- @function [parent=#ui] updateAll

---
-- Layout
-- @type Layout
-- @field type Type of the widget, one of the values in #TYPE. Must match the type in #Template if both are present
-- @field #string layer Optional layout to display in. Only applies for the root widget.
--   Note: if the #Element isn't attached to anything, it won't be visible!
-- @field #string name Optional name of the layout. Allows access by name from Content
-- @field #table props Optional table of widget properties
-- @field #table events Optional table of event callbacks
-- @field #Content content Optional @{openmw.ui#Content} of children layouts
-- @field #Template template Optional #Template
-- @field #table external Optional table of external properties
-- @field userData Arbitrary data for you to use, e. g. when receiving the layout in an event callback

---
-- Template
-- @type Template
-- @field #table props
-- @field #Content content
-- @field type One of the values in #TYPE, serves as the default value for the #Layout

---
-- @type Layer
-- @field #string name Name of the layer
-- @field openmw.util#Vector2 size Size of the layer in pixels

---
-- Layers. Implements [iterables#List](iterables.html#List) of #Layer.
-- @type Layers
-- @list <#Layer>
-- @usage
-- ui.layers.insertAfter('HUD', 'NewLayer', { interactive = true })
-- local fourthLayer = ui.layers[4]
-- local windowsIndex = ui.layers.indexOf('Windows')
-- for i, layer in ipairs(ui.layers) do
--   print('layer', i, layer.name, layer.size)
-- end

---
-- Index of the layer with the given name. Returns nil if the layer doesn't exist
-- @function [parent=#Layers] indexOf
-- @param #string name Name of the layer
-- @return #number, #nil index

---
-- Creates a layer and inserts it after another layer (shifts indexes of some other layers).
-- @function [parent=#Layers] insertAfter
-- @param #string afterName Name of the layer after which the new layer will be inserted
-- @param #string name Name of the new layer
-- @param #table options Table with a boolean `interactive` field (default is true). Layers with interactive = false will ignore all mouse interactions.

---
-- Creates a layer and inserts it before another layer (shifts indexes of some other layers).
-- @function [parent=#Layers] insertBefore
-- @param #string beforeName Name of the layer before which the new layer will be inserted
-- @param #string name Name of the new layer
-- @param #table options Table with a boolean `interactive` field (default is true). Layers with interactive = false will ignore all mouse interactions.

---
-- Content. An array-like container, which allows to reference elements by their name.
-- Implements [iterables#List](iterables.html#List) of #Layout or #Element and [iterables#Map](iterables.html#Map) of #string to #Layout or #Element.
-- @type Content
-- @list <#any>
-- @usage
-- local content = ui.content {
--    { name = 'input' },
-- }
-- -- bad idea!
-- -- content[1].name = 'otherInput'
-- -- do this instead:
-- content.input = { name = 'otherInput' }
-- @usage
-- local content = ui.content {
--    { name = 'display' },
--    { name = 'submit' },
-- }
-- -- allowed, but shifts all the items after it "up" the array
-- content.display = nil
-- -- still no holes after this!
-- @usage
-- -- iterate over a Content
-- for i = 1, #content do
--    print('widget',content[i].name,'at',i)
-- end
-- @usage
-- -- Note: layout names can collide with method names. Because of that you can't use a layout name such as "insert":
-- local content = ui.content {
--     { name = 'insert '}
-- }
-- content.insert.content = ui.content {} -- fails here, content.insert is a function!

---
-- Content also acts as a map of names to Layouts
-- @function [parent=#Content] __index
-- @param self
-- @param #string name
-- @return #any

---
-- Puts the layout at given index by shifting all the elements after it
-- @function [parent=#Content] insert
-- @param self
-- @param #number index
-- @param #any layoutOrElement

---
-- Adds the layout at the end of the Content
-- (same as calling insert with `last index + 1`)
-- @function [parent=#Content] add
-- @param self
-- @param #any layoutOrElement

---
-- Finds the index of the given layout. If it is not in the container, returns nil
-- @function [parent=#Content] indexOf
-- @param self
-- @param #any layoutOrElement
-- @return #number, #nil index

---
-- Element. An element of the user interface
-- @type Element

---
-- Refreshes the rendered element to match the current layout state.
-- Refreshes positions and sizes, but not the layout of the child Elements.
-- @function [parent=#Element] update
-- @param self

-- @usage
-- local child = ui.create {
--     type = ui.TYPE.Text,
--     props = {
--         text = 'child 1',
--     },
-- }
-- local parent = ui.create {
--     content = ui.content {
--         child,
--         {
--             type = ui.TYPE.Text,
--             props = {
--                 text = 'parent 1',
--             },
--         }
--     }
-- }
-- -- ...
-- child.layout.props.text = 'child 2'
-- parent.layout.content[2].props.text = 'parent 2'
-- parent:update() -- will show 'parent 2', but 'child 1'


---
-- Destroys the element
-- @function [parent=#Element] destroy
-- @param self

---
-- Access or replace the element's layout
--   Note: Is reset to `nil` on `destroy`
-- @field [parent=#Element] #Layout layout

---
-- Mouse event, passed as an argument to relevant UI events
-- @type MouseEvent
-- @field openmw.util#Vector2 position Absolute position of the mouse cursor
-- @field openmw.util#Vector2 offset Position of the mouse cursor relative to the widget
-- @field #number button Mouse button which triggered the event.
--   Matches the arguments of @{openmw_input#input.isMouseButtonPressed} (`nil` for none, 1 for left, 3 for right).

---
-- Register a new texture resource. Can be used to manually atlas UI textures.
-- @function [parent=#ui] texture
-- @param #TextureResourceOptions options
-- @return #TextureResource
-- @usage
-- local ui = require('openmw.ui')
-- local vector2 = require('openmw.util').vector2
-- local myAtlas = 'textures/my_atlas.dds' -- a 128x128 atlas
-- local texture1 = ui.texture { -- texture in the top left corner of the atlas
--     path = myAtlas,
--     offset = vector2(0, 0),
--     size = vector2(64, 64),
-- }
-- local texture2 = ui.texture { -- texture in the top right corner of the atlas
--     path = myAtlas,
--     offset = vector2(64, 0),
--     size = vector2(64, 64),
-- }

---
-- A texture ready to be used by UI widgets
-- @type TextureResource

---
-- Table with arguments passed to ui.texture.
-- @type TextureResourceOptions
-- @field #string path Path to the texture file. Required
-- @field openmw.util#Vector2 offset Offset of this resource in the texture. (0, 0) by default
-- @field openmw.util#Vector2 size Size of the resource in the texture. (0, 0) by default. 0 means the whole texture size is used.

return nil
