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
| onSave() -> data                 | Called when the game is saving.                             |
+----------------------------------+-------------------------------------------------------------+
| onLoad(data)                     | Called on loading with the data previosly returned by onSave|
+----------------------------------+-------------------------------------------------------------+
| **Only for global scripts**                                                                    |
+----------------------------------+-------------------------------------------------------------+
| onNewGame()                      | New game is started                                         |
+----------------------------------+-------------------------------------------------------------+
| onPlayerAdded(player)            |Player added to game world. The argument is a `Game object`_.|
+----------------------------------+-------------------------------------------------------------+
| onActorActive(actor)             | Actor (NPC or Creature) becomes active.                     |
+----------------------------------+-------------------------------------------------------------+
| **Only for local scripts attached to a player**                                                |
+----------------------------------+-------------------------------------------------------------+
| onKeyPress(symbol, modifiers)    | | Key pressed. `Symbol` is an ASCII code, `modifiers` is    |
|                                  | | a binary OR of flags of special keys (ctrl, shift, alt).  |
+----------------------------------+-------------------------------------------------------------+

.. _Game object:

Game object reference
=====================

Game object is a universal reference to an object in the game world. Anything that has a reference number.

**Can be used on any object:**

+---------------------------------------------+--------------------------------------------------+
| Function                                    | Description                                      |
+=============================================+==================================================+
| object:isValid()                            | | Returns true if the object exists and loaded,  |
|                                             | | and false otherwise. If false, then every      |
|                                             | | access to the object will raise an error.      |
+---------------------------------------------+--------------------------------------------------+
| object:sendEvent(eventName, eventData)      | Sends local event to the object.                 |
+---------------------------------------------+--------------------------------------------------+
| object:isEquipped(item)                     | Returns true if `item` is equipped on `object`.  |
+---------------------------------------------+--------------------------------------------------+
| object:getEquipment() -> table              | | Returns a table `slot` -> `object` of currently|
|                                             | | equipped items. See `core.EQUIPMENT_SLOT`.     |
|                                             | | Returns empty table if `object` doesn't have   |
|                                             | | equipment slots.                               |
+---------------------------------------------+--------------------------------------------------+
| object:setEquipment(table)                  | | Sets equipment. Keys in the table are equipment|
|                                             | | slots (see `core.EQUIPMENT_SLOT`). Each value  |
|                                             | | can be either an object or `recordId`. Raises  |
|                                             | | an error if `object` doesn't have equipment    |
|                                             | | slots and `table` is not empty. Can be called  |
|                                             | | only on `self` or from a global script.        |
+---------------------------------------------+--------------------------------------------------+
| object:addScript(scriptPath)                | | Adds new script to the object.                 |
|                                             | | Can be called only from a global script.       |
+---------------------------------------------+--------------------------------------------------+
| object:teleport(cell, pos, [rot])           | | Moves object to given cell and position.       |
|                                             | | The effect is not immediate: the position will |
|                                             | | be updated only in the next frame.             |
|                                             | | Can be called only from a global script.       |
+---------------------------------------------+--------------------------------------------------+

+-----------------------+---------------------+--------------------------------------------------+
| Field                 | Type                | Description                                      |
+=======================+=====================+==================================================+
| object.position       | vector3_            | Position                                         |
+-----------------------+---------------------+--------------------------------------------------+
| object.rotation       | vector3_            | Rotation                                         |
+-----------------------+---------------------+--------------------------------------------------+
| object.cell           | string              | Cell                                             |
+-----------------------+---------------------+--------------------------------------------------+
| object.type           | string              | :ref:`Type <Object type>` of the object          |
+-----------------------+---------------------+--------------------------------------------------+
| object.count          | integer             | Count (makes sense if holded in a container)     |
+-----------------------+---------------------+--------------------------------------------------+
| object.recordId       | string              | Record ID                                        |
+-----------------------+---------------------+--------------------------------------------------+
| object.inventory      | Inventory           | Inventory of an actor or content of a container  |
+-----------------------+---------------------+--------------------------------------------------+

**Can be used if object.type == 'Door':**

+-----------------------+---------------------+--------------------------------------------------+
| Field                 | Type                | Description                                      |
+=======================+=====================+==================================================+
| object.isTeleport     | boolean             | True if it is a teleport door                    |
+-----------------------+---------------------+--------------------------------------------------+
| object.destPosition   | vector3_            | Destination (only if a teleport door)            |
+-----------------------+---------------------+--------------------------------------------------+
| object.destRotation   | vector3_            | Destination rotation (only if a teleport door)   |
+-----------------------+---------------------+--------------------------------------------------+
| object.destCell       | string              | Destination cell (only if a teleport door)       |
+-----------------------+---------------------+--------------------------------------------------+

