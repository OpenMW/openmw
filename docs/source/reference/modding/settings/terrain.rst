Terrain Settings
################

lod factor
----------

:Type:		float
:Range:		>0
:Default:	1.0

Controls the level of detail of the terrain. Higher values increase detail at the cost of performance, lower values reduce detail but increase performance.

composite map level
-------------------

:Type:		float
:Range:		>= 0
:Default:	1

Controls when the terrain will start to use a composite map instead of the high-detail textures. Should be changed in multiples of two (e.g. 0.125, 0.25, 0.5, 1, 2). A value of 0.125 (which is the minimum size of a terrain chunk) results in composite maps being used everywhere. Values higher than 1 result in the map window no longer using composited textures.

A composite map is a pre-rendered texture that contains all the texture layers combined. Note the resolution of composite maps is currently always fixed at 'composite map resolution', regardless of the resolution of the underlying terrain textures. This makes using composite maps for nearby terrain quite apparent if high-detail texture replacers are used.

composite map resolution
------------------------

:Type:		int
:Range:		>0
:Default:	512

Controls the resolution of composite maps. Larger values result in increased detail, but may take longer to prepare and thus result in longer loading times and an increased chance of frame drops during play. As with most other texture resolution settings, it's most efficient to use values that are powers of two.

wait for composite maps
-----------------------

:Type:      bool
:Range:     True/False
:Default:   True

If disabled, composited terrain chunks will be rendered without textures if its composite maps are not ready yet. If enabled, the game will effectively freeze until those composite maps are ready and only then commence rendering.

This setting can be disabled for testing purposes so that the game loads faster.
