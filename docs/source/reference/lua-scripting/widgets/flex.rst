Flex Widget
===========

Aligns its children along either a column or a row, depending on the `horizontal` property.

Properties
----------

.. list-table::
  :header-rows: 1
  :widths: 20 20 60

  * - name
    - type (default value)
    - description
  * - position
    - util.vector2 (0, 0)
    - | Offsets the position of the widget from its parent's
      | top-left corner in pixels.
  * - size
    - util.vector2 (0, 0)
    - Increases the widget's size in pixels.
  * - relativePosition  
    - util.vector2 (0, 0)
    - | Offsets the position of the widget from its parent's
      | top-left corner as a fraction of the parent's size.
  * - relativeSize
    - util.vector2 (0, 0)
    - Increases the widget's size by a fraction of its parent's size.
  * - anchor
    - util.vector2 (0, 0)
    - | Offsets the widget's position by a fraction of its size.
      | Useful for centering or aligning to a corner.
  * - visible
    - boolean (true)
    - Defines if the widget is visible
  * - propagateEvents
    - boolean (true)
    - Allows base widget events to propagate to the widget's parent.
  * - alpha
    - number (1.0)
    - | Set the opacity of the widget and its contents.
      | If `inheritAlpha` is set to `true`, this becomes the maximum alpha value the widget can take.
  * - inheritAlpha
    - boolean (true)
    - | Modulate `alpha` with parents `alpha`.
      | If the parent has `inheritAlpha` set to `true`, the value after modulating is passed to the child.
  * - horizontal
    - bool (false)
    - | Flex aligns its children in a row (main axis is horizontal) if true,
      | otherwise in a column (main axis is vertical).
  * - autoSize
    - bool (true)
    - | If true, Flex will automatically resize to fit its contents.
      | Children can't be relatively position/sized when true.
  * - align
    - ui.ALIGNMENT (Start)
    - Where to align the children in the main axis.
  * - arrange
    - ui.ALIGNMENT (Start)
    - How to arrange the children in the cross axis.
  * - gap
    - number (0)
    - The pixel gap between children.
  * - wrap
    - bool (false)
    - | If true, children will wrap to the next row (or column) when they exceed the main axis size.
      | Only applicable when `autoSize` is `false`.

Events
------

Base widget events are special, they can propagate up to the parent widget.
This can be prevented by changing the `propagateEvents` property, or by assigning an  event handler.
The event is still allowed to propagate if the event handler returns `true`.

.. list-table::
  :header-rows: 1
  :widths: 20 20 60

  * - name
    - first argument type
    - description
  * - keyPress
    - `KeyboardEvent <../openmw_input.html##(KeyboardEvent)>`_
    - A key was pressed with this widget in focus
  * - keyRelease
    - `KeyboardEvent <../openmw_input.html##(KeyboardEvent)>`_
    - A key was released with this widget in focus
  * - mouseMove
    - `MouseEvent <../openmw_ui.html##(MouseEvent)>`_
    - | Mouse cursor moved on this widget
      | `MouseEvent.button` is the mouse button being held
      | (nil when simply moving, and not dragging)
  * - mouseClick
    - nil
    - Widget was clicked with left mouse button
  * - mouseDoubleClick
    - nil
    - Widget was double clicked with left mouse button
  * - mousePress  
    - `MouseEvent <../openmw_ui.html##(MouseEvent)>`_
    - A mouse button was pressed on this widget
  * - mouseRelease  
    -  `MouseEvent <../openmw_ui.html##(MouseEvent)>`_
    - A mouse button was released on this widget
  * - focusGain
    - nil
    - Widget gained focus (either through mouse or keyboard)
  * - focusLoss
    - nil
    - Widget lost focus
  * - textInput
    - string
    - Text input with this widget in focus

External
--------
.. list-table::
  :header-rows: 1
  :widths: 20 20 60

  * - name
    - type (default value)
    - description
  * - grow
    - float (0)
    - | Grow factor for the child. If there is unused space in the Flex,
      | it will be split between widgets according to this value.
      | Has no effect if `autoSize` is `true`.
  * - stretch
    - float (0)
    - | Stretches the child to a percentage of the Flex's cross axis size.

Examples
--------

Horizontal Wrapping Flex with Gap
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
This example demonstrates creating a horizontal flex widget that incorporates wrap and gap usage. A basic white image texture is used for visual demonstration.

.. tab-set::

   .. tab-item:: scripts/flex-example.lua

      .. code-block:: lua

         local ui = require('openmw.ui')
         local util = require('openmw.util')
  
         ui.create({
          layer = "Windows",
          type = ui.TYPE.Flex,
          props = {
            autoSize = false, -- Wrapping doesn't work with autoSize
            horizontal = true,
            wrap = true,
            gap = 20,
            position = util.vector2(100, 100),
            size = util.vector2(220, 400),
          },
          content = ui.content({
            {
              type = ui.TYPE.Image,
              props = {
                resource = ui.texture({
                  path = "white"
                }),
                size = util.vector2(100, 100)
              }
            },
            {
              type = ui.TYPE.Image,
              props = {
                resource = ui.texture({
                  path = "white"
                }),
                size = util.vector2(100, 100)
              }
            },
            {
              type = ui.TYPE.Image,
              props = {
                resource = ui.texture({
                  path = "white"
                }),
                size = util.vector2(100, 100)
              }
            }
          })
        })

   .. tab-item:: flex-example.omwscripts

      .. code-block::

         PLAYER: scripts/flex-example.lua

   .. tab-item:: Result

      .. figure:: https://gitlab.com/nox7/openmw-docs/-/raw/ui-docs-images/docs/source/reference/lua-scripting/_static/flex-horizontal-wrap-gap.webp
         :alt: Example of a horizontal wrapping flex with gap

