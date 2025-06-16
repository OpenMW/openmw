#################
Lua API reference
#################

.. include:: version.rst

.. toctree::
    :hidden:

    index_packages
    index_auxpackages
    index_aipackages
    index_interfaces
    UI <user_interface>
    setting_renderers
    Engine Handlers <engine_handlers>
    events
    Iterables <iterables>

**API packages**

API packages provide functions that can be called by scripts. I.e. it is a script-to-engine interaction.
A package can be loaded with ``require('<package name>')``.
It can not be overloaded even if there is a lua file with the same name.
The list of available packages is different for global and for local scripts.
Player scripts are local scripts that are attached to a player.

.. include:: tables/packages.rst

**Auxiliary packages**

``openmw_aux.*`` are built-in libraries that are itself implemented in Lua. They can not do anything that is not possible with the basic API, they only make it more convenient.
Sources can be found in ``resources/vfs/openmw_aux``. In theory mods can override them, but it is not recommended.

.. include:: tables/aux_packages.rst

**Interfaces of built-in scripts**

.. include:: tables/interfaces.rst
