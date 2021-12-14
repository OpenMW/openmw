User interface reference
========================

.. toctree::
    :hidden:

    widgets/widget

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
6. `layer`: only applies for the root widget. 

Layers
------
Layers control how widgets overlap - layers with higher indexes cover render over layers with lower indexes.
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

.. TODO: Talk about skins/templates here when they are ready

Events
------

| A table mapping event names to `openmw.async.callback` s.
| When an event triggers, the callback is called with two arguments:
   an event-specific value, and that widget's layout table.
| See the Widget type pages for information on what events exist, and which first argument they pass. 

Widget types
------------

.. list-table::
   :widths: 30 70

   * - :ref:`Widget`
     - Base widget type, all the other widget inherit its properties and events.
   * - `Text`
     - Displays text.
   * - EditText
     - Accepts text input from the user.
   * - Window
     - Can be moved and resized by the user.

Example
-------

*scripts/requirePassword.lua*

.. code-block:: Lua

   local core = require('openmw.core')
   local async = require('openmw.async')
   local ui = require('openmw.ui')
   local v2 = require('openmw.util').vector2

   local layout = {
      layers = 'Windows',
      type = ui.TYPE.Window,
      skin = 'MW_Window', -- TODO: replace all skins here when they are properly implemented
      props = {
         size = v2(200, 250),
         -- put the window in the middle of the screen
         relativePosition = v2(0.5, 0.5),
         anchor = v2(0.5, 0.5),
      },
      content = ui.content {
         {
            type = ui.TYPE.Text,
            skin = 'SandText',
            props = {
               caption = 'Input password',
               relativePosition = v2(0.5, 0),
               anchor = v2(0.5, 0),
            },
         },
         {
            name = 'input',
            type = ui.TYPE.TextEdit,
            skin = "MW_TextEdit",
            props = {
               caption = '',
               relativePosition = v2(0.5, 0.5),
               anchor = v2(0.5, 0.5),
               size = v2(125, 50),
            },
            events = {}
         },
         {
            name = 'submit',
            type = ui.TYPE.Text, -- TODO: replace with button when implemented
            skin = "MW_Button",
            props = {
               caption = 'Submit',
               -- position at the bottom
               relativePosition = v2(0.5, 1.0),
               anchor = v2(0.5, 1.0),
               autoSize = false,
               size = v2(75, 50),
            },
            events = {},
         },
      },
   }

   local element = nil

   local input = layout.content.input
   -- TODO: replace with a better event when TextEdit is finished
   input.events.textInput = async:callback(function(text)
      input.props.caption = input.props.caption .. text
   end)

   local submit = layout.content.submit
   submit.events.mouseClick = async:callback(function()
      if input.props.caption == 'very secret password' then
         if element then
            element:destroy()
         end
      else
         print('wrong password', input.props.caption)
         core.quit()
      end
   end)

   element = ui.create(layout)

*requirePassword.omwscripts*

::

  PLAYER: scripts/requirePassword.lua
