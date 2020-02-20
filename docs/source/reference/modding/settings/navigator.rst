Navigator Settings
################

Main settings
*************

This section is for players.

enable
------

:Type:		boolean
:Range:		True/False
:Default:	True

Enable navigator.
When enabled background threads are started to build nav mesh for world geometry.
Pathfinding system uses nav mesh to build paths.
When disabled only pathgrid is used to build paths.
Single-core CPU systems may have big performance impact on exiting interior location and moving across exterior world.
May slightly affect performance on multi-core CPU systems.
Multi-core CPU systems may have different latency for nav mesh update depending on other settings and system performance.
Moving across external world, entering/exiting location produce nav mesh update.
NPC and creatures may not be able to find path before nav mesh is built around them.
Try to disable this if you want to have old fashioned AI which doesn't know where to go when you stand behind that stone and casting a firebolt.

max tiles number
----------------

:Type:		integer
:Range:		>= 0
:Default:	512

Number of tiles at nav mesh.
Nav mesh covers circle area around player.
This option allows to set an explicit limit for nav mesh size, how many tiles should fit into circle.
If actor is inside this area it able to find path over nav mesh.
Increasing this value may decrease performance.

.. note::
    Don't expect infinite nav mesh size increasing.
    This condition is always true: ``max tiles number * max polygons per tile <= 4194304``.
    It's a limitation of `Recastnavigation <https://github.com/recastnavigation/recastnavigation>`_ library.

Advanced settings
*****************

This section is for advanced PC uses who understands concepts of OS thread and memory.

async nav mesh updater threads
------------------------------

:Type:		integer
:Range:		>= 1
:Default:	1

Number of background threads to update nav mesh.
Increasing this value may decrease performance, but also may decrease or increase nav mesh update latency depending on number of CPU cores.
On systems with not less than 4 CPU cores latency dependens approximately like 1/log(n) from number of threads.
Don't expect twice better latency by doubling this value.

max nav mesh tiles cache size
-----------------------------

:Type:		integer
:Range:		>= 0
:Default:	268435456

Maximum total cached size of all nav mesh tiles in bytes.
Setting greater than zero value will reduce nav mesh update latency for previously visited locations.
Increasing this value may increase total memory consumption, but potentially will reduce latency for recently visited locations.
Limit this value by total available physical memory minus base game memory consumption and other applications.
Game will not eat all memory at once.
Memory will be consumed in approximately linear dependency from number of nav mesh updates.
But only for new locations or already dropped from cache.

min update interval ms
----------------

:Type:		integer
:Range:		>= 0
:Default:	250

Minimum time duration required to pass before next navmesh update for the same tile in milliseconds.
Only tiles affected where objects are transformed.
Next update for tile with added or removed object will not be delayed.
Visible ingame effect is navmesh update around opening or closing door.
Primary usage is for rotating signs like in Seyda Neen at Arrille's Tradehouse entrance.
Decreasing this value may increase CPU usage by background threads.

Developer's settings
********************

This section is for developers or anyone who wants to investigate how nav mesh system works in OpenMW.

enable write recast mesh to file
--------------------------------

:Type:		boolean
:Range:		True/False
:Default:	False

Write recast mesh to file in .obj format for each use to update nav mesh.
Option is used to find out what world geometry is used to build nav mesh.
Potentially decreases performance.

enable write nav mesh to file
-----------------------------

:Type:		boolean
:Range:		True/False
:Default:	False

Write nav mesh to file to be able to open by RecastDemo application.
Usually it is more usefull to have both enable write recast mesh to file and this options enabled.
RecastDemo supports .obj files.
Potentially decreases performance.

enable recast mesh file name revision
-------------------------------------

:Type:		boolean
:Range:		True/False
:Default:	False

Write each recast mesh file with revision in name.
Otherwise will rewrite same file.
If it is unclear when geometry is changed use this option to dump multiple files for each state.

enable nav mesh file name revision
----------------------------------

:Type:		boolean
:Range:		True/False
:Default:	False

Write each nav mesh file with revision in name.
Otherwise will rewrite same file.
If it is unclear when nav mesh is changed use this option to dump multiple files for each state.

recast mesh path prefix
-----------------------

:Type:		string
:Range:		file system path
:Default:	""

Write recast mesh file at path with this prefix.

nav mesh path prefix
--------------------

:Type:		string
:Range:		file system path
:Default:	""

Write nav mesh file at path with this prefix.

enable nav mesh render
----------------------

:Type:		boolean
:Range:		True/False
:Default:	False

Render nav mesh.
Allows to do in-game debug.
Every nav mesh is visible and every update is noticable.
Potentially decreases performance.

enable agents paths render
-------------------

:Type:		boolean
:Range:		True/False
:Default:	False

