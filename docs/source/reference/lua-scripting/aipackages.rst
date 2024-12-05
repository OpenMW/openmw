Built-in AI packages
====================

.. include:: version.rst

Starting an AI package
----------------------

There are two ways to start AI package:

.. code-block:: Lua

    -- from local script add package to self
    local AI = require('openmw.interfaces').AI
    AI.startPackage(options)

    -- via event to any actor
    actor:sendEvent('StartAIPackage', options)

``options`` is Lua table with arguments of the AI package.

**Common arguments that can be used with any AI package**

.. list-table::
  :header-rows: 1
  :widths: 20 20 60

  * - name
    - type
    - description
  * - type
    - string [required]
    - the name of the package (see packages listed below)
  * - cancelOther
    - boolean [default=true]
    - whether to cancel all other AI packages

Combat
------

Attack another actor.

**Arguments**

.. list-table::
  :header-rows: 1
  :widths: 20 20 60

  * - name
    - type
    - description
  * - target
    - `GameObject <openmw_core.html##(GameObject)>`_ [required]
    - the actor to attack

**Examples**

.. code-block:: Lua

    -- from local script add package to self
    local AI = require('openmw.interfaces').AI
    AI.startPackage({type='Combat', target=anotherActor})

    -- via event to any actor
    actor:sendEvent('StartAIPackage', {type='Combat', target=anotherActor})

Pursue
------

Pursue another actor.

**Arguments**

.. list-table::
  :header-rows: 1
  :widths: 20 20 60

  * - name
    - type
    - description
  * - target
    - `GameObject <openmw_core.html##(GameObject)>`_ [required]
    - the actor to pursue

Follow
------

Follow another actor.

**Arguments**

.. list-table::
  :header-rows: 1
  :widths: 20 20 60

  * - name
    - type
    - description
  * - target
    - `GameObject <openmw_core.html##(GameObject)>`_ [required]
    - the actor to follow
  * - destCell
    - Cell [optional]
    - the destination cell
  * - duration
    - number [optional]
    - duration in game time (will be rounded up to the next hour)
  * - destPosition
    - `3d vector <openmw_util.html##(Vector3)>`_ [optional]
    - the destination point
  * - isRepeat
    - boolean [optional]
    - Will the package repeat (true or false)

Escort
------

Escort another actor to the given location.

**Arguments**

.. list-table::
  :header-rows: 1
  :widths: 20 20 60

  * - name
    - type
    - description
  * - target
    - `GameObject <openmw_core.html##(GameObject)>`_ [required]
    - the actor to follow
  * - destPosition
    - `3d vector <openmw_util.html##(Vector3)>`_ [required]
    - the destination point
  * - destCell
    - Cell [optional]
    - the destination cell
  * - duration
    - number [optional]
    - duration in game time (will be rounded up to the next hour)
  * - isRepeat
    - boolean [optional]
    - Will the package repeat (true or false)

**Example**

.. code-block:: Lua

    actor:sendEvent('StartAIPackage', {
        type = 'Escort',
        target = object.self,
        destPosition = util.vector3(x, y, z),
        duration = 3 * time.hour,
        isRepeat = true
    })

Wander
------

Wander nearby current position.

**Arguments**

.. list-table::
  :header-rows: 1
  :widths: 20 20 60

  * - name
    - type
    - description
  * - distance
    - float [default=0]
    - the actor to follow
  * - duration
    - number [optional]
    - duration in game time (will be rounded up to the next hour)
  * - idle
    - table [optional]
    - Idle chance values, up to 8
  * - isRepeat
    - boolean [optional]
    - Will the package repeat (true or false)

**Example**

.. code-block:: Lua

    local idleTable = {
        idle2 = 60,
        idle3 = 50,
        idle4 = 40,
        idle5 = 30,
        idle6 = 20,
        idle7 = 10,
        idle8 = 0,
        idle9 = 25
    }
    actor:sendEvent('StartAIPackage', {
        type = 'Wander',
        distance = 5000,
        duration = 5 * time.hour,
        idle = idleTable,
        isRepeat = true
    })

Travel
------

Go to given location.

**Arguments**

.. list-table::
  :header-rows: 1
  :widths: 20 20 60

  * - name
    - type
    - description
  * - destPosition
    - `3d vector <openmw_util.html##(Vector3)>`_ [required]
    - the point to travel to
  * - isRepeat
    - boolean [optional]
    - Will the package repeat (true or false)
