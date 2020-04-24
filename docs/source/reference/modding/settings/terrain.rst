Terrain Settings
################

distant terrain
---------------

:Type:		boolean
:Range:		True/False
:Default:	False

Controls whether the engine will use paging and LOD algorithms to load the terrain of the entire world at all times.
Otherwise, only the terrain of the surrounding cells is loaded.

.. note::
	When enabling distant terrain, make sure the 'viewing distance' in the camera section is set to a larger value so
	that you can actually see the additional terrain.

To avoid frame drops as the player moves around, nearby terrain pages are always preloaded in the background,
regardless of the preloading settings in the 'Cells' section,
but the preloading of terrain behind a door or a travel destination, for example,
will still be controlled by cell preloading settings.

The distant terrain engine is currently considered experimental
and may receive updates and/or further configuration options in the future.
The glaring omission of non-terrain objects in the distance somewhat limits this setting's usefulness.

vertex lod mod
--------------

:Type:      integer
:Range:     any
:Default:   0

Controls only the Vertex LOD of the terrain. The amount of terrain chunks and the detail of composite maps is left unchanged.

Must be changed in increments of 1. Each increment will double (for positive values) or halve (for negative values) the number of vertices rendered.
For example: -2 means 4x reduced detail, +3 means 8x increased detail.

Note this setting will typically not affect near terrain. When set to increase detail, the detail of near terrain can not be increased
because the detail is simply not there in the data files, and when set to reduce detail,
the detail of near terrain will not be reduced because it was already less detailed than the far terrain (in view relative terms) to begin with.

lod factor
----------

:Type:		float
:Range:		>0
:Default:	1.0

Controls the level of detail if distant terrain is enabled.
Higher values increase detail at the cost of performance, lower values reduce detail but increase performance.

Note: it also changes how the Quad Tree is split.
Increasing detail with this setting results in the visible terrain being divided into more chunks,
where as reducing detail with this setting would reduce the number of chunks.

Fewer terrain chunks is faster for rendering, but on the other hand a larger proportion of the entire terrain
must be rebuilt when LOD levels change as the camera moves.
This could result in frame drops if moving across the map at high speed.

For this reason, it is not recommended to change this setting if you want to change the LOD.
If you want to do that, first try using the 'vertex lod mod' setting to configure the detail of the terrain outlines
to your liking and then use 'composite map resolution' to configure the texture detail to your liking.
But these settings can only be changed in multiples of two, so you may want to adjust 'lod factor' afterwards for even more fine-tuning.

composite map level
-------------------

:Type:		integer
:Range:		>= -3
:Default:	0

Controls at which minimum size (in 2^value cell units) terrain chunks will start to use a composite map instead of the high-detail textures.
With value -3 composite maps are used everywhere.

A composite map is a pre-rendered texture that contains all the texture layers combined.
Note that resolution of composite maps is currently always fixed at 'composite map resolution',
regardless of the resolution of the underlying terrain textures.
If high resolution texture replacers are used, it is recommended to increase 'composite map resolution' setting value.

composite map resolution
------------------------

:Type:		integer
:Range:		>0
:Default:	512

Controls the resolution of composite maps. Larger values result in increased detail,
but may take longer to prepare and thus could result in longer loading times and an increased chance of frame drops during play.
As with most other texture resolution settings, it's most efficient to use values that are powers of two.

An easy way to observe changes to loading time is to load a save in an interior next to an exterior door
(so it will start preloding terrain) and watch how long it takes for the 'Composite' counter on the F4 panel to fall to zero.

max composite geometry size
---------------------------

:Type:		float
:Range:		>=1.0
:Default:	4.0

Controls the maximum size of simple composite geometry chunk in cell units. With small values there will more draw calls and small textures,
but higher values create more overdraw (not every texture layer is used everywhere).
