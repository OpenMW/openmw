Overview of Lua scripting
#########################

Language and sandboxing
=======================

OpenMW supports scripts written in Lua 5.1.
There are no plans to switch to any newer version of the language, because newer versions are not supported by LuaJIT.

Here are starting points for learning Lua:

- `Programing in Lua <https://www.lua.org/pil/contents.html>`__ (first edition, aimed at Lua 5.0)
- `Lua 5.1 Reference Manual <https://www.lua.org/manual/5.1/>`__

Each script works in a separate sandbox and doesn't have any access to the underlying operating system.
Only a limited list of allowed standard libraries can be used:
`coroutine <https://www.lua.org/manual/5.1/manual.html#5.2>`__,
`math <https://www.lua.org/manual/5.1/manual.html#5.6>`__,
`string <https://www.lua.org/manual/5.1/manual.html#5.4>`__,
`table <https://www.lua.org/manual/5.1/manual.html#5.5>`__.
These libraries are loaded automatically and are always available (except the function `math.randomseed` -- it is called by the engine on startup and not available from scripts).

Allowed `basic functions <https://www.lua.org/manual/5.1/manual.html#5.1>`__:
``assert``, ``error``, ``ipairs``, ``next``, ``pairs``, ``pcall``, ``print``, ``select``, ``tonumber``, ``tostring``, ``type``, ``unpack``, ``xpcall``, ``rawequal``, ``rawget``, ``rawset``, ``getmetatable``, ``setmetatable``.

Loading libraries with ``require('library_name')`` is allowed, but limited. It works this way:

1. If `library_name` is one of the standard libraries, then return the library.
2. If `library_name` is one of the built-in `API packages`_, then return the package.
3. Otherwise search for a Lua source file with such name in :ref:`data folders <Multiple data folders>`. For example ``require('my_lua_library.something')`` will try to open the file ``my_lua_library/something.lua``.

Loading DLLs and precompiled Lua files is intentionally prohibited for compatibility and security reasons.

Basic concepts
==============

Game object
    Any object that exists in the game world and has a specific location. Player, actors, items, and statics are game objects.

Record
    Persistent information about an object. Includes starting stats and links to assets, but doesn't have a location. Game objects are instances of records. Some records (e.g. a unique NPC) have a single instance, some (e.g. a specific potion) may correspond to multiple objects.

.. note::
    Don't be confused with MWSE terminology. In MWSE game objects are "references" and records are "objects".

Cell
    An area of the game world. A position in the world is a link to a cell and X, Y, Z coordinates in the cell. At a specific moment in time each cell can be active or inactive. Inactive cells don't perform physics updates.

Global scripts
    Lua scripts that are not attached to any game object and are always active. Global scripts can not be started or stopped during a game session. Lists of global scripts are defined by `omwscripts` files, which should be :ref:`registered <Lua scripting>` in `openmw.cfg`.

Local scripts
    Lua scripts that are attached to some game object. A local script is active only if the object it is attached to is in an active cell. There are no limitations to the number of local scripts on one object. Local scripts can be attached to (or detached from) any object at any moment by a global script. In some cases inactive local scripts still can run code (for example during saving and loading), but while inactive they can not see nearby objects.

.. note::
    Currently scripts on objects in a container or an inventory are considered inactive. Probably later this behaviour will be changed.

Player scripts
    A specific kind of local scripts; *player script* is a local script that is attached to a player. It can do everything that a normal local script can do, plus some player-specific functionality (e.g. control UI and camera).

This scripting API was developed to be conceptually compatible with `multiplayer <https://github.com/TES3MP/openmw-tes3mp>`__. In multiplayer the server is lightweight and delegates most of the work to clients. Each client processes some part of the game world. Global scripts are server-side and local scripts are client-side. Because of this, there are several rules for the Lua scripting API:

