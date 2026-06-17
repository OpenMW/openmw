Travel
======

.. include:: ../version.rst

Go to given location.

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
  * - destPosition
    - `3d vector <../openmw_util.html##(Vector3)>`_ [required]
    - the point to travel to
  * - isRepeat
    - boolean [optional]
    - Will the package repeat (true or false)
