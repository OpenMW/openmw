Terrain Settings
################

vertex lod mod
--------------

:Type:      int
:Range:     int
:Default:   0

Controls only the Vertex LOD of the terrain. The amount of terrain chunks and the detail of composite maps is left unchanged.

Must be changed in increments of 1. Each increment will double (for positive values) or halve (for negative values) the number of vertices rendered. For example: -2 means 4x reduced detail, +3 means 8x increased detail.

Note this setting will typically not affect near terrain. When set to increase detail, the detail of near terrain can not be increased because the detail is simply not there in the data files, and when set to reduce detail, the detail of near terrain will not be reduced because it was already less detailed than the far terrain (in view relative terms) to begin with.

composite map level
-------------------

:Type:		float
:Range:		>= 0
:Default:	1

Controls at what size (in cell units) terrain chunks will start to use a composite map instead of the high-detail textures. Should be changed in multiples of two (e.g. 0.125, 0.25, 0.5, 1, 2). A value of 0.125 (which is the minimum size of a terrain chunk) results in composite maps being used everywhere. Values higher than 1 result in the map window no longer using composited textures.

A composite map is a pre-rendered texture that contains all the texture layers combined. Note the resolution of composite maps is currently always fixed at 'composite map resolution', regardless of the resolution of the underlying terrain textures. This makes using composite maps for nearby terrain quite apparent if high-detail texture replacers are used.

composite map resolution
------------------------

:Type:		int
:Range:		>0
:Default:	512

Controls the resolution of composite maps. Larger values result in increased detail, but may take longer to prepare and thus could result in longer loading times and an increased chance of frame drops during play. As with most other texture resolution settings, it's most efficient to use values that are powers of two.

An easy way to observe changes to loading time is to load a save in an interior next to an exterior door (so it will start preloding terrain) and watch how long it takes for the 'Composite' counter on the F4 panel to fall to zero.

wait for composite maps
-----------------------

:Type:      bool
:Range:     True/False
:Default:   True

If disabled, composited terrain chunks will be rendered without textures if its composite maps are not ready yet. If enabled, the game will effectively freeze until those composite maps are ready and only then commence rendering.

This setting can be disabled for testing purposes so that the game loads faster.

lod factor
----------

:Type:		float
:Range:		>0
:Default:	1.0

Be careful about changing this. There are some consequences described below:

Controls the level of detail of the terrain. Values > 1 increase detail at the cost of performance, values < 1 reduce detail but increase performance. Both Vertex LOD and Texture LOD are affected.

Note this also changes how the Quad Tree is split. Increasing detail with this setting results in the visible terrain being divided into more chunks, where as reducing detail with this setting would reduce the number of chunks.

Fewer terrain chunks is faster for rendering, but on the other hand a larger proportion of the entire terrain must be rebuilt when LOD levels change as the camera moves. This could result in frame drops if moving across the map at high speed. 

For this reason, it is not recommended to change this setting if you want to change the LOD. If you want to do that, first try using the 'vertex lod mod' setting to configure the detail of the terrain outlines to your liking and then use 'composite map resolution' to configure the texture detail to your liking. But these settings can only be changed in multiples of two, so you may want to adjust 'lod factor' afterwards for even more fine-tuning.
