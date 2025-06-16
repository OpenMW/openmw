.. list-table::
   :widths: 30 40 60
   :header-rows: 1

   * - Package
     - Context
     - Description
   * - :doc:`core </reference/lua-scripting/openmw_core>`
     - |bdg-ctx-all|
     - Functions that are common for both global and local scripts
   * - :doc:`async </reference/lua-scripting/openmw_async>`
     - |bdg-ctx-all|
     - Timers and callbacks.
   * - :ref:`interfaces <Script interfaces>`
     - |bdg-ctx-all|
     - Public interfaces of other scripts.
   * - :doc:`markup </reference/lua-scripting/openmw_markup>`
     - |bdg-ctx-all|
     - API to work with markup languages.
   * - :doc:`storage </reference/lua-scripting/openmw_storage>`
     - |bdg-ctx-all|
     - Storage API. In particular can be used to store data between game sessions.
   * - :doc:`types </reference/lua-scripting/openmw_types>`
     - |bdg-ctx-all|
     - Functions for specific types of game objects.
   * - :doc:`util </reference/lua-scripting/openmw_util>`
     - |bdg-ctx-all|
     - Defines utility functions and classes like 3D vectors, that don't depend on the game world.
   * - :doc:`vfs </reference/lua-scripting/openmw_vfs>`
     - |bdg-ctx-all|
     - Read-only access to data directories via VFS.
   * - :doc:`world </reference/lua-scripting/openmw_world>`
     - |bdg-ctx-global|
     - Read-write access to the game world.
   * - :doc:`menu </reference/lua-scripting/openmw_menu>`
     - |bdg-ctx-menu|
     - Main menu functionality, such as managing game saves
   * - :doc:`animation </reference/lua-scripting/openmw_animation>`
     - |bdg-ctx-local|
     - Animation controls
   * - :doc:`nearby </reference/lua-scripting/openmw_nearby>`
     - |bdg-ctx-local|
     - Read-only access to the nearest area of the game world.
   * - :doc:`self </reference/lua-scripting/openmw_self>`
     - |bdg-ctx-local|
     - Full access to the object the script is attached to.
   * - :doc:`camera </reference/lua-scripting/openmw_camera>`
     - |bdg-ctx-player|
     - Controls camera.
   * - :doc:`debug </reference/lua-scripting/openmw_debug>`
     - |bdg-ctx-player|
     - Collection of debug utils.
   * - :doc:`postprocessing </reference/lua-scripting/openmw_postprocessing>`
     - |bdg-ctx-player|
     - Controls post-process shaders.
   * - :doc:`ambient </reference/lua-scripting/openmw_ambient>`
     - |bdg-ctx-menu| |bdg-ctx-player|
     - Controls background sounds for given player.
   * - :doc:`input </reference/lua-scripting/openmw_input>`
     - |bdg-ctx-menu| |bdg-ctx-player|
     - User input.
   * - :doc:`ui </reference/lua-scripting/openmw_ui>`
     - |bdg-ctx-menu| |bdg-ctx-player|
     - Controls :ref:`user interface <UI reference>`.