Object type
-----------

Type is represented as a string. Can be one of:

- "Activator"
- "Armor"
- "Book"
- "Clothing"
- "Container"
- "Creature"
- "Door"
- "Ingredient"
- "Light"
- "Miscellaneous"
- "NPC"
- "Player"
- "Potion"
- "Static"
- "Weapon"

ObjectList
----------

List of game objects. Can't be created or modified by a script.

.. code-block:: Lua

    -- Iteration by index
    for i = 1, #someList do
        doSomething(someList[i])
    end

    -- Generic for (equivalent to iteration by index)
    for i, item in someList:ipairs() do
        doSomething(item)
    end

    -- WRONG: for i, item in ipairs(someList) do
    -- It doesn't work because Lua 5.1 doesn't allow to overload ipairs for userdata.

+---------------------------------------------+--------------------------------------------------+
| Function                                    | Description                                      |
+=============================================+==================================================+
| list:ipairs()                               | Returns an iterator                              |
+---------------------------------------------+--------------------------------------------------+
| list:select(query) -> ObjectList            | Returns a filtered list                          |
+---------------------------------------------+--------------------------------------------------+

Object inventory
----------------

+---------------------------------------------+--------------------------------------------------+
| Function                                    | Description                                      |
+=============================================+==================================================+
| inv:countOf(recordId) -> int                | The number of items with given recordId          |
+---------------------------------------------+--------------------------------------------------+
| inv:getAll(recordId) -> ObjectList_         | All contained items                              |
+---------------------------------------------+--------------------------------------------------+
| inv:getPotions() -> ObjectList_             | All potions from the inventory                   |
+---------------------------------------------+--------------------------------------------------+
| inv:getApparatuses() -> ObjectList_         | All apparatuses from the inventory               |
+---------------------------------------------+--------------------------------------------------+
| inv:getArmors() -> ObjectList_              | All armors from the inventory                    |
+---------------------------------------------+--------------------------------------------------+
| inv:getBooks() -> ObjectList_               | All books from the inventory                     |
+---------------------------------------------+--------------------------------------------------+
| inv:getClothing() -> ObjectList_            | All clothing from the inventory                  |
+---------------------------------------------+--------------------------------------------------+
| inv:getIngredients() -> ObjectList_         | All ingredients from the inventory               |
+---------------------------------------------+--------------------------------------------------+
| inv:getLights() -> ObjectList_              | All lights from the inventory                    |
+---------------------------------------------+--------------------------------------------------+
| inv:getLockpicks() -> ObjectList_           | All lockpicks from the inventory                 |
+---------------------------------------------+--------------------------------------------------+
| inv:getMiscellaneous() -> ObjectList_       | All miscellaneous items from the inventory       |
+---------------------------------------------+--------------------------------------------------+
| inv:getProbes() -> ObjectList_              | All probes from the inventory                    |
+---------------------------------------------+--------------------------------------------------+
| inv:getRepairKits() -> ObjectList_          | All repair kits from the inventory               |
+---------------------------------------------+--------------------------------------------------+
| inv:getWeapons() -> ObjectList_             | All weapon from the inventory                    |
+---------------------------------------------+--------------------------------------------------+

openmw.util
===========

+---------------------------------------------+--------------------------------------------------+
| Function                                    | Description                                      |
+=============================================+==================================================+
| vector2(x, y) -> vector2_                   | Creates 2D vector                                |
+---------------------------------------------+--------------------------------------------------+
| vector3(x, y, z) -> vector3_                | Creates 3D vector                                |
+---------------------------------------------+--------------------------------------------------+
| clamp(value, from, to) -> number            | Limits given value to the interval [from, to]    |
+---------------------------------------------+--------------------------------------------------+
| normalizeAngle(radians) -> number           | Adds 2pi*k and puts the angle in range [-pi, pi] |
+---------------------------------------------+--------------------------------------------------+

vector2
-------

Immutable 2D vector.

.. code-block:: Lua

    v = vector2(3, 4)
    v.x, v.y       -- 3.0, 4.0
    str(v)         -- "(3.0, 4.0)"
    v:length()     -- 5.0    length
    v:length2()    -- 25.0   square of the length
    v:normalize()  -- vector2(3/5, 4/5)
    v:rotate(radians)    -- rotate clockwise (returns rotated vector)
    v1:dot(v2)     -- dot product (returns a number)
    v1 * v2        -- dot product
    v1 + v2        -- vector addition
    v1 - v2        -- vector subtraction
    v1 * x         -- multiplication by a number
    v1 / x         -- division by a number

