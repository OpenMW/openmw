Pursue
======

.. include:: ../version.rst

Pursue another actor.

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
    - the actor to pursue