1. A local script can see only some area of the game world (cells that are active on a specific client). Any data from inactive cells can't be used, as they are not synchronized and could be already changed on another client.
2. A local script can only modify the object it is attached to. Other objects can theoretically be processed by another client. To prevent synchronization problems the access to them is read-only.
3. Global scripts can access and modify the whole game world including unloaded areas, but the global scripts API is different from the local scripts API and in some aspects limited, because it is not always possible to have all game assets in memory at the same time.
4. Though the scripting system doesn't actually work with multiplayer yet, the API assumes that there can be several players. That's why any command related to UI, camera, and everything else that is player-specific can be used only by player scripts.


How to run a script
===================

Let's write a simple example of a `Player script`:

.. code-block:: Lua

    -- Save to my_lua_mod/example/player.lua

    local ui = require('openmw.ui')

    return {
        engineHandlers = {
            onKeyPress = function(key)
                if key.symbol == 'x' then
                    ui.showMessage('You have pressed "X"')
                end
            end
        }
    }

The script will be used only if it is specified in one of content files.
OpenMW Lua is an inclusive OpenMW feature, so it can not be controlled by ESP/ESM.
The options are:


1. Create text file "my_lua_mod.omwscripts" with the following line:
::

    PLAYER: example/player.lua

2. (not implemented yet) Add the script in OpenMW CS on "Lua scripts" view and save as "my_lua_mod.omwaddon".


Enable it in ``openmw.cfg`` the same way as any other mod:

::

    data=path/to/my_lua_mod
    content=my_lua_mod.omwscripts  # or content=my_lua_mod.omwaddon

Now every time the player presses "X" on a keyboard, a message is shown.


Format of ``.omwscripts``
=========================

::

    # Lines starting with '#' are comments

    GLOBAL: my_mod/some_global_script.lua

    # Script that will be automatically attached to the player
    PLAYER: my_mod/player.lua

    # Local script that will be automatically attached to every NPC and every creature in the game
    NPC, CREATURE: my_mod/some_other_script.lua

    # Local script that can be attached to any object by a global script
    CUSTOM: my_mod/something.lua

    # Local script that will be automatically attached to any Container AND can be
    # attached to any other object by a global script.
    CONTAINER, CUSTOM: my_mod/container.lua

Each script is described by one line:
``<flags>: <path to .lua file in virtual file system>``.
The order of lines determines the script load order (i.e. script priorities).

Possible flags are:

- ``GLOBAL`` - a global script; always active, can not by stopped;
- ``CUSTOM`` - dynamic local script that can be started or stopped by a global script;
- ``PLAYER`` - an auto started player script;
- ``ACTIVATOR`` - a local script that will be automatically attached to any activator;
- ``ARMOR`` - a local script that will be automatically attached to any armor;
- ``BOOK`` - a local script that will be automatically attached to any book;
- ``CLOTHING`` - a local script that will be automatically attached to any clothing;
- ``CONTAINER`` - a local script that will be automatically attached to any container;
- ``CREATURE`` - a local script that will be automatically attached to any creature;
- ``DOOR`` - a local script that will be automatically attached to any door;
- ``INGREDIENT`` - a local script that will be automatically attached to any ingredient;
- ``LIGHT`` - a local script that will be automatically attached to any light;
- ``MISC_ITEM`` - a local script that will be automatically attached to any miscellaneous item;
- ``NPC`` - a local script that will be automatically attached to any NPC;
- ``POTION`` - a local script that will be automatically attached to any potion;
- ``WEAPON`` - a local script that will be automatically attached to any weapon.

Several flags (except ``GLOBAL``) can be used with a single script. Use space or comma as a separator.

Hot reloading
=============

It is possible to modify a script without restarting OpenMW. To apply changes, open the in-game console and run the command: ``reloadlua``.
This will restart all Lua scripts using the `onSave and onLoad`_ handlers the same way as if the game was saved or loaded.
It reloads all ``.omwscripts`` files and ``.lua`` files that are not packed to any archives. ``.omwaddon`` files and scripts packed to BSA can not be changed without restarting the game.

