Escort
======

.. include:: ../version.rst

Escort another actor to the given location.

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
    - the actor to follow
  * - destPosition
    - `3d vector <../openmw_util.html##(Vector3)>`_ [required]
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