Render agents paths.
Make visible all NPC's and creaure's plans where they are going.
Works even if Navigator is disabled.
Potentially decreases performance.

enable recast mesh render
----------------------

:Type:		boolean
:Range:		True/False
:Default:	False

Render recast mesh that is built as set of culled tiles from physical mesh.
Should show similar mesh to physical one.
Little difference can be a result of floating point error.
Absent pieces usually mean a bug in recast mesh tiles building.
Allows to do in-game debug.
Potentially decreases performance.

Expert settings
***************

This section is for developers who wants to go deeper into Detournavigator component logic.

recast scale factor
-------------------

:Type:		floating point
:Range:		> 0.0
:Default:	0.029411764705882353

Scale of nav mesh coordinates to world coordinates. Recastnavigation builds voxels for world geometry.
Basically voxel size is 1 / "cell size". To reduce amount of voxels we apply scale factor, to make voxel size
"recast scale factor" / "cell size". Default value calculates by this equation:
sStepSizeUp * "recast scale factor" / "cell size" = 5 (max climb height should be equal to 4 voxels).
Changing this value will change generated nav mesh. Some locations may become unavailable for NPC and creatures.
Pay attention to slopes and roofs when change it. Increasing this value will reduce nav mesh update latency.

max polygon path size
---------------------

:Type:		integer
:Range:		> 0
:Default:	1024

Maximum size of path over polygons.

max smooth path size
--------------------

:Type:		integer
:Range:		> 0
:Default:	1024

Maximum size of smoothed path.

triangles per chunk
-------------------

:Type:		integer
:Range:		> 0
:Default:	256

Maximum number of triangles in each node of mesh AABB tree.

Expert Recastnavigation related settings
****************************************

This section is for OpenMW developers who knows about `Recastnavigation <https://github.com/recastnavigation/recastnavigation>`_ library and understands how it works.

cell height
-----------

:Type:		floating point
:Range:		> 0.0
:Default:	0.2

The z-axis cell size to use for fields.
Defines voxel/grid/cell size. So their values have significant
side effects on all parameters defined in voxel units.
The minimum value for this parameter depends on the platform's floating point
accuracy, with the practical minimum usually around 0.05.
Same default value is used in RecastDemo.

cell size
---------

:Type:		floating point
:Range:		> 0.0
:Default:	0.2

The xy-plane cell size to use for fields.
Defines voxel/grid/cell size. So their values have significant
side effects on all parameters defined in voxel units.
The minimum value for this parameter depends on the platform's floating point
accuracy, with the practical minimum usually around 0.05.
Same default value is used in RecastDemo.

detail sample dist
------------------

:Type:		floating point
:Range:		= 0.0 or >= 0.9
:Default:	6.0

Sets the sampling distance to use when generating the detail mesh.

detail sample max error
-----------------------

:Type:		floating point
:Range:		>= 0.0
:Default:	1.0

The maximum distance the detail mesh surface should deviate from heightfield data.

max simplification error
------------------------

:Type:		floating point
:Range:		>= 0.0
:Default:	1.3

The maximum distance a simplfied contour's border edges should deviate the original raw contour.

tile size
---------

:Type:		integer
:Range:		> 0
:Default:	64

The width and height of each tile.

border size
-----------

:Type:		integer
:Range:		>= 0
:Default:	16

The size of the non-navigable border around the heightfield.

max edge len
------------

:Type:		integer
:Range:		>= 0
:Default:	12

The maximum allowed length for contour edges along the border of the mesh.

max nav mesh query nodes
------------------------

:Type:		integer
:Range:		0 < value <= 65535
:Default:	2048

Maximum number of search nodes.

max polygons per tile
---------------------

:Type:		integer
:Range:		2^n, 0 < n < 22
:Default:	4096

Maximum number of polygons per nav mesh tile. Maximum number of nav mesh tiles depends on
this value. 22 bits is a limit to store both tile identifier and polygon identifier (tiles = 2^(22 - log2(polygons))).
See `recastnavigation <https://github.com/recastnavigation/recastnavigation>`_ for more details.

.. Warning::
    Lower value may lead to ignored world geometry on nav mesh.
    Greater value will reduce number of nav mesh tiles.
    This condition is always true: ``max tiles number * max polygons per tile <= 4194304``.
    It's a limitation of `Recastnavigation <https://github.com/recastnavigation/recastnavigation>`_ library.

max verts per poly
------------------

:Type:		integer
:Range:		>= 3
:Default:	6

The maximum number of vertices allowed for polygons generated during the contour to polygon conversion process.

region merge size
-----------------

:Type:		integer
:Range:		>= 0
:Default:	20

Any regions with a span count smaller than this value will, if possible, be merged with larger regions.

region min size
---------------

:Type:		integer
:Range:		>= 0
:Default:	8

The minimum number of cells allowed to form isolated island areas.