Script structure
================

Each script is a separate file in the game assets.
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
        interfaceName = 'MyScriptInterface',
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
Not visible to other scripts. If several scripts register an engine handler with the same name,
the engine calls all of them according to the load order (i.e. the order of ``content=`` entries in ``openmw.cfg``) and the order of scripts in ``omwaddon/omwscripts``.

Some engine handlers are allowed only for global, or only for local/player scripts. Some are universal.
See :ref:`Engine handlers reference`.


onSave and onLoad
=================

When a game is saved or loaded, the engine calls the engine handlers `onSave` or `onLoad` for every script.
The value that `onSave` returns will be passed to `onLoad` when the game is loaded.
It is the only way to save the internal state of a script. All other script variables will be lost after closing the game.
The saved state must be :ref:`serializable <Serializable data>`.

`onSave` and `onLoad` can be called even for objects in inactive state, so it shouldn't use `openmw.nearby`.

An example:

.. code-block:: Lua

    ...

    local scriptVersion = 3  -- increase it every time when `onSave` is changed

    local function onSave()
        return {
            version = scriptVersion
            some = someVariable,
            someOther = someOtherVariable
        }
    end

    local function onLoad(data)
        if not data or not data.version or data.version < 2 then
            print('Was saved with an old version of the script, initializing to default')
            someVariable = 'some value'
            someOtherVariable = 42
            return
        end
        if data.version > scriptVersion then
            error('Required update to a new version of the script')
        end
        someVariable = data.some
        if data.version == scriptVersion then
            someOtherVariable = data.someOther
        else
            print(string.format('Updating from version %d to %d', data.version, scriptVersion))
            someOtherVariable = 42
        end
    end

    return {
        engineHandlers = {
            onUpdate = update,
            onSave = onSave,
            onLoad = onLoad,
        }
    }

Serializable data
-----------------

`Serializable` value means that OpenMW is able to convert it to a sequence of bytes and then (probably on a different computer and with different OpenMW version) restore it back to the same form.

Serializable value is one of:

- `nil` value
- a number
- a string
- a game object
- a value of a type, defined by :ref:`openmw.util <Package openmw.util>`
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

+---------------------------------------------------------+--------------------+---------------------------------------------------------------+
| Package                                                 | Can be used        | Description                                                   |
+=========================================================+====================+===============================================================+
|:ref:`openmw.interfaces <Script interfaces>`             | everywhere         | | Public interfaces of other scripts.                         |
+---------------------------------------------------------+--------------------+---------------------------------------------------------------+
|:ref:`openmw.util <Package openmw.util>`                 | everywhere         | | Defines utility functions and classes like 3D vectors,      |
|                                                         |                    | | that don't depend on the game world.                        |
+---------------------------------------------------------+--------------------+---------------------------------------------------------------+
|:ref:`openmw.settings <Package openmw.settings>`         | everywhere         | | Access to GMST records in content files (implemented) and   |
|                                                         |                    | | to mod settings (not implemented).                          |
+---------------------------------------------------------+--------------------+---------------------------------------------------------------+
|:ref:`openmw.core <Package openmw.core>`                 | everywhere         | | Functions that are common for both global and local scripts |
+---------------------------------------------------------+--------------------+---------------------------------------------------------------+
|:ref:`openmw.async <Package openmw.async>`               | everywhere         | | Timers (implemented) and coroutine utils (not implemented)  |
+---------------------------------------------------------+--------------------+---------------------------------------------------------------+
|:ref:`openmw.query <Package openmw.query>`               | everywhere         | | Tools for constructing queries: base queries and fields.    |
+---------------------------------------------------------+--------------------+---------------------------------------------------------------+
|:ref:`openmw.world <Package openmw.world>`               | by global scripts  | | Read-write access to the game world.                        |
+---------------------------------------------------------+--------------------+---------------------------------------------------------------+
|:ref:`openmw.self <Package openmw.self>`                 | by local scripts   | | Full access to the object the script is attached to.        |
+---------------------------------------------------------+--------------------+---------------------------------------------------------------+
|:ref:`openmw.nearby <Package openmw.nearby>`             | by local scripts   | | Read-only access to the nearest area of the game world.     |
+---------------------------------------------------------+--------------------+---------------------------------------------------------------+
|:ref:`openmw.input <Package openmw.input>`               | by player scripts  | | User input                                                  |
+---------------------------------------------------------+--------------------+---------------------------------------------------------------+
|:ref:`openmw.ui <Package openmw.ui>`                     | by player scripts  | | Controls user interface                                     |
+---------------------------------------------------------+--------------------+---------------------------------------------------------------+
|openmw.camera                                            | by player scripts  | | Controls camera (not implemented)                           |
+---------------------------------------------------------+--------------------+---------------------------------------------------------------+