vector3
-------

.. code-block:: Lua

    v = vector3(3, 4, 5)
    v.x, v.y, v.z  -- 3.0, 4.0, 5.0
    str(v)         -- "(3.0, 4.0, 4.5)"
    v:length()     -- length
    v:length2()    -- square of the length
    v:normalize()  -- normalized vector
    v1:dot(v2)     -- dot product (returns a number)
    v1 * v2        -- dot product (returns a number)
    v1:cross(v2)   -- cross product (returns a vector)
    v1 ^ v2        -- cross product (returns a vector)
    v1 + v2        -- vector addition
    v1 - v2        -- vector subtraction
    v1 * x         -- multiplication by a number
    v1 / x         -- division by a number

openmw.core
===========

+-----------------------+---------------------+---------------------------------------------------+
| Field                 | Type                | Description                                       |
+=======================+=====================+===================================================+
| OBJECT_TYPE           | Readonly table      | Possible :ref:`object type <Object type>` values  |
+-----------------------+---------------------+---------------------------------------------------+
| EQUIPMENT_SLOTS       | Readonly table      | | Contains keys that can be used in               |
|                       |                     | | `object:getEquipment` and `object:setEquipment`.|
+-----------------------+---------------------+---------------------------------------------------+

``EQUIPMENT_SLOTS`` contains fields: "Helmet", "Cuirass", "Greaves", "LeftPauldron", "RightPauldron",
"LeftGauntlet", "RightGauntlet", "Boots", "Shirt", "Pants", "Skirt", "Robe", "LeftRing", "RightRing",
"Amulet", "Belt", "CarriedRight", "CarriedLeft", "Ammunition".

+---------------------------------------------+--------------------------------------------------+
| Function                                    | Description                                      |
+=============================================+==================================================+
| sendGlobalEvent(eventName, eventData)       | Sends an event to global scripts                 |
+---------------------------------------------+--------------------------------------------------+
| getRealTime()                               | | The number of seconds (with floating point)    |
|                                             | | passed from 00:00 UTC 1 Januar 1970 (unix time)|
+---------------------------------------------+--------------------------------------------------+
| getGameTimeInSeconds()                      | | The number of seconds in the game world, passed|
|                                             | | from starting a new game.                      |
+---------------------------------------------+--------------------------------------------------+
| getGameTimeInHours()                        | | Current time of the game world in hours.       |
|                                             | | Note that the number of game seconds in a game |
|                                             | | hour is not guaranteed to be fixed.            |
+---------------------------------------------+--------------------------------------------------+

openmw.async
============

Timers and coroutine utils.
All functions require the package itself as a first argument.
I.e. functions should be called with ``:`` rather than with ``.``.

+-----------------------------------------------------+--------------------------------------------------+
| Function                                            | Description                                      |
+=====================================================+==================================================+
| async:registerTimerCallback(name, func) -> Callback | Registers a function as a timer callback         |
+-----------------------------------------------------+--------------------------------------------------+
| async:newTimerInSeconds(delay, Callback, arg)       | | Calls `Callback(arg)` in `delay` seconds.      |
|                                                     | | `Callback` must be registered in advance.      |
+-----------------------------------------------------+--------------------------------------------------+
| async:newTimerInHours(delay, Callback, arg)         | | Calls `Callback(arg)` in `delay` game hours.   |
|                                                     | | `Callback` must be registered in advance.      |
+-----------------------------------------------------+--------------------------------------------------+
| async:newUnsavableTimerInSeconds(delay, func)       | | Call `func()` in `delay` seconds. The timer    |
|                                                     | | will be lost if the game is saved and loaded.  |
+-----------------------------------------------------+--------------------------------------------------+
| async:newUnsavableTimerInHours(delay, func)         | | Call `func()` in `delay` game hours. The timer |
|                                                     | | will be lost if the game is saved and loaded.  |
+-----------------------------------------------------+--------------------------------------------------+

openmw.query
============

**TODO**

openmw.world
============

Interface to the game world. Can be used only by global scripts.

+-----------------------+---------------------+--------------------------------------------------+
| Field                 | Type                | Description                                      |
+=======================+=====================+==================================================+
| activeActors          | ObjectList_         | List of currently active actors                  |
+-----------------------+---------------------+--------------------------------------------------+

+---------------------------------------------+--------------------------------------------------+
| Function                                    | Description                                      |
+=============================================+==================================================+
| selectObjects(query) -> ObjectList_         | Evaluates a query                                |
+---------------------------------------------+--------------------------------------------------+

