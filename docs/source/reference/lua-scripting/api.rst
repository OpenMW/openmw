#################
Lua API reference
#################

.. toctree::
    :hidden:

    engine_handlers
    user_interface
    aipackages
    events
    openmw_util
    openmw_storage
    openmw_core
    openmw_types
    openmw_async
    openmw_world
    openmw_self
    openmw_nearby
    openmw_input
    openmw_ui
    openmw_camera
    openmw_aux_calendar
    openmw_aux_util
    openmw_aux_time
    interface_ai
    interface_camera
    iterables


- :ref:`Engine handlers reference`
- :ref:`User interface reference <User interface reference>`
- `Game object reference <openmw_core.html##(GameObject)>`_
- `Cell reference <openmw_core.html##(Cell)>`_
- :ref:`Built-in AI packages`
- :ref:`Built-in events`

**API packages**

API packages provide functions that can be called by scripts. I.e. it is a script-to-engine interaction.
A package can be loaded with ``require('<package name>')``.
It can not be overloaded even if there is a lua file with the same name.
The list of available packages is different for global and for local scripts.
Player scripts are local scripts that are attached to a player.

+---------------------------------------------------------+--------------------+---------------------------------------------------------------+
| Package                                                 | Can be used        | Description                                                   |
+=========================================================+====================+===============================================================+
|:ref:`openmw.interfaces <Script interfaces>`             | everywhere         | | Public interfaces of other scripts.                         |
+---------------------------------------------------------+--------------------+---------------------------------------------------------------+
|:ref:`openmw.util <Package openmw.util>`                 | everywhere         | | Defines utility functions and classes like 3D vectors,      |
|                                                         |                    | | that don't depend on the game world.                        |
+---------------------------------------------------------+--------------------+---------------------------------------------------------------+
|:ref:`openmw.storage <Package openmw.storage>`           | everywhere         | | Storage API. In particular can be used to store data        |
|                                                         |                    | | between game sessions.                                      |
+---------------------------------------------------------+--------------------+---------------------------------------------------------------+
|:ref:`openmw.core <Package openmw.core>`                 | everywhere         | | Functions that are common for both global and local scripts |
+---------------------------------------------------------+--------------------+---------------------------------------------------------------+
|:ref:`openmw.types <Package openmw.types>`               | everywhere         | | Functions for specific types of game objects.               |
+---------------------------------------------------------+--------------------+---------------------------------------------------------------+
|:ref:`openmw.async <Package openmw.async>`               | everywhere         | | Timers (implemented) and coroutine utils (not implemented)  |
+---------------------------------------------------------+--------------------+---------------------------------------------------------------+
|:ref:`openmw.world <Package openmw.world>`               | by global scripts  | | Read-write access to the game world.                        |
+---------------------------------------------------------+--------------------+---------------------------------------------------------------+
|:ref:`openmw.self <Package openmw.self>`                 | by local scripts   | | Full access to the object the script is attached to.        |
+---------------------------------------------------------+--------------------+---------------------------------------------------------------+
|:ref:`openmw.nearby <Package openmw.nearby>`             | by local scripts   | | Read-only access to the nearest area of the game world.     |
+---------------------------------------------------------+--------------------+---------------------------------------------------------------+
|:ref:`openmw.input <Package openmw.input>`               | by player scripts  | | User input.                                                 |
+---------------------------------------------------------+--------------------+---------------------------------------------------------------+
|:ref:`openmw.ui <Package openmw.ui>`                     | by player scripts  | | Controls :ref:`user interface <User interface reference>`.  |
+---------------------------------------------------------+--------------------+---------------------------------------------------------------+
|:ref:`openmw.camera <Package openmw.camera>`             | by player scripts  | | Controls camera.                                            |
+---------------------------------------------------------+--------------------+---------------------------------------------------------------+

**openmw_aux**

``openmw_aux.*`` are built-in libraries that are itself implemented in Lua. They can not do anything that is not possible with the basic API, they only make it more convenient.
Sources can be found in ``resources/vfs/openmw_aux``. In theory mods can override them, but it is not recommended.

+---------------------------------------------------------+--------------------+---------------------------------------------------------------+
| Built-in library                                        | Can be used        | Description                                                   |
+=========================================================+====================+===============================================================+
|:ref:`openmw_aux.calendar <Package openmw_aux.calendar>` | everywhere         | | Game time calendar                                          |
+---------------------------------------------------------+--------------------+---------------------------------------------------------------+
|:ref:`openmw_aux.util <Package openmw_aux.util>`         | everywhere         | | Miscellaneous utils                                         |
+---------------------------------------------------------+--------------------+---------------------------------------------------------------+
|:ref:`openmw_aux.time <Package openmw_aux.time>`         | everywhere         | | Timers and game time utils                                  |
+---------------------------------------------------------+--------------------+---------------------------------------------------------------+

**Interfaces of built-in scripts**

+---------------------------------------------------------+--------------------+---------------------------------------------------------------+
| Interface                                               | Can be used        | Description                                                   |
+=========================================================+====================+===============================================================+
|:ref:`AI <Interface AI>`                                 | by local scripts   | | Control basic AI of NPCs and creatures.                     |
+---------------------------------------------------------+--------------------+---------------------------------------------------------------+
|:ref:`Camera <Interface Camera>`                         | by player scripts  | | Allows to alter behavior of the built-in camera script      |
|                                                         |                    | | without overriding the script completely.                   |
+---------------------------------------------------------+--------------------+---------------------------------------------------------------+