openmw_aux
----------

``openmw_aux.*`` are built-in libraries that are themselves implemented in Lua. They can not do anything that is not possible with the basic API, they only make it more convenient.
Sources can be found in ``resources/vfs/openmw_aux``. In theory mods can override them, but it is not recommended.

+---------------------------------------------------------+--------------------+---------------------------------------------------------------+
| Built-in library                                        | Can be used        | Description                                                   |
+=========================================================+====================+===============================================================+
|:ref:`openmw_aux.util <Package openmw_aux.util>`         | everywhere         | | Miscellaneous utils                                         |
+---------------------------------------------------------+--------------------+---------------------------------------------------------------+

They can be loaded with ``require`` the same as API packages. For example:

.. code-block:: Lua

    local aux_util = require('openmw_aux.util')
    aux_util.runEveryNSeconds(15, doSomething)  -- run `doSomething()` every 15 seconds


Script interfaces
=================

Each script can provide a named interface for other scripts.
It is a script-to-script interaction. This mechanism is not used by the engine itself.

A script can use an interface of another script either if both are global scripts, or both are local scripts on the same object.
In other cases events should be used.

Defining an interface:

.. code-block:: Lua

    return {
        interfaceName = "SomeUtils"
        interface = {
            version = 1,
            doSomething = function(x, y) ... end,
        }
    }

Overriding the interface and adding a debug output:

.. code-block:: Lua

    local baseInterface = nil  -- will be assigned by `onInterfaceOverride`
    interface = {
        version = 1,
        doSomething = function(x, y)
            print(string.format('SomeUtils.doSomething(%d, %d)', x, y))
            baseInterface.doSomething(x, y)  -- calls the original `doSomething`

            -- WRONG! Would lead to an infinite recursion.
            -- local interfaces = require('openmw.interfaces')
            -- interfaces.SomeUtils.doSomething(x, y)
        end,
    }

    return {
        interfaceName = "SomeUtils",
        interface = interface,
        engineHandlers = {
            onInterfaceOverride = function(base) baseInterface = base end,
        },
    }

A general recommendation about overriding is that the new interface should be fully compatible with the old one.
So it is fine to change the behaviour of `SomeUtils.doSomething`, but if you want to add a completely new function, it would be
better to create a new interface for it. For example `SomeUtilsExtended` with an additional function `doSomethingElse`.

Using the interface:

.. code-block:: Lua

    local interfaces = require('openmw.interfaces')

    local function onUpdate()
        interfaces.SomeUtils.doSomething(2, 3)
    end

    return { engineHandlers = {onUpdate = onUpdate} }

The order in which the scripts are started is important. So if one mod should override an interface provided by another mod, make sure that load order (i.e. the sequence of `lua-scripts=...` in `openmw.cfg`) is correct.


Event system
============

This is another kind of script-to-script interactions. The differences:

