Interface MWUI
==============

.. include:: version.rst

.. raw:: html
   :file: generated_html/scripts_omw_mwui_init.html

Examples
--------
The following examples demonstrate how to use the MWUI interface to create Morrowind-themed UI elements.

Basic Window
^^^^^^^^^^^^
This example utilizes the ``boxTransparentThick`` template and thus uses the `Container <./widgets/container.html>`_ widget type as the base of the window. Because the container sizes itself to fit its contents, we'll provide a generic child widget (in the ``content`` table property) that has no additional content and simply exists to give the container something to size itself to.

The transparency of the black background will match the transparency setting that is set in your game settings.

.. tab-set::

   .. tab-item:: scripts/example-window.lua

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
                  props = {
                     size = util.vector2(300, 150)
                  }
               }
            })
         })


   .. tab-item:: example-window.omwscripts

      .. code-block::

         PLAYER: scripts/example-window.lua
   
   .. tab-item:: Result

      .. figure:: https://gitlab.com/nox7/openmw-docs/-/raw/ui-docs-images/docs/source/reference/lua-scripting/_static/mwui-transparent-box-example.webp
         :alt: Example of a window created using the boxTransparentThick template

Window With Text
^^^^^^^^^^^^^^^^
Similar to the above example, a window will be rendered with text all styled in Morrowind's UI style. We'll continue to use a blank child widget to force the window to size itself for this example.

.. tab-set::

   .. tab-item:: scripts/example-window.lua

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
                  props = {
                     size = util.vector2(300, 150)
                  },
                  content = ui.content({
                     {
                        type = ui.TYPE.Text,
                        template = mwui.templates.textNormal,
                        props = {
                           text = "Example text",
                           textSize = 18
                        }
                     }
                  })
               }
            })
         })


   .. tab-item:: example-window.omwscripts

      .. code-block::

         PLAYER: scripts/example-window.lua
   
   .. tab-item:: Result

      .. figure:: https://gitlab.com/nox7/openmw-docs/-/raw/ui-docs-images/docs/source/reference/lua-scripting/_static/mwui-box-with-text-example.png
         :alt: Example of a window and text created using the boxTransparentThick and textNormal templates