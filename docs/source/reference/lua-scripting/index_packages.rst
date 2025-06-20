########
Packages
########

.. include:: version.rst

.. toctree::
    :hidden:

    ambient <openmw_ambient>
    animation <openmw_animation>
    async <openmw_async>
    camera <openmw_camera>
    core <openmw_core>
    debug <openmw_debug>
    input <openmw_input>
    markup <openmw_markup>
    menu <openmw_menu>
    nearby <openmw_nearby>
    postprocessing <openmw_postprocessing>
    self <openmw_self>
    storage <openmw_storage>
    types <openmw_types>
    ui <openmw_ui>
    util <openmw_util>
    vfs <openmw_vfs>
    world <openmw_world>

**API packages**

API packages provide functions that can be called by scripts. I.e. it is a script-to-engine interaction.
A package can be loaded with ``require('<package name>')``.
It can not be overloaded even if there is a lua file with the same name.
The list of available packages is different for global and for local scripts.
Player scripts are local scripts that are attached to a player.

.. include:: tables/packages.rst
