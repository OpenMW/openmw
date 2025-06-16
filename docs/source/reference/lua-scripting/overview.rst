Overview of Lua scripting
#########################

.. include:: version.rst

Language and sandboxing
=======================

OpenMW supports scripts written in Lua 5.1 with some extensions (see below) from Lua 5.2 and Lua 5.3.
There are no plans to switch to any newer version of the language, because newer versions are not supported by LuaJIT.

.. note::
  There are also experimental declarations available for Teal, a typed dialect of Lua. see :ref:`Teal` for more details.

Here are some starting points for learning Lua:

- `Programing in Lua <https://www.lua.org/pil/contents.html>`__ (first edition, aimed at Lua 5.0)
- `Lua 5.1 Reference Manual <https://www.lua.org/manual/5.1/>`__

Each script works in a separate sandbox and doesn't have any access to the underlying operating system.
Only a limited list of allowed standard libraries can be used:
`coroutine <https://www.lua.org/manual/5.1/manual.html#5.2>`__,
`math <https://www.lua.org/manual/5.1/manual.html#5.6>`__ (except `math.randomseed` -- it is called by the engine on startup and not available from scripts),
`string <https://www.lua.org/manual/5.1/manual.html#5.4>`__,
`table <https://www.lua.org/manual/5.1/manual.html#5.5>`__,
`os <https://www.lua.org/manual/5.1/manual.html#5.8>`__ (only `os.date`, `os.difftime`, `os.time`).
These libraries are loaded automatically and are always available.

Allowed `basic functions <https://www.lua.org/manual/5.1/manual.html#5.1>`__:
``assert``, ``error``, ``ipairs``, ``next``, ``pairs``, ``pcall``, ``print``, ``select``, ``tonumber``, ``tostring``, ``type``, ``unpack``, ``xpcall``, ``rawequal``, ``rawget``, ``rawset``, ``getmetatable``, ``setmetatable``.

Supported Lua 5.2 features:

- ``goto`` and ``::labels::``;
- hex escapes ``\x3F`` and ``\*`` escape in strings;
- ``math.log(x [,base])``;
- ``string.rep(s, n [,sep])``;
- in ``string.format()``: ``%q`` is reversible, ``%s`` uses ``__tostring``, ``%a`` and ``%A`` are added;
- String matching pattern ``%g``;
- ``__pairs`` and ``__ipairs`` metamethods;
- Function ``table.unpack`` (alias to Lua 5.1 ``unpack``).

Supported Lua 5.3 features:

- All functions in the `UTF-8 Library <https://www.lua.org/manual/5.3/manual.html#6.5>`__

Loading libraries with ``require('library_name')`` is allowed, but limited. It works this way:

1. If `library_name` is one of the standard libraries, then return the library.
2. If `library_name` is one of the built-in `API packages`_, then return the package.
3. Otherwise search for a Lua source file with such name in :ref:`data folders <Multiple data folders>`. For example ``require('my_lua_library.something')`` will try to open one of the files ``my_lua_library/something.lua`` or ``my_lua_library/something/init.lua``.

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

Menu scripts
    Lua scripts that are ran regardless of a game being loaded. They can be used to add features to the main menu and manage save files.

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

    -- Save to my_lua_mod/scripts/example/player.lua

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

    PLAYER: scripts/example/player.lua

2. (not implemented yet) Add the script in OpenMW CS on "Lua scripts" view and save as "my_lua_mod.omwaddon".


Enable it in ``openmw.cfg`` the same way as any other mod:

::

    data=path/to/my_lua_mod
    content=my_lua_mod.omwscripts  # or content=my_lua_mod.omwaddon

Now every time the player presses "X" on a keyboard, a message is shown.


Lua scripts naming policy
=========================

Technically scripts can be placed anywhere in the virtual file system, but we recommend to follow the naming policy and choose one of:

- ``scripts/<ModName>/<ScriptName>.lua``: general case.
- ``scripts/<AuthorName>/<ModName>/<ScriptName>.lua``: if "ModName" is short and can potentially collide with other mods.
- ``scripts/<ModName>.lua``: if it is a simple mod that consists of a single script, the script can be placed in ``scripts/`` without subdirs.

``scripts/omw/`` is reserved for built-in scripts, don't use it in mods. Overriding built-in scripts is not recommended, prefer to adjust their behaviour via :ref:`Interfaces of built-in scripts` instead.

See also naming policy of :ref:`Localisation Files`.

Format of ``.omwscripts``
=========================

::

    # Lines starting with '#' are comments

    GLOBAL: scripts/my_mod/some_global_script.lua

    # Script that will be automatically attached to the player
    PLAYER: scripts/my_mod/player.lua

    # Local script that will be automatically attached to every NPC and every creature in the game
    NPC, CREATURE: scripts/my_mod/some_other_script.lua

    # Local script that can be attached to any object by a global script
    CUSTOM: scripts/my_mod/something.lua

    # Local script that will be automatically attached to any Container AND can be
    # attached to any other object by a global script.
    CONTAINER, CUSTOM: scripts/my_mod/container.lua

Each script is described by one line:
``<flags>: <path to .lua file in virtual file system>``.
The order of lines determines the script load order (i.e. script priorities).

Possible flags are:

