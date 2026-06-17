Follow
======

.. include:: ../version.rst

Follow another actor.

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
  * - destCell
    - Cell [optional]
    - the destination cell
  * - duration
    - number [optional]
    - duration in game time (will be rounded up to the next hour)
  * - destPosition
    - `3d vector <../openmw_util.html##(Vector3)>`_ [optional]
    - the destination point
  * - isRepeat
    - boolean [optional]
    - Will the package repeat (true or false)
