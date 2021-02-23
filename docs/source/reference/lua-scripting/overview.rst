Overview of Lua scripting
#########################

Language and sandboxing
=======================

OpenMW supports scripts written in Lua 5.1.
There are no plans to switch to any newer version of the language, because newer versions are not supported by LuaJIT.

Here are starting points for learning Lua:

- `Programing in Lua <https://www.lua.org/pil/contents.html>`__ (first edition, aimed at Lua 5.0)
- `Lua 5.1 Reference Manual <https://www.lua.org/manual/5.1/>`__

Each script works in a separate sandbox and doesn't have any access to operation system.
Only limited list of allowed standard libraries can be used:
`coroutine <https://www.lua.org/manual/5.1/manual.html#5.2>`__,
`math <https://www.lua.org/manual/5.1/manual.html#5.6>`__,
`string <https://www.lua.org/manual/5.1/manual.html#5.4>`__,
`table <https://www.lua.org/manual/5.1/manual.html#5.5>`__.
These libraries are loaded automatically and are always available (except the function `math.randomseed` -- it is called by the engine on startup and not available from scripts).

Allowed `basic functions <https://www.lua.org/manual/5.1/manual.html#5.1>`__:
``assert``, ``error``, ``ipairs``, ``next``, ``pairs``, ``pcall``, ``print``, ``tonumber``, ``tostring``, ``type``, ``unpack``, ``xpcall``, ``rawequal``, ``rawget``, ``rawset``, ``setmetatable``.

Loading libraries with ``require('library_name')`` is allowed, but limited. It works this way:

1. If `library_name` is one of the standard libraries, then return the library.
2. If `library_name` is one of the built-in `API packages`_, then return the package.
3. Otherwise search for a Lua source file with such name in :ref:`data folders <Multiple data folders>`. For example ``require('my_lua_library.something')`` will try to open the file ``my_lua_library/something.lua``.

Loading DLLs and precompiled Lua files is intentionally prohibited for reasons of safety and compatibility between different platforms.

Basic concepts
==============

Game object
    Any object that exists in the game world and has a specific location. Player, actors, items, and statics are game objects.

Record
    Persistent information about an object. Includes starting stats and links to assets, but doesn't have a location. Game objects are instances of records. Some records (e.g. a unique NPC) have a single instance, some (e.g. a specific potion) may correspond to multiple objects.

.. note::
    Don't be confused with MWSE terminology. In MWSE game objects are "references" and records are "objects".

Cell
    An area of the game world. A position in the world is a link to a cell and coordinates X, Y, Z in the cell. At a specific moment in time each cell can be active or inactive. Inactive cells don't perform physics updates.

Global scripts
    Lua scripts that are not attached to any game object and are always active. Global scripts can not be started or stopped during a game session. List of global scripts is defined by `omwscripts` files, that should be :ref:`registered <Lua scripting>` in `openmw.cfg`.

Local scripts
    Lua scripts that are attached to some game object. A local script is active only if the object it is attached to is in an active cell. There are no limitations to the number of local scripts on one object. Local scripts can be attached to (or detached from) any object at any moment by a global script.

Player scripts
    It is a specific case of local scripts. *Player script* is a local script that is attached to a player. It can do everything that a normal local script can do, plus some player-specific functionality (e.g. control UI and camera).

Scripting API was developed to be conceptually compatible with `multiplayer <https://github.com/TES3MP/openmw-tes3mp>`__. In multiplayer the server is lightweight and delegates most of the work to clients. Each client processes some part of the game world. Global scripts are server-side and local scripts are client-side. It leads to several rules of Lua scripting API:

1. A local script can see only some area of the game world (cells that are active on a specific client). Any data from inactive cells can't be used, as they are not synchronized and could be already changed on another client.
2. A local script can modify only the object it is attached to. Other objects can theoretically be processed by another client. To prevent synchronization problems the access to them is read only.
3. Global scripts can access and modify the whole game world including unloaded areas, but the global scripts API is different from the local scripts API and in some aspects limited, because it is not always possible to have all game assets in memory at the same time.
4. Though the scripting system doesn't actually work with multiplayer yet, the API assumes that there can be several players. That's why any command related to UI, camera, and everything else that is player-specific can be used only by player scripts.


How to run a script
===================

Let's write a simple example of a `Player script`:

.. code-block:: Lua

    -- Saved to my_lua_mod/example/player.lua

    local ui = require('openmw.ui')

    return {
        engineHandlers = {
            onKeyPress = function(code, modifiers)
                if code == string.byte('x') then
                    ui.showMessage('You have pressed "X"')
                end
            end
        }
    }

In order to attach it to the player we also need a global script:

.. code-block:: Lua

    -- Saved to my_lua_mod/example/global.lua

    return {
        engineHandlers = {
            onPlayerAdded = function(player) player:addScript('example/player.lua') end
        }
    }

And one more file -- to start the global script:

::

    # Saved to my_lua_mod/my_lua_mod.omwscripts

    # It is just a list of global scripts to run. Each file is on a separate line.
    example/global.lua

Finally :ref:`register <Lua scripting>` it in ``openmw.cfg``:

::

    data=path/to/my_lua_mod
    lua-scripts=my_lua_mod.omwscripts

Now every time the player presses "X" on a keyboard, a message is shown.

Script structure
================

Each script is a separate file in game assets.
`Starting a script` means that the engine runs the file, parses the table it returns, and registers its interface, event handlers, and engine handlers. The handlers are permanent and exist until the script is stopped (if it is a local script, because global scripts can not be stopped).

Here is an example of a basic script structure:

