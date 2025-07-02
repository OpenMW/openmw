Combat
======

.. include:: ../version.rst

Attack another actor.

**Arguments**

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
  * - target
    - `GameObject <../openmw_core.html##(GameObject)>`_ [required]
    - the actor to attack

**Examples**

.. code-block:: Lua

    -- from local script add package to self
    local AI = require('openmw.interfaces').AI
    AI.startPackage({type='Combat', target=anotherActor})

    -- via event to any actor
    actor:sendEvent('StartAIPackage', {type='Combat', target=anotherActor})