- Any script can send an event to any object or a global event to global scripts.
- Events are delivered with a small delay (in single player the delay is always one frame).
- Event handlers can not return any data to the sender.
- Event handlers have a single argument `eventData` (must be :ref:`serializable <Serializable data>`)

Events are the main way of interacting between local and global scripts.
They are not recommended for interactions between two global scripts, because in this case interfaces are more convenient.

If several scripts register handlers for the same event, the handlers will be called in reverse order (opposite to engine handlers).
I.e. the handler from the last script in the load order will be called first.
Return value 'false' means "skip all other handlers for this event".
Any other return value (including nil) means nothing.

An example. Imagine we are working on a mod that adds some "dark power" with special effects.
We attach a local script to an item that can explode.
At some moment it will send the 'DamagedByDarkPower' event to all nearby actors:

.. code-block:: Lua

    local self = require('openmw.self')
    local nearby = require('openmw.nearby')

    local function onActivate()
        for i, actor in nearby.actors:ipairs() do
            local dist = (self.position - actor.position):length()
            if dist < 500 then
                local damage = (1 - dist / 500) * 200
                actor:sendEvent('DamagedByDarkPower', {source=self.object, damage=damage})
            end
        end
    end

    return { engineHandlers = { ... } }

And every actor should have a local script that processes this event:

.. code-block:: Lua

    local function damagedByDarkPower(data)
        ...  -- apply `data.damage` to stats / run custom animation / etc
    end

    return {
        eventHandlers = { DamagedByDarkPower = damagedByDarkPower },
    }

Someone may create an additional mod that adds a protection from the dark power.
The protection mod attaches an additional local script to every actor. The script intercepts and modifies the event:

.. code-block:: Lua

    local protectionLevel = ...

    local function reduceDarkDamage(data)
        data.damage = data.damage - protectionLevel  -- reduce the damage
        return data.damage > 0  -- it skips the original handler if the damage becomes <= 0
    end

    return {
        eventHandlers = { DamagedByDarkPower = reduceDarkDamage },
    }

In order to be able to intercept the event, the protection script should be placed in the load order below the original script.


Timers
======

Timers are in the :ref:`openmw.async <Package openmw.async>` package.
They can be set either in game seconds or in game hours.

- `Game seconds`: the number of seconds in the game world (i.e. seconds when the game is not paused), passed from starting a new game.
- `Game hours`: current time of the game world in hours. The number of seconds in a game hour is not guaranteed to be fixed.

When the game is paused, all timers are paused as well.

When an object becomes inactive, timers on this object are not paused, but callbacks are called only when the object becomes active again.
For example if there were 3 timers with delays 30, 50, 90 seconds, and from the 15-th to the 65-th second the object was inactive, then the first two callbacks are both evaluated on the 65-th second and the third one -- on the 90-th second.

There are two types: *reliable* and *unsavable* timers.

Reliable timer
--------------

Reliable timers are automatically saved and restored when the game is saved or loaded.
When the game is saved each timer record contains only name of a callback, the time when the callback should be called, and an argument that should be passed to the callback.
The callback itself is not stored. That's why callbacks must be registered when the script is initialized with a function ``async:registerTimerCallback(name, func)``.
`Name` is an arbitrary string.

An example:

.. code-block:: Lua

    local async = require('openmw.async')

    local teleportWithDelayCallback = async:registerTimerCallback('teleport',
    function(data)
        data.actor:teleport(data.destCellName, data.destPos)
    end)

    local function teleportWithDelay(delay, actor, cellName, pos)
        async:newTimerInSeconds(delay, teleportWithDelayCallback, {
            actor = actor,
            destCellName = cellName,
            destPos = pos,
        })
    end

Unsavable timer
---------------

Unsavable timers can be created from any function without registering a callback in advance, but they can not be saved.
If the player saves the game when an unsavable timer is running, then the timer will be lost after reloading.
So be careful with unsavable timers and don't use them if there is a risk of leaving the game world in an inconsistent state.

