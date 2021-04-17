Cells Settings
##############

preload enabled
---------------

:Type:		boolean
:Range:		True/False
:Default:	True

Controls whether textures and objects will be pre-loaded in background threads.
This setting being enabled should result in a reduced amount of loading screens, no impact on frame rate,
and a varying amount of additional RAM usage, depending on how the preloader was configured (see the below settings).
The default preloading settings with vanilla game files should only use negligible amounts of RAM, however,
when using high-res texture and model replacers
it may be necessary to tweak these settings to prevent the game from running out of memory.

The effects of (pre-)loading can be observed on the in-game statistics panel brought up with the 'F4' key.

All settings starting with 'preload' in this section will have no effect if preloading is disabled,
and can only be configured by editing the settings configuration file.


preload num threads
-------------------

:Type:		integer
:Range:		>=1
:Default:	1

Controls the number of worker threads used for preloading operations.
In addition to the preloading threads, OpenMW uses a main thread, a sound streaming thread, and a graphics thread.
Therefore, the default setting of one preloading thread will result in a total of 4 threads used,
which should work well with quad-core CPUs. If you have additional cores to spare,
consider increasing the number of preloading threads to 2 or 3 for a boost in preloading performance.
Faster preloading will reduce the chance that a cell could not be completely loaded before the player moves into it,
and hence reduce the chance of seeing loading screens or frame drops.
This may be especially relevant when the player moves at high speed
and/or a large number of objects are preloaded due to large viewing distance.

A value of 4 or higher is not recommended.
With 4 or more threads, improvements will start to diminish due to file reading and synchronization bottlenecks.

preload exterior grid
---------------------

:Type:		boolean
:Range:		True/False
:Default:	True

Controls whether adjacent cells are preloaded when the player moves close to an exterior cell border.

preload fast travel
-------------------

:Type:		boolean
:Range:		True/False
:Default:	False

Controls whether fast travel destinations are preloaded when the player moves close to a travel service.
Because the game can not predict the destination that the player will choose,
all possible destinations will be preloaded. This setting is disabled by default
due to the adverse effect on memory usage caused by the preloading of all possible destinations.

preload doors
-------------

:Type:		boolean
:Range:		True/False
:Default:	True

Controls whether locations behind a door are preloaded when the player moves close to the door.

preload distance
----------------

:Type:		floating point
:Range:		>0
:Default:	1000

Controls the distance in in-game units that is considered the player being 'close' to a preloading trigger.
Used by all the preloading mechanisms i.e. 'preload exterior grid', 'preload fast travel' and 'preload doors'.

For measurement purposes, the distance to an object in-game can be observed by opening the console,
clicking on the object and typing 'getdistance player'.

preload instances
-----------------

:Type:		boolean
:Range:		True/False
:Default:	True

Controls whether or not objects are also pre-instanced on top of being pre-loaded.
Instancing happens when the same object is placed more than once in the cell,
and to be sure that any modifications to one instance do not affect the other,
the game will create independent copies (instances) of the object.
If this setting is enabled, the creation of instances will be done in the preloading thread;
otherwise, instancing will only happen in the main thread once the cell is actually loaded.

Enabling this setting should reduce the chance of frame drops when transitioning into a preloaded cell,
but will also result in some additional memory usage.

preload cell cache min
----------------------

:Type:		integer
:Range:		>0
:Default:	12

The minimum number of preloaded cells that will be kept in the cache.
Once the number of preloaded cells in the cache exceeds this setting,
the game may start to expire preloaded cells based on the 'preload cell expiry delay' setting,
starting with the oldest cell.
When a preloaded cell expires, all the assets that were loaded for it will also expire
and will have to be loaded again the next time the cell is requested for preloading.

preload cell cache max
----------------------

:Type:		integer
:Range:		>0
:Default:	20

The maximum number of cells that will ever be in pre-loaded state simultaneously.
This setting is intended to put a cap on the amount of memory that could potentially be used by preload state.

preload cell expiry delay
-------------------------

:Type:		floating point
:Range:		>=0
:Default:	5

The amount of time (in seconds) that a preloaded cell will stay in cache after it is no longer referenced or required,
for example, after the player has moved away from a door without entering it.

prediction time
---------------

:Type:		floating point
:Range:		>=0
:Default:	1

The amount of time (in seconds) in the future to predict the player position for. 
This predicted position is used to preload any cells and/or distant terrain required at that position.

This setting will only have an effect if 'preload enabled' is set or the 'distant terrain' in the Terrain section is set.

Increasing this setting from its default may help if your computer/hard disk is too slow to preload in time and you see
loading screens and/or lag spikes.

cache expiry delay
------------------

:Type:		floating point
:Range:		>=0
:Default:	5

The amount of time (in seconds) that a preloaded texture or object will stay in cache
after it is no longer referenced or required, for example, when all cells containing this texture have been unloaded.

target framerate
----------------
:Type:          floating point
:Range:         >0
:Default:       60

Affects the time to be set aside each frame for graphics preloading operations.
The game will distribute the preloading over several frames so as to not go under the specified framerate. 
For best results, set this value to the monitor's refresh rate. If you still experience stutters on turning around, 
you can try a lower value, although the framerate during loading will suffer a bit in that case.

pointers cache size
-------------------

:Type:		integer
:Range:		40 to 1000
:Default:	40

The count of object pointers that will be saved for a faster search by object ID.
This is a temporary setting that can be used to mitigate scripting performance issues with certain game files. 
If your profiler (press F3 twice) displays a large overhead for the Scripting section, try increasing this setting. 