- ``GLOBAL`` - a global script; always active, can not be stopped;
- ``MENU`` - a menu script; always active, even before a game is loaded
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
- ``WEAPON`` - a local script that will be automatically attached to any weapon;
- ``APPARATUS`` - a local script that will be automatically attached to any apparatus;
- ``LOCKPICK`` - a local script that will be automatically attached to any lockpick;
- ``PROBE`` - a local script that will be automatically attached to any probe tool;
- ``REPAIR`` - a local script that will be automatically attached to any repair tool.

Several flags (except ``GLOBAL``) can be used with a single script. Use space or comma as a separator.

Hot reloading
=============

It is possible to modify a script without restarting OpenMW. To apply changes, open the in-game console and run the command: ``reloadlua``.
This will restart all Lua scripts using the `onSave and onLoad`_ handlers the same way as if the game was saved or loaded.
It reloads all ``.omwscripts`` files and ``.lua`` files that are not packed to any archives. ``.omwaddon`` files and scripts packed to BSA can not be changed without restarting the game.

Lua console
===========

It is also possible to run Lua commands directly from the in-game console.

To enter the Lua mode run one of the commands:

- ``lua player`` or ``luap`` - enter player context
- ``lua global`` or ``luag`` - enter global context
- ``lua selected`` or ``luas`` - enter local context on the selected object
- ``lua menu`` or ``luam`` - enter menu context

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

When a game is saved or loaded, the engine calls the engine handlers `onSave` or `onLoad` to save or load each script.
The value that `onSave` returns will be passed to `onLoad` when the game is loaded.
It is the only way to save the internal state of a script. All other script variables will be lost after closing the game.
The saved state must be :ref:`serializable <Serializable data>`.

Note that `onLoad` means loading a script rather than loading a game.
If a script did not exist when a game was saved then `onLoad` will not be called, but `onInit` will.

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

.. include:: tables/packages.rst

Auxiliary packages
------------------

``openmw_aux.*`` are built-in libraries that are themselves implemented in Lua. They can not do anything that is not possible with the basic API, they only make it more convenient.
Sources can be found in ``resources/vfs/openmw_aux``. In theory mods can override them, but it is not recommended.

.. include:: tables/aux_packages.rst

They can be loaded with ``require`` the same as API packages. For example:

.. code-block:: Lua

    local time = require('openmw_aux.time')
    time.runRepeatedly(doSomething, 15 * time.second)  -- run `doSomething()` every 15 seconds


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

Interfaces of built-in scripts
------------------------------

.. include:: tables/interfaces.rst

Event system
============

This is another kind of script-to-script interactions. The differences:

- Any script can send an event to any object or a global event to global scripts.
- Events are delivered with a small delay (in single player the delay is always one frame).
- Event handlers can not return any data to the sender.
- Event handlers have a single argument `eventData` (must be :ref:`serializable <Serializable data>`)

There are a few methods for sending events:

- `core.sendGlobalEvent <openmw_core.html##(sendGlobalEvent)>`_ to send events to global scripts
- `GameObject:sendEvent <openmw_core.html##(GameObject).sendEvent>`_ to send events to local scripts attached to a game object
- `types.Player.sendMenuEvent <openmw_types.html##(Player).sendMenuEvent>`_ to send events to menu scripts of the given player

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

    local function onActivated()
        for i, actor in ipairs(nearby.actors) do
            local dist = (self.position - actor.position):length()
            if dist < 500 then
                local damage = (1 - dist / 500) * 200
                actor:sendEvent('DamagedByDarkPower', {source=self.object, damage=damage})
            end
        end
    end

    return { engineHandlers = { onActivated = onActivated } }

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

See :ref:`the list of events <Events>` that are used by built-in scripts.


Timers
======

Timers are in the :ref:`openmw.async <Package openmw.async>` package.
They can be set either in simulation time or in game time.

- `Simulation time`: the number of seconds in the game world (i.e. seconds when the game is not paused), passed from starting a new game.
- `Game time`: current time of the game world in seconds. Note that game time generally goes faster than the simulation time.

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
        async:newSimulationTimer(delay, teleportWithDelayCallback, {
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
                    async:newUnsavableSimulationTimer(
                        10,
                        function()
                            ui.showMessage('You have pressed "X" 10 seconds ago')
                        end)
                end
            end,
        }
    }

Also in `Auxiliary packages`_ is the helper function ``runRepeatedly``, it is implemented on top of unsavable timers:

.. code-block:: Lua

    local core = require('openmw.core')
    local time = require('openmw_aux.time')

    -- call `doSomething()` at the end of every game day.
    -- the second argument (`time.day`) is the interval.
    -- the periodical evaluation can be stopped at any moment by calling `stopFn()`
    local timeBeforeMidnight = time.day - core.getGameTime() % time.day
    local stopFn = time.runRepeatedly(doSomething, time.day, {
        initialDelay = timeBeforeMidnight,
        type = time.GameTime,
    })


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
- Specify there paths to ``resources/lua_api`` and ``resources/vfs`` in your OpenMW installation.

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

In order to have autocompletion for script interfaces the information where to find these interfaces should be provided.
For example for the camera interface (defined in ``resources/vfs/scripts/omw/camera.lua``):

.. code-block:: Lua

    --- @type Interfaces
    -- @field scripts.omw.camera#Interface Camera
    -- ... other interfaces here
    --- @field #Interfaces I
    local I = require('openmw.interfaces')

    I.Camera.disableZoom()

See `LDT Documentation Language <https://wiki.eclipse.org/LDT/User_Area/Documentation_Language>`__ for more details.
