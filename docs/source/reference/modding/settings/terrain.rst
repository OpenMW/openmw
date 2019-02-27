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

lod factor
----------

:Type:		float
:Range:		>0
:Default:	1.0

Controls the level of detail if distant terrain is enabled. Higher values increase detail at the cost of performance, lower values reduce detail but increase performance.

composite map level
-------------------

:Type:		integer
:Range:		>= -3
:Default:	0

Controls at what size (in 2^value cell units) terrain chunks will start to use a composite map instead of the high-detail textures.
With value -3 composite maps are used everywhere.
With value >= 1 the map window will not use composited textures.

A composite map is a pre-rendered texture that contains all the texture layers combined. Note that resolution of composite maps is currently always fixed at 'composite map resolution', regardless of the resolution of the underlying terrain textures. If high-detail texture replacers are used, probably it is worth to increase 'composite map resolution' setting value.

composite map resolution
------------------------

:Type:		integer
:Range:		>0
:Default:	512

Controls the resolution of composite maps. Larger values result in increased detail, but may take longer to prepare and thus could result in longer loading times and an increased chance of frame drops during play. As with most other texture resolution settings, it's most efficient to use values that are powers of two.

An easy way to observe changes to loading time is to load a save in an interior next to an exterior door (so it will start preloding terrain) and watch how long it takes for the 'Composite' counter on the F4 panel to fall to zero.
