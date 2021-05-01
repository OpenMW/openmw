Terrain Settings
################

distant terrain
---------------

:Type:		boolean
:Range:		True/False
:Default:	False

Controls whether the engine will use paging (chunking) and LOD algorithms to load the terrain of the entire world at all times.
Otherwise, only the terrain of the surrounding cells is loaded.

.. note::
	When enabling distant terrain, make sure the 'viewing distance' in the camera section is set to a larger value so
	that you can actually see the additional terrain and objects.

To avoid frame drops as the player moves around, nearby terrain pages are always preloaded in the background,
regardless of the preloading settings in the 'Cells' section,
but the preloading of terrain behind a door or a travel destination, for example,
will still be controlled by cell preloading settings.

The distant terrain engine is currently considered experimental and may receive updates in the future.

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

object paging
-------------

:Type:		boolean
:Range:		True/False
:Default:	True

Controls whether the engine will use paging (chunking) algorithms to load non-terrain objects
outside of the active cell grid.

Depending on the settings below every object in the game world has a chance
to be batched and be visible in the game world, effectively allowing
the engine to render distant objects with a relatively low performance impact automatically.

In general, an object is more likely to be batched if the number of the object's vertices
and the corresponding memory cost of merging the object is low compared to
the expected number of the draw calls that are going to be optimized out.
This memory cost and the saved number of draw calls shall be called
the "merging cost" and the "merging benefit" in the following documentation.

Objects that are scripted to disappear from the game world
will be handled properly as long as their scripts have a chance to actually disable them.

This setting has no effect if distant terrain is disabled.

object paging active grid
-------------------------
:Type:		boolean
:Range:		True/False
:Default:	True

Controls whether the objects in the active cells use the mentioned paging algorithms.
Active grid paging significantly improves the framerate when your setup is CPU-limited.

.. note::
	Given that only 8 light sources may affect an object at a time at the moment,
	lighting issues arising due to merged objects being considered a single object
	may disrupt your gameplay experience.

object paging merge factor
--------------------------
:Type:		float
:Range:		>0
:Default:	250.0

Affects the likelyhood of objects being merged.
Higher values improve the framerate at the cost of memory.

Technically this is implemented as a multiplier to the merging benefit, and since
an object has a lot of vertices, sometimes in terms of hundreds and thousands,
and doesn't often need as much draw calls to be rendered (typically that number is in 1 or 2 digits)
this value needs to be large enough, as this is what makes
the merging cost and the merging benefit actually comparable for the sake of paging.

object paging min size
----------------------
:Type:		float
:Range:		>0
:Default:	0.01

Controls how large an object must be to be visible in the scene.
The object's size is divided by its distance to the camera
and the result of the division is compared with this value.
The smaller this value is, the more objects you will see in the scene.

object paging min size merge factor
-----------------------------------
:Type:		float
:Range:		>0
:Default:	0.3

This setting gives inexpensive objects a chance to be rendered from a greater distance
even if the engine would rather discard them according to the previous setting.

It controls the factor that the minimum size is multiplied by
roughly according to the following formula:

	factor = merge cost * min size cost multiplier / merge benefit
	
	factor = factor + (1 - factor) * min size merge factor

Since the larger this factor is, the smaller chance a large object has to be rendered,
decreasing this value makes more objects visible in the scene
without impacting the performance as dramatically as the minimum size setting.

object paging min size cost multiplier
--------------------------------------
:Type:		float
:Range:		>0
:Default:	25.0

This setting adjusts the calculated cost of merging an object used in the mentioned functionality.
The larger this value is, the less expensive objects can be before they are discarded.
See the formula above to figure out the math.

object paging debug batches
---------------------------
:Type:		boolean
:Range:		True/False
:Default:	False

This debug setting allows you to see what objects have been merged in the scene
by making them colored randomly.
