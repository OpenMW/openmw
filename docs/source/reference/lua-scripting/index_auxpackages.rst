##################
Auxiliary Packages
##################

.. include:: version.rst

.. toctree::
    :hidden:

    calendar <openmw_aux_calendar>
    time <openmw_aux_time>
    ui <openmw_aux_ui>
    util <openmw_aux_util>


**Auxiliary packages**

``openmw_aux.*`` are built-in libraries that are itself implemented in Lua. They can not do anything that is not possible with the basic API, they only make it more convenient.
Sources can be found in ``resources/vfs/openmw_aux``. In theory mods can override them, but it is not recommended.

.. include:: tables/aux_packages.rst
