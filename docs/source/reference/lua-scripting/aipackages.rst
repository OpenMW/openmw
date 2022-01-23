Built-in AI packages
====================

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

**Example**

.. code-block:: Lua

    actor:sendEvent('StartAIPackage', {
        type = 'Escort',
        target = object.self,
        destPosition = util.vector3(x, y, z),
        duration = 3 * time.hour,
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

