+------------------------------------------------------------+--------------------+---------------------------------------------------------------+
| Package                                                    | Can be used        | Description                                                   |
+============================================================+====================+===============================================================+
|:ref:`openmw.interfaces <Script interfaces>`                | everywhere         | | Public interfaces of other scripts.                         |
+------------------------------------------------------------+--------------------+---------------------------------------------------------------+
|:ref:`openmw.util <Package openmw.util>`                    | everywhere         | | Defines utility functions and classes like 3D vectors,      |
|                                                            |                    | | that don't depend on the game world.                        |
+------------------------------------------------------------+--------------------+---------------------------------------------------------------+
|:ref:`openmw.storage <Package openmw.storage>`              | everywhere         | | Storage API. In particular can be used to store data        |
|                                                            |                    | | between game sessions.                                      |
+------------------------------------------------------------+--------------------+---------------------------------------------------------------+
|:ref:`openmw.core <Package openmw.core>`                    | everywhere         | | Functions that are common for both global and local scripts |
+------------------------------------------------------------+--------------------+---------------------------------------------------------------+
|:ref:`openmw.types <Package openmw.types>`                  | everywhere         | | Functions for specific types of game objects.               |
+------------------------------------------------------------+--------------------+---------------------------------------------------------------+
|:ref:`openmw.animation <Package openmw.animation>`          | everywhere         | | Animation controls                                          |
+------------------------------------------------------------+--------------------+---------------------------------------------------------------+
|:ref:`openmw.async <Package openmw.async>`                  | everywhere         | | Timers and callbacks.                                       |
+------------------------------------------------------------+--------------------+---------------------------------------------------------------+
|:ref:`openmw.vfs <Package openmw.vfs>`                      | everywhere         | | Read-only access to data directories via VFS.               |
+------------------------------------------------------------+--------------------+---------------------------------------------------------------+
|:ref:`openmw.markup <Package openmw.markup>`                | everywhere         | | API to work with markup languages.                          |
+------------------------------------------------------------+--------------------+---------------------------------------------------------------+
|:ref:`openmw.world <Package openmw.world>`                  | by global scripts  | | Read-write access to the game world.                        |
+------------------------------------------------------------+--------------------+---------------------------------------------------------------+
|:ref:`openmw.self <Package openmw.self>`                    | by local scripts   | | Full access to the object the script is attached to.        |
+------------------------------------------------------------+--------------------+---------------------------------------------------------------+
|:ref:`openmw.nearby <Package openmw.nearby>`                | by local scripts   | | Read-only access to the nearest area of the game world.     |
+------------------------------------------------------------+--------------------+---------------------------------------------------------------+
|:ref:`openmw.ambient <Package openmw.ambient>`              | by player scripts  | | Controls background sounds for given player.                |
+------------------------------------------------------------+--------------------+---------------------------------------------------------------+
|:ref:`openmw.input <Package openmw.input>`                  | by player scripts  | | User input.                                                 |
+------------------------------------------------------------+--------------------+---------------------------------------------------------------+
|:ref:`openmw.ui <Package openmw.ui>`                        | by player scripts  | | Controls :ref:`user interface <User interface reference>`.  |
+------------------------------------------------------------+--------------------+---------------------------------------------------------------+
|:ref:`openmw.menu <Package openmw.menu>`                    | by menu scripts    | | Main menu functionality, such as managing game saves        |
+------------------------------------------------------------+--------------------+---------------------------------------------------------------+
|:ref:`openmw.camera <Package openmw.camera>`                | by player scripts  | | Controls camera.                                            |
+------------------------------------------------------------+--------------------+---------------------------------------------------------------+
|:ref:`openmw.postprocessing <Package openmw.postprocessing>`| by player scripts  | | Controls post-process shaders.                              |
+------------------------------------------------------------+--------------------+---------------------------------------------------------------+
|:ref:`openmw.debug <Package openmw.debug>`                  | by player scripts  | | Collection of debug utils.                                  |
+------------------------------------------------------------+--------------------+---------------------------------------------------------------+