.. code-block:: Lua

    local util = require('openmw.util')

    local function onUpdate(dt)
        ...
    end

    local function onSave()
        ...
        return data
    end

    local function onLoad(data)
        ...
    end

    local function myEventHandler(eventData)
        ...
    end

    local function somePublicFunction(params, ...)
        ...
    end

    return {
        name = 'MyScriptInterface',
        interface = {
            somePublicFunction = somePublicFunction,
        },

        eventHandlers = { MyEvent = myEventHandler },

        engineHandlers = {
            onUpdate = onUpdate,
            onSave = onSave,
            onLoad = onLoad,
        }
    }


.. note::
    Every instance of every script works in a separate enviroment, so it is not necessary
    to make everything local. It's local just because it makes the code a bit faster.

All sections in the returned table are optional.
If you just want to do something every frame, it is enough to write the following:

.. code-block:: Lua

    return {
        engineHandlers = {
            onUpdate = function()
                print('Hello, World!')
            end
        }
    }


Engine handlers
===============

An engine handler is a function defined by a script, that can be called by the engine. I.e. it is an engine-to-script interaction.
Not visible for other scripts. If several scripts register an engine handler with the same name,
the engine calls all of them in the same order as the scripts were started.

Some engine handlers are allowed only for global, or only for local/player scripts. Some are universal.
See :ref:`Engine handlers reference`.


onSave and onLoad
=================

When game is saved or loaded, the engine calls engine handlers `onSave` or `onLoad` for every script.
The value that `onSave` returns will be passed to `onLoad` when the game is loaded.
It is the only way to save internal state of a script. All other script vatiables will be lost after closing the game.
The saved state must be :ref:`serializable <Serializable data>`.

`onSave` and `onLoad` are special:

- Unlike all other engine handlers it is called even for objects in inactive cells.
- During saving and loading the environment may be not fully initialized, so these handlers shouldn't use any API calls.

TODO: example, explain how global scripts are loading

Serializable data
-----------------

`Serializable` value means that OpenMW is able to convert it to a sequence of bytes and then (probably on a different computer and with different OpenMW version) restore it back to the same form.

Serializable value is one of:

- `nil` value
- a number
- a string
- a game object
- a value of a type, defined by :ref:`openmw.util`
- a table whith serializable keys and values

Serializable data can not contain:

- Functions
- Tables with custom metatables
- Several references to the same table. For example ``{ x = some_table, y = some_table }`` is not allowed.
- Circular references (i.e. when some table contains itself).

API packages
============

API packages provide functions that can be called by scripts. I.e. it is a script-to-engine interaction.
A package can be loaded with ``require('<package name>')``.
It can not be overloaded even if there is a lua file with the same name.
The list of available packages is different for global and for local scripts.
Player scripts are local scripts that are attached to a player.

+----------------------+--------------------+---------------------------------------------------------------+
| Package              | Can be used        | Description                                                   |
+======================+====================+===============================================================+
|:ref:`openmw.util`    | everywhere         | | Defines utility functions and classes like 3D vectors,      |
|                      |                    | | that don't depend on the game world.                        |
+----------------------+--------------------+---------------------------------------------------------------+
|:ref:`openmw.core`    | everywhere         | | Functions that are common for both global and local scripts |
+----------------------+--------------------+---------------------------------------------------------------+
|:ref:`openmw.async`   | everywhere         | | Timers (implemented) and coroutine utils (not implemented)  |
+----------------------+--------------------+---------------------------------------------------------------+
|:ref:`openmw.query`   | everywhere         | **TODO: write description**                                   |
+----------------------+--------------------+---------------------------------------------------------------+
|:ref:`openmw.world`   | by global scripts  | | Read-write access to the game world.                        |
+----------------------+--------------------+---------------------------------------------------------------+
|:ref:`openmw.self`    | by local scripts   | | Full access to the object the script is attached to.        |
+----------------------+--------------------+---------------------------------------------------------------+
|:ref:`openmw.nearby`  | by local scripts   | | Read-only access to the nearest area of the game world.     |
+----------------------+--------------------+---------------------------------------------------------------+
|:ref:`openmw.ui`      | by player scripts  | | Controls user interface                                     |
+----------------------+--------------------+---------------------------------------------------------------+
|:ref:`openmw.camera`  | by player scripts  | | Controls camera (not implemented)                           |
+----------------------+--------------------+---------------------------------------------------------------+


Script interfaces
=================

.. warning::
    Not implemented yet.

Each script can provide a named interface for other scripts.
It is a script-to-script interaction. This mechanism is not used by the engine itself.

A script can use an interface of another script only if either both a global scripts, or both are local scripts on the same object.
In other cases events should be used.

TODO: example, overloading


Event system
============

It is another way of script-to-script interactions. The differences:

- Any script can send an event to any object or a global event to global scripts.
- Events are always delivered with a delay.
- Event handlers can not return any data to a sender.
- Event handlers have a single argument `eventData` (must be :ref:`serializable <Serializable data>`)

Events are the main way of interactions between local and global scripts.
It is not recommended to use for interactions between two global scripts, because in this case interfaces are more convenient.

If several scripts register handlers for the same event, it will be called in the reversed order (opposite to engine handlers).
I.e. handler from the last attached script will be called first.
Return value 'false' in a handler means "skip all other handlers for this event".
Any other return value (including nil) means nothing.

TODO: example


Timers
======

**TODO**


Queries
=======

**TODO:** describe the concepts of `openmw.query` and `world.selectObjects`/`nearby.selectObjects`.


Using IDE for Lua scripting
===========================

.. warning::
    This section is not written yet. Later it will explain how to setup Lua Development Tools (eclipse-based IDE) with code autocompletion and integrated OpenMW API reference.