Vertical Flex with Arrangement and Stretch
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
This example demonstrates creating a vertical flex widget that incorporates arrange and stretch usage. A basic white image texture is used for visual demonstration. The arrangement prop will arrange the flex children on their cross axis. In the case of a vertical flex, arrange will be on the horizontal axis.

The second flex child has an external stretch property set. It is set to *1* so that it takes up 100% of the available space on that cross axis. Setting it to 0.5, for example, would size the width of that widget to be 50% of the available space on the cross axis.

.. tab-set::

   .. tab-item:: scripts/flex-example.lua

      .. code-block:: lua

        local ui = require('openmw.ui')
        local util = require('openmw.util')

        ui.create({
          layer = "Windows",
          type = ui.TYPE.Flex,
          props = {
            gap = 20,
            arrange = ui.ALIGNMENT.Center,
            position = util.vector2(100, 100),
            size = util.vector2(220, 400),
          },
          content = ui.content({
            {
              type = ui.TYPE.Image,
              props = {
                resource = ui.texture({
                  path = "white"
                }),
                size = util.vector2(50, 100)
              }
            },
            {
              type = ui.TYPE.Image,
              external = {
                stretch = 1
              },
              props = {
                resource = ui.texture({
                  path = "white"
                }),
                size = util.vector2(0, 25)
              }
            },
            {
              type = ui.TYPE.Image,
              props = {
                resource = ui.texture({
                  path = "white"
                }),
                size = util.vector2(50, 100)
              }
            }
          })
        })

   .. tab-item:: flex-example.omwscripts

      .. code-block::

         PLAYER: scripts/flex-example.lua

   .. tab-item:: Result

      .. figure:: https://gitlab.com/nox7/openmw-docs/-/raw/ui-docs-images/docs/source/reference/lua-scripting/_static/flex-vertical-arrange-stretch.webp
         :alt: Example of a vertical flex with arrangement and stretch

Flex With Grow Children
^^^^^^^^^^^^^^^^^^^^^^^
The example below will demonstrate using text with additional children to "grow" to fill up any remaining space on that flex axis track. Review the *Result* tab to see the visual outcome of using the grow property. Because it is the only child with grow set, it will take up all remaining space not used by the text widget.

This example uses the `MWUI Interface <./../interface_mwui.html>`_ to style widgets in Morrowind's UI style.

.. tab-set::

   .. tab-item:: scripts/flex-example.lua

      .. code-block:: lua

        local ui = require('openmw.ui')
        local util = require('openmw.util')
        local mwui = require('openmw.interfaces').MWUI

        ui.create({
          layer = "Windows",
          template = mwui.templates.boxTransparentThick,
          props = {
              position = util.vector2(100, 100)
          },
          content = ui.content({
              {
                type = ui.TYPE.Flex,
                props = {
                  gap = 20,
                  horizontal = true,
                  size = util.vector2(500, 150)
                },
                content = ui.content({
                  {
                    template = mwui.templates.textNormal,
                    type = ui.TYPE.Text,
                    props = {
                      textSize = 20,
                      text = "Hello, World!"
                    }
                  },
                  {
                    external = {
                      grow = 1
                    },
                    type = ui.TYPE.Image,
                    props = {
                      size = util.vector2(0, 20),
                      resource = ui.texture({ path = "white" })
                    }
                  }
                })
              }
          })
        })

   .. tab-item:: flex-example.omwscripts

      .. code-block::

         PLAYER: scripts/flex-example.lua

   .. tab-item:: Result

      .. figure:: https://gitlab.com/nox7/openmw-docs/-/raw/ui-docs-images/docs/source/reference/lua-scripting/_static/flex-grow-example-1.png
         :alt: Example of a horizontal flex with a growing child

To better understand how the external grow property works, another example is provided below with two white-image widgets both with grow set to "1". Because they are on the same track (there is only one track as *wrap* is not enabled), they will add up the total "grow" value (to be "2") and then their individual grow values are divided by that total to determine how much of the remaining space they will take. In this case, both widgets have the same grow value, so they will each take up 50% of the remaining space on that track.

.. tab-set::

   .. tab-item:: scripts/flex-example.lua

      .. code-block:: lua

        local ui = require('openmw.ui')
        local util = require('openmw.util')
        local mwui = require('openmw.interfaces').MWUI

        ui.create({
          layer = "Windows",
          template = mwui.templates.boxTransparentThick,
          props = {
              position = util.vector2(100, 100)
          },
          content = ui.content({
              {
                type = ui.TYPE.Flex,
                props = {
                  gap = 20,
                  horizontal = true,
                  size = util.vector2(500, 150)
                },
                content = ui.content({
                  {
                    template = mwui.templates.textNormal,
                    type = ui.TYPE.Text,
                    props = {
                      textSize = 20,
                      text = "Hello, World!"
                    }
                  },
                  {
                    external = {
                      grow = 1
                    },
                    type = ui.TYPE.Image,
                    props = {
                      size = util.vector2(0, 20),
                      resource = ui.texture({ path = "white" })
                    }
                  },
                  {
                    external = {
                      grow = 1
                    },
                    type = ui.TYPE.Image,
                    props = {
                      size = util.vector2(0, 20),
                      resource = ui.texture({ path = "white" })
                    }
                  }
                })
              }
          })
        })

   .. tab-item:: flex-example.omwscripts

      .. code-block::

         PLAYER: scripts/flex-example.lua

   .. tab-item:: Result

      .. figure:: https://gitlab.com/nox7/openmw-docs/-/raw/ui-docs-images/docs/source/reference/lua-scripting/_static/flex-grow-example-2.png
         :alt: Example of a horizontal flex with two growing children