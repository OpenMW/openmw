Navigator Settings
##################

.. omw-setting::
   :title: enable
   :type: boolean
   :range: true, false
   :default: true

   Enables the navigator system, building a navmesh for pathfinding.
   Disabling uses only path grid, which affects NPC AI and pathfinding.
   May impact performance, especially on single-core CPUs.

.. omw-setting::
   :title: max tiles number
   :type: int
   :range: ≥ 0
   :default: 512

   Sets the number of tiles in the navmesh area around the player.
   Increasing can decrease performance.
   Must satisfy: max tiles number * max polygons per tile ≤ 4194304.

.. omw-setting::
   :title: wait until min distance to player
   :type: int
   :range: ≥ 0
   :default: 5

   Distance in tiles around player to delay loading screen until navmesh is generated.
   Zero disables waiting.

.. omw-setting::
   :title: enable nav mesh disk cache
   :type: boolean
   :range: true, false
   :default: true

   Enables using disk cache for navmesh tiles in addition to memory cache.

.. omw-setting::
   :title: write to navmeshdb
   :type: boolean
   :range: true, false
   :default: true

   Enables writing generated navmesh tiles to disk cache during runtime.

.. omw-setting::
   :title: max navmeshdb file size
   :type: uint
   :range: > 0
   :default: 2147483648

   Maximum size in bytes of navmesh disk cache file.

.. omw-setting::
   :title: async nav mesh updater threads
   :type: uint
   :range: ≥ 1
   :default: 1

   Number of background threads updating navmesh.
   Increasing threads may affect latency and performance.

.. omw-setting::
   :title: max nav mesh tiles cache size
   :type: uint
   :range: ≥ 0
   :default: 268435456

   Maximum memory size for cached navmesh tiles.
   Larger cache reduces update latency but uses more memory.

.. omw-setting::
   :title: min update interval ms
   :type: int
   :range: ≥ 0
   :default: 250

   Minimum milliseconds between navmesh updates per tile when objects move.
   Smaller values increase CPU usage.

.. omw-setting::
   :title: enable write recast mesh to file
   :type: boolean
   :range: true, false
   :default: false

   Write recast mesh to .obj file on each update for debugging.

.. omw-setting::
   :title: enable write nav mesh to file
   :type: boolean
   :range: true, false
   :default: false

   Write navmesh to file readable by RecastDemo app.

.. omw-setting::
   :title: enable recast mesh file name revision
   :type: boolean
   :range: true, false
   :default: false

   Append revision number to recast mesh file names to keep history.

.. omw-setting::
   :title: enable nav mesh file name revision
   :type: boolean
   :range: true, false
   :default: false

   Append revision number to navmesh file names to keep history.

.. omw-setting::
   :title: recast mesh path prefix
   :type: string
   :default: ""

   File path prefix for recast mesh files.

.. omw-setting::
   :title: nav mesh path prefix
   :type: string
   :default: ""

   File path prefix for navmesh files.

.. omw-setting::
   :title: enable nav mesh render
   :type: boolean
   :range: true, false
   :default: false

   Render the navmesh in-game for debugging.

.. omw-setting::
   :title: nav mesh render mode
   :type: string
   :range: "area type", "update frequency"
   :default: "area type"

   Mode to render navmesh: color by area type or show update frequency heatmap.

.. omw-setting::
   :title: enable agents paths render
   :type: boolean
   :range: true, false
   :default: false

   Render NPC/creature planned paths, even if navigator disabled.

.. omw-setting::
   :title: enable recast mesh render
   :type: boolean
   :range: true, false
   :default: false

   Render recast mesh (culled tiles from physical mesh) for debugging.

.. omw-setting::
   :title: wait for all jobs on exit
   :type: boolean
   :range: true, false
   :default: false

   Wait for all async navmesh jobs to complete before engine exit.

.. omw-setting::
   :title: recast scale factor
   :type: float32
   :range: > 0.0
   :default: 0.029411764705882353

   Scale factor between navigation mesh voxels and world units.
   Changing affects mesh generation and navigation accuracy.

.. omw-setting::
   :title: max polygon path size
   :type: uint
   :range: > 0
   :default: 1024

   Maximum path length over polygons.

.. omw-setting::
   :title: max smooth path size
   :type: uint
   :range: > 0
   :default: 1024

   Maximum length of smoothed path.

.. omw-setting::
   :title: cell height
   :type: float32
   :range: > 0.0
   :default: 0.2

   Height (Z axis) size of each voxel cell in navigation mesh.

.. omw-setting::
   :title: cell size
   :type: float32
   :range: > 0.0
   :default: 0.2

   XY plane size of each voxel cell in navigation mesh.

.. omw-setting::
   :title: detail sample dist
   :type: float32
   :range: 0.0 or ≥ 0.9
   :default: 6.0

   Sampling distance when generating detail mesh.

.. omw-setting::
   :title: detail sample max error
   :type: float32
   :range: ≥ 0.0
   :default: 1.0

   Maximum deviation distance of detail mesh surface from heightfield.

.. omw-setting::
   :title: max simplification error
   :type: float32
   :range: ≥ 0.0
   :default: 1.3

   Max deviation for simplified contours from raw contour.

.. omw-setting::
   :title: tile size
   :type: int
   :range: > 0
   :default: 128

   Width and height of each navmesh tile in voxels.

.. omw-setting::
   :title: border size
   :type: int
   :range: ≥ 0
   :default: 16

   Size of non-navigable border around heightfield.

.. omw-setting::
   :title: max edge len
   :type: int
   :range: ≥ 0
   :default: 12

   Max length for contour edges on mesh border.

.. omw-setting::
   :title: max nav mesh query nodes
   :type: int
   :range: [1, 65535]
   :default: 2048

   Maximum number of search nodes for pathfinding queries.

.. omw-setting::
   :title: max polygons per tile
   :type: int
   :range: powers of two [0, 22]
   :default: 4096

   Max polygons per navmesh tile.
   Must satisfy: max tiles number * max polygons per tile ≤ 4194304.

.. omw-setting::
   :title: max verts per poly
   :type: int
   :range: ≥ 3
   :default: 6

   Max vertices per polygon in mesh.

.. omw-setting::
   :title: region merge area
   :type: int
   :range: ≥ 0
   :default: 400

   Regions smaller than this may be merged with larger ones.

.. omw-setting::
   :title: region min area
   :type: int
   :range: ≥ 0
   :default: 64

   Minimum cell count to form isolated regions.
