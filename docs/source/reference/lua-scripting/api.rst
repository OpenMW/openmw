#################
Lua API reference
#################

.. toctree::
    :caption: Table of Contents
    :maxdepth: 2

Engine handlers reference
=========================

Engine handler is a function defined by a script, that can be called by the engine.

+------------------------------------------------------------------------------------------------+
| **Can be defined by any script**                                                               |
+----------------------------------+-------------------------------------------------------------+
| onUpdate(dt)                     | | Called every frame if game not paused. `dt` is the time   |
|                                  | | from the last update in seconds.                          |
+----------------------------------+-------------------------------------------------------------+
| onSave() -> data                 | | Called when the game is saving. May be called in inactive |
|                                  | | state, so it shouldn't use `openmw.nearby`.               |
+----------------------------------+-------------------------------------------------------------+
| onLoad(data)                     | | Called on loading with the data previosly returned by     |
|                                  | | onSave. During loading the object is always in inactive.  |
+----------------------------------+-------------------------------------------------------------+
| **Only for global scripts**                                                                    |
+----------------------------------+-------------------------------------------------------------+
| onNewGame()                      | New game is started                                         |
+----------------------------------+-------------------------------------------------------------+
| onPlayerAdded(player)            |Player added to game world. The argument is a `Game object`. |
+----------------------------------+-------------------------------------------------------------+
| onActorActive(actor)             | Actor (NPC or Creature) becomes active.                     |
+----------------------------------+-------------------------------------------------------------+
| **Only for local scripts**                                                                     |
+----------------------------------+-------------------------------------------------------------+
| onActive()                       | | Called when the object becomes active (either a player    |
|                                  | | came to this cell again, or a save was loaded).           |
+----------------------------------+-------------------------------------------------------------+
| onInactive()                     | | Object became inactive. Since it is inactive the handler  |
|                                  | | can not access anything nearby, but it is possible to send|
|                                  | | an event to global scripts.                               |
+----------------------------------+-------------------------------------------------------------+
| onConsume(recordId)              | | Called if `recordId` (e.g. a potion) is consumed.         |
+----------------------------------+-------------------------------------------------------------+
| **Only for local scripts attached to a player**                                                |
+----------------------------------+-------------------------------------------------------------+
| onKeyPress(symbol, modifiers)    | | Key pressed. `Symbol` is an ASCII code, `modifiers` is    |
|                                  | | a binary OR of flags of special keys (ctrl, shift, alt).  |
+----------------------------------+-------------------------------------------------------------+


Packages reference
==================

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
|`openmw.util <../../lua-api-reference/util.html>`_       | everywhere         | | Defines utility functions and classes like 3D vectors,      |
|                                                         |                    | | that don't depend on the game world.                        |
+---------------------------------------------------------+--------------------+---------------------------------------------------------------+
|`openmw.core <../../lua-api-reference/core.html>`_       | everywhere         | | Functions that are common for both global and local scripts |
+---------------------------------------------------------+--------------------+---------------------------------------------------------------+
|`openmw.async <../../lua-api-reference/async.html>`_     | everywhere         | | Timers (implemented) and coroutine utils (not implemented)  |
+---------------------------------------------------------+--------------------+---------------------------------------------------------------+
|`openmw.query <../../lua-api-reference/query.html>`_     | everywhere         | | Tools for constructing queries: base queries and fields.    |
+---------------------------------------------------------+--------------------+---------------------------------------------------------------+
|`openmw.world <../../lua-api-reference/world.html>`_     | by global scripts  | | Read-write access to the game world.                        |
+---------------------------------------------------------+--------------------+---------------------------------------------------------------+
|`openmw.self <../../lua-api-reference/self.html>`_       | by local scripts   | | Full access to the object the script is attached to.        |
+---------------------------------------------------------+--------------------+---------------------------------------------------------------+
|`openmw.nearby <../../lua-api-reference/nearby.html>`_   | by local scripts   | | Read-only access to the nearest area of the game world.     |
+---------------------------------------------------------+--------------------+---------------------------------------------------------------+
|`openmw.ui <../../lua-api-reference/ui.html>`_           | by player scripts  | | Controls user interface                                     |
+---------------------------------------------------------+--------------------+---------------------------------------------------------------+
|openmw.camera                                            | by player scripts  | | Controls camera (not implemented)                           |
+---------------------------------------------------------+--------------------+---------------------------------------------------------------+