An example:

.. code-block:: Lua

    local async = require('openmw.async')
    local ui = require('openmw.ui')

    return {
        engineHandlers = {
            onKeyPress = function(key)
                if key.symbol == 'x' then
                    async:newUnsavableTimerInSeconds(
                        10,
                        function()
                            ui.showMessage('You have pressed "X" 10 seconds ago')
                        end)
                end
            end,
        }
    }

Also in `openmw_aux`_ are the helper functions ``runEveryNSeconds`` and ``runEveryNHours``, they are implemented on top of unsavable timers:

.. code-block:: Lua

    local async = require('openmw.async')
    local core = require('openmw.core')

    -- call `doSomething()` at the end of every game day.
    -- `timeBeforeMidnight` is a delay before the first call. `24` is an interval.
    -- the periodical evaluation can be stopped at any moment by calling `stopFn()`
    local timeBeforeMidnight = 24 - math.fmod(core.getGameTimeInHours(), 24)
    local stopFn = aux_util.runEveryNHours(24, doSomething, timeBeforeMidnight)

    return {
        engineHandlers = {
            onLoad = function()
                -- the timer is unsavable, so we need to restart it in `onLoad`.
                timeBeforeMidnight = 24 - math.fmod(core.getGameTimeInHours(), 24)
                stopFn = aux_util.runEveryNHours(24, doSomething, timeBeforeMidnight)
            end,
        }
    }


Queries
=======

`openmw.query` contains base queries of each type (e.g. `query.doors`, `query.containers`...), which return all of the objects of given type in no particular order. You can then modify that query to filter the results, sort them, group them, etc. Queries are immutable, so any operations on them return a new copy, leaving the original unchanged.

`openmw.world.selectObjects` and `openmw.nearby.selectObjects` both accept a query and return objects that match it. However, `nearby.selectObjects` is only available in local scripts, and returns only objects from currently active cells, while `world.selectObjects` is only available in global scripts, and returns objects regardless of them being in active cells.
**TODO:** describe how to filter out inactive objects from world queries

An example of an object query:

.. code-block:: Lua

    local query = require('openmw.query')
    local nearby = require('openmw.nearby')
    local ui = require('openmw.ui')

    local function selectDoors(namePattern)
        local query = query.doors:where(query.DOOR.destCell.name:like(namePattern))
        return nearby.selectObjects(query)
    end

    local function showGuildDoors()
        ui.showMessage('Here are all the entrances to guilds!')
        for _, door in selectDoors("%Guild%"):ipairs() do
            local pos = door.position
            local message = string.format("%.0f;%.0f;%.0f", pos.x, pos.y, pos.z)
            ui.showMessage(message)
        end
    end

    return {
        engineHandlers = {
            onKeyPress = function(key)
                if key.symbol == 'e' then
                    showGuildDoors()
                end
            end
        }
    }

.. warning::
    The example above uses operation `like` that is not implemented yet.

**TODO:** add non-object queries, explain how relations work, and define what a field is