openmw.nearby
=============

Can be used only by local scripts.
Read-only access to the nearest area of the game world.

+-----------------------+---------------------+--------------------------------------------------+
| Field                 | Type                | Description                                      |
+=======================+=====================+==================================================+
| activators            | ObjectList_         | List of nearby activators                        |
+-----------------------+---------------------+--------------------------------------------------+
| actors                | ObjectList_         | List of nearby actors                            |
+-----------------------+---------------------+--------------------------------------------------+
| containers            | ObjectList_         | List of nearby containers                        |
+-----------------------+---------------------+--------------------------------------------------+
| doors                 | ObjectList_         | List of nearby doors                             |
+-----------------------+---------------------+--------------------------------------------------+
| items                 | ObjectList_         | Everything that can be picked up in the nearby   |
+-----------------------+---------------------+--------------------------------------------------+

+---------------------------------------------+--------------------------------------------------+
| Function                                    | Description                                      |
+=============================================+==================================================+
| selectObjects(query) -> ObjectList_         | Evaluates a query                                |
+---------------------------------------------+--------------------------------------------------+

openmw.self
===========

Can be used only by local scripts. Full access to the object the script is attached to.
All fields and function of `Game object`_ are also available for `openmw.self`. For example:

.. code-block:: Lua

    local self = require('openmw.self')
    if self.type == 'Player' then
        self:sendEvent("something", self.position)
    end

Note that `self` is not a Game Object. If you need an actual object, use `self.object`:

.. code-block:: Lua

    if self == someObject then ...  -- Incorrect, this condition is always false
    core.sendGlobalEvent('something', self)  -- Incorrect, will raise an error

    if self.object == someObject then ... -- Correct
    core.sendGlobalEvent('something', self.object)  -- Correct


+-----------------------+---------------------+--------------------------------------------------+
| Field                 | Type                | Description                                      |
+=======================+=====================+==================================================+
| self.object           | `Game object`_      | The object the script is attached to (readonly)  |
+-----------------------+---------------------+--------------------------------------------------+
| self.controls         | `Actor controls`_   | Movement controls (only for actors)              |
+-----------------------+---------------------+--------------------------------------------------+

+---------------------------------------------+--------------------------------------------------+
| Function                                    | Description                                      |
+=============================================+==================================================+
| self:setDirectControl(bool)                 | Enables or disables direct movement control      |
+---------------------------------------------+--------------------------------------------------+
| self:setEquipment(table)                    | | Sets equipment. Keys in the table are equipment|
|                                             | | slots (see `core.EQUIPMENT_SLOT`). Each value  |
|                                             | | can be either an object or `recordId`. Raises  |
|                                             | | an error if the object has no equipment        |
|                                             | | slots and `table` is not empty.                |
+---------------------------------------------+--------------------------------------------------+
| self:getCombatTarget() -> `Game object`_    | Returns current target or nil if not in combat   |
+---------------------------------------------+--------------------------------------------------+
| self:stopCombat()                           | Removes all combat packages from the actor       |
+---------------------------------------------+--------------------------------------------------+
| self:startCombat(target)                    | Attack `target`                                  |
+---------------------------------------------+--------------------------------------------------+


Actor controls
--------------

Allows to control movements of an actor. Makes an effect only if `setDirectControl(true)` was called.
All fields are mutable.

+-----------------------+---------------------+--------------------------------------------------+
| Field                 | Type                | Description                                      |
+=======================+=====================+==================================================+
| movement              | float number        | +1 - move forward, -1 - move backward            |
+-----------------------+---------------------+--------------------------------------------------+
| sideMovement          | float number        | +1 - move right, -1 - move left                  |
+-----------------------+---------------------+--------------------------------------------------+
| turn                  | float number        | turn right (radians); if negative - turn left    |
+-----------------------+---------------------+--------------------------------------------------+
| run                   | boolean             | true - run, false - walk                         |
+-----------------------+---------------------+--------------------------------------------------+
| jump                  | boolean             | if true - initiate a jump                        |
+-----------------------+---------------------+--------------------------------------------------+

openmw.ui
=========

Can be used only by local scripts, that are attached to a player.

+---------------------------------------------+--------------------------------------------------+
| Function                                    | Description                                      |
+=============================================+==================================================+
| showMessage(string)                         | Shows given message at the bottom of the screen. |
+---------------------------------------------+--------------------------------------------------+

openmw.camera
=============

.. warning::
    Not implemented yet.
