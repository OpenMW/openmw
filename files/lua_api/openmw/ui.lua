---
-- `openmw.ui` controls user interface.
-- Can be used only by local scripts, that are attached to a player.
-- @module ui
-- @usage
-- local ui = require('openmw.ui')

---
-- @field [parent=#ui] #WIDGET_TYPE WIDGET_TYPE

---
-- Tools for working with layers
-- @field [parent=#ui] #Layers layers

---
-- @type WIDGET_TYPE
-- @field [parent=#WIDGET_TYPE] Widget Base widget type
-- @field [parent=#WIDGET_TYPE] Text Display text
-- @field [parent=#WIDGET_TYPE] TextEdit Accepts user text input
-- @field [parent=#WIDGET_TYPE] Window Can be moved and resized by the user

---
-- Shows given message at the bottom of the screen.
-- @function [parent=#ui] showMessage
-- @param #string msg

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
-- Layout
-- @type Layout
-- @field #string name Optional name of the layout. Allows access by name from Content
-- @field #table props Optional table of widget properties
-- @field #table events Optional table of event callbacks
-- @field #Content content Optional @{openmw.ui#Content} of children layouts

---
-- Layers
-- @type Layers
-- @usage
-- ui.layers.insertAfter('HUD', 'NewLayer', { interactive = true })
-- local fourthLayerName = ui.layers[4]
-- local windowsIndex = ui.layers.indexOf('Windows')

---
-- Index of the layer with the givent name. Returns nil if the layer doesn't exist
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
-- Content. An array-like container, which allows to reference elements by their name
-- @type Content
-- @list <#Layout>
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

---
-- Puts the layout at given index by shifting all the elements after it
-- @function [parent=#Content] insert
-- @param self
-- @param #number index
-- @param #Layout layout

---
-- Adds the layout at the end of the Content
-- (same as calling insert with `last index + 1`)
-- @function [parent=#Content] add
-- @param self
-- @param #Layout layout

---
-- Finds the index of the given layout. If it is not in the container, returns nil
-- @function [parent=#Content] indexOf
-- @param self
-- @param #Layout layout
-- @return #number, #nil index

---
-- Element. An element of the user interface
-- @type Element

---
-- Refreshes the rendered element to match the current layout state
-- @function [parent=#Element] update
-- @param self

---
-- Destroys the element
-- @function [parent=#Element] destroy
-- @param self

---
-- Access or replace the element's layout
-- @field [parent=#Element] #Layout layout

---
-- Mouse event, passed as an argument to relevant UI events
-- @type MouseEvent
-- @field openmw.util#Vector2 position Absolute position of the mouse cursor
-- @field openmw.util#Vector2 offset Position of the mouse cursor relative to the widget
-- @field #number button Mouse button which triggered the event (could be nil)

return nil
