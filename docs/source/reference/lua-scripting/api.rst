#################
Lua API reference
#################

.. include:: version.rst

.. toctree::
    :hidden:

    engine_handlers
    user_interface
    aipackages
    setting_renderers
    events
    openmw_util
    openmw_storage
    openmw_core
    openmw_types
    openmw_animation
    openmw_async
    openmw_vfs
    openmw_world
    openmw_self
    openmw_nearby
    openmw_input
    openmw_ambient
    openmw_ui
    openmw_camera
    openmw_postprocessing
    openmw_debug
    openmw_aux_calendar
    openmw_aux_util
    openmw_aux_time
    openmw_aux_ui
    interface_activation
    interface_ai
    interface_animation
    interface_camera
    interface_controls
    interface_item_usage
    interface_mwui
    interface_settings
    interface_ui
    iterables


- :ref:`Engine handlers reference`
- :ref:`User interface reference <User interface reference>`
- `Game object reference <openmw_core.html##(GameObject)>`_
- `Cell reference <openmw_core.html##(Cell)>`_
- :ref:`Built-in AI packages`
- :ref:`Built-in events`

**API packages**

API packages provide functions that can be called by scripts. I.e. it is a script-to-engine interaction.
A package can be loaded with ``require('<package name>')``.
It can not be overloaded even if there is a lua file with the same name.
The list of available packages is different for global and for local scripts.
Player scripts are local scripts that are attached to a player.

.. include:: tables/packages.rst

**openmw_aux**

``openmw_aux.*`` are built-in libraries that are itself implemented in Lua. They can not do anything that is not possible with the basic API, they only make it more convenient.
Sources can be found in ``resources/vfs/openmw_aux``. In theory mods can override them, but it is not recommended.

.. include:: tables/aux_packages.rst

**Interfaces of built-in scripts**

.. include:: tables/interfaces.rst
