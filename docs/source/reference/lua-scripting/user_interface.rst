UI reference
============

.. include:: version.rst

Layouts
-------

Every widget is defined by a layout, which is a Lua table with the following fields (all of them are optional):

1. `type`: One of the available widget types from `openmw.ui.TYPE`.
2. | `props`: A Lua table, containing all the properties values.
   | Properties define most of the information about the widget: its position, data it displays, etc.
   | See the widget pages (table below) for details on specific properties.
   | Properties of the basic Widget are inherited by all the other widgets.
3. | `events`: A Lua table, containing `openmw.async.callback` values, which trigger on various interactions with the widget.
   | See the Widget pages for details on specific events.
   | Events of the basic Widget are inherited by all the other widgets.
4. `content`: a Content (`openmw.ui.content`), which contains layouts for the children of this widget.
5. | `name`: an arbitrary string, the only limitatiion is it being unique within a `Content`.
   | Helpful for navigatilng through the layouts.
6. `layer`: only applies for the root widget. (Windows, HUD, etc)
7. `template`: a Lua table which pre-defines a layout for this widget. See Templates below for more details.
8. `external`: similar to properties, but they affect how other widgets interact with this one. See the widget pages for details.

Layers
------
Layers control how widgets overlap - layers with higher indexes render over layers with lower indexes.
Widgets within the same layer which were added later overlap the ones created earlier.
A layer can also be set as non-interactive, which prevents all mouse interactions with the widgets in that layer.

.. TODO: Move this list when layers are de-hardcoded

Pre-defined OpenMW layers:

1. `HUD` interactive
2. `Windows` interactive
3. `Notification` non-interactive
4. `MessageBox` interactive

Elements
--------

Element is the root widget of a layout.
It is an independent part of the UI, connected only to a specific layer, but not any other layouts.
Creating or destroying an element also creates/destroys all of its children.

Content
-------

A container holding all the widget's children. It has a few important differences from a Lua table:

1. All the keys are integers, i. e. it is an "array"
2. Holes are not allowed. At any point all keys from `1` to the highest `n` must contain a value.
3. | You can access the values by their `name` field as a `Content` key.
   | While there is nothing preventing you from changing the `name` of a table inside a content, it is not supported, and will lead to undefined behaviour.
   | If you have to change the name, assign a new table to the index instead.

Templates
---------

Templates are Lua tables with the following (optional) fields:

1. `props`: Same as in layouts, defines the behaviour of this widget. Can be overwritten by `props` values in the layout.
2. | `content`: Extra children to add to the widget. For example, the frame and caption for Window widgets.
   | Contains normal layouts

Events
------

| A table mapping event names to `openmw.async.callback` s.
| When an event triggers, the callback is called with two arguments:
   an event-specific value, and that widget's layout table.
| See the Widget type pages for information on what events exist, and which first argument they pass. 

.. toctree::
   :maxdepth: 1
   :hidden:

   Widget <widgets/widget>
   Container <widgets/container>
   Flex <widgets/flex>
   Image <widgets/image>
   Text <widgets/text>
   TextEdit <widgets/textedit>

Example
-------

.. tab-set::

   .. tab-item:: scripts/clock.lua

      .. code-block:: lua

         local ui = require('openmw.ui')
         local util = require('openmw.util')
         local calendar = require('openmw_aux.calendar')
         local time = require('openmw_aux.time')

         local element = ui.create {
            -- important not to forget the layer
            -- by default widgets are not attached to any layer and are not visible
            layer = 'HUD',
            type = ui.TYPE.Text,
            props = {
            -- position in the top right corner
            relativePosition = util.vector2(1, 0),
            -- position is for the top left corner of the widget by default
            -- change it to align exactly to the top right corner of the screen
            anchor = util.vector2(1, 0),
            text = calendar.formatGameTime('%H:%M'),
            textSize = 24,
            -- default black text color isn't always visible
            textColor = util.color.rgb(0, 1, 0),
            },
         }

         local function updateTime()
            -- formatGameTime uses current time by default
            -- otherwise we could get it by calling `core.getGameTime()`
            element.layout.props.text = calendar.formatGameTime('%H:%M')
            -- the layout changes won't affect the widget unless we request an update
            element:update()
         end

         -- we are showing game time in hours and minutes
         -- so no need to update more often than once a game minute
         time.runRepeatedly(updateTime, 1 * time.minute, { type = time.GameTime })


   .. tab-item:: clock.omwscripts

      .. code-block::

         PLAYER: scripts/clock.lua