Queries are constructed through the following method calls: (if you've used SQL before, you will find them familiar)

- `:where(filter)` - filters the results to match the combination of conditions passed as the argument
- `:orderBy(field)` and `:orderByDesc(field)` sort the result by the `field` argument. Sorts in descending order in case of `:orderByDesc`. Multiple calls can be chained, with the first call having priority. (i. e. if the first field is equal, objects are sorted by the second one...) **(not implemented yet)**
- `:groupBy(field)` returns only one result for each value of the `field` argument. The choice of the result is arbitrary. Useful for counting only unique objects, or checking if certain objects exist. **(not implemented yet)**
- `:limit(number)` will only return `number` of results (or fewer)
- `:offset(number)` skips the first `number` results. Particularly useful in combination with `:limit` **(not implemented yet)**

Filters consist of conditions, which are combined with "and" (operator `*`), "or" (operator `+`), "not" (operator `-`) and braces `()`.

To make a condition, take a field from the `openmw.query` package and call any of the following methods:

- `:eq` equal to
- `:neq` not equal to
- `:gt` greater than
- `:gte` greater or equal to
- `:lt` less than
- `:lte` less or equal to
- `:like` matches a pattern. Only applicable to text (strings) **(not implemented yet)**

**TODO:** describe the pattern format

All the condition methods are type sensitive, and will throw an error if you pass a value of the wrong type into them.

A few examples of filters:

.. warning::
    `openmw.query.ACTOR` is not implemented yet

.. code-block:: Lua

    local query = require('openmw.query')
    local ACTOR = query.ACTOR

    local strong_guys_from_capital = (ACTOR.stats.level:gt(10) + ACTOR.stats.strength:gt(70))
        * ACTOR.cell.name:eq("Default city")

    -- could also write like this:
    local strong_guys = ACTOR.stats.level:gt(10) + ACTOR.stats.strength:gt(70)
    local guys_from_capital = ACTOR.cell.name:eq("Default city")
    local strong_guys_from_capital_2 = strong_guys * guys_from_capital

    local DOOR = query.DOOR

    local interestingDoors = -DOOR.name:eq("") * DOOR.isTeleport:eq(true) * Door.destCell.isExterior:eq(false)


Using IDE for Lua scripting
===========================

Find the directory ``resources/lua_api`` in your installation of OpenMW.
It describes OpenMW LuaAPI in
`LDT Documentation Language <https://wiki.eclipse.org/LDT/User_Area/Documentation_Language>`__.
It is the source from which the :ref:`API reference <Lua API reference>` is generated.

If you write scripts using `Lua Development Tools <https://www.eclipse.org/ldt/>`__ (eclipse-based IDE),
you can import these files to get code autocompletion and integrated OpenMW API reference. Here are the steps:

- Install and run `LDT <https://www.eclipse.org/ldt/#installation>`__.
- Press `File` / `New` / `Lua Project` in menu.

.. image:: https://gitlab.com/OpenMW/openmw-docs/raw/master/docs/source/reference/lua-scripting/_static/lua-ide-create-project.png

- Specify project name (for example the title of your omwaddon)
- Set `Targeted Execution Environment` to `No Execution Environment`, and `Target Grammar` to `lua-5.1`.

.. image:: https://gitlab.com/OpenMW/openmw-docs/raw/master/docs/source/reference/lua-scripting/_static/lua-ide-project-settings.png

- Press `Next`, choose the `Libraries` tab, and click `Add External Source Folder`.
- Specify there the path to ``resources/lua_api`` in your OpenMW installation.
- If you use `openmw_aux`_, add ``resources/vfs`` as an additional external source folder.

.. image:: https://gitlab.com/OpenMW/openmw-docs/raw/master/docs/source/reference/lua-scripting/_static/lua-ide-import-api.png

- Press `Finish`. Create a new Lua file.
- Now you have code completion! Press ``Ctrl+Space`` in any place to see the variants.

.. image:: https://gitlab.com/OpenMW/openmw-docs/raw/master/docs/source/reference/lua-scripting/_static/lua-ide-code-completion1.png

In some cases LDT can deduce types automatically, but it is not always possible.
You can add special hints to give LDT more information:

- Before function definition: ``--- @param TYPE argName``
- Before variable definition: ``--- @field TYPE variableName``

.. code-block:: Lua

    --- @param openmw.core#GameObject obj
    local function doSomething(obj)
        -- autocompletion now works with `obj`
    end

    --- @field openmw.util#Vector3 c
    local c

    -- autocompletion now works with `c`

.. image:: https://gitlab.com/OpenMW/openmw-docs/raw/master/docs/source/reference/lua-scripting/_static/lua-ide-code-completion2.png

See `LDT Documentation Language <https://wiki.eclipse.org/LDT/User_Area/Documentation_Language>`__ for more details.


