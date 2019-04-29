Camera Settings
###############

near clip
---------

:Type:		floating point
:Range:		> 0
:Default:	1.0

This setting controls the distance to the near clipping plane. The value must be greater than zero.
Values greater than approximately 18.0 will occasionally clip objects in the world in front of the character.
Values greater than approximately 8.0 will clip the character's hands in first person view
and/or the back of their head in third person view.

This setting can only be configured by editing the settings configuration file.

small feature culling
---------------------

:Type:		boolean
:Range:		True/False
:Default:	True

This setting determines whether objects that render to a few pixels or smaller will be culled (not drawn).
It generally improves performance to enable this feature,
and by definition the culled objects will be very small on screen.
The size in pixels for an object to be considered 'small' is controlled by a separate setting.

This setting can only be configured by editing the settings configuration file.

small feature culling pixel size
--------------------------------

:Type:		floating point
:Range:		> 0
:Default:	2.0

Controls the cutoff in pixels for the 'small feature culling' setting,
which will have no effect if 'small feature culling' is disabled.

This setting can only be configured by editing the settings configuration file.

viewing distance
----------------

:Type:		floating point
:Range:		> 0
:Default:	6656.0

This value controls the maximum visible distance (also called the far clipping plane).
Larger values significantly improve rendering in exterior spaces,
but also increase the amount of rendered geometry and significantly reduce the frame rate.
This value interacts with the exterior cell load distance setting
in that it's probably undesired for this value to provide visibility into cells that have not yet been loaded.
When cells are visible before loading, the geometry will "pop-in" suddenly, creating a jarring visual effect.
To prevent this effect, this value must be less than::

	(8192 * exterior cell load distance - 1024) * 0.93

The constant 8192 is the size of a cell, and 1024 is the threshold distance for loading a new cell.
Additionally, the field of view setting also interacts with this setting because the view frustum end is a plane,
so you can see further at the edges of the screen than you should be able to.
This can be observed in game by looking at distant objects
and rotating the camera so the objects are near the edge of the screen.
As a result, this setting should further be reduced by a factor that depends on the field of view setting.
In the default configuration this reduction is 7%, hence the factor of 0.93 above.
Using this factor, approximate values recommended for other exterior cell load distance settings are:

======= ========
Cells	Viewing
        Distance
=======	========
2		14285
3		21903
4		29522
5		35924
=======	========

Reductions of up to 25% or more can be required to completely eliminate pop-in for wide fields of view
and long viewing distances near the edges of the screen.
Such situations are unusual and probably not worth the performance penalty introduced
by loading geometry obscured by fog in the center of the screen.
See RenderingManager::configureFog for the relevant source code.

Enabling the distant terrain setting is an alternative to increasing exterior cell load distance.
Note that the distant land setting does not include rendering of distant static objects,
so the resulting visual effect is not the same.

This setting can be adjusted in game from the ridiculously low value of 2048.0 to a maximum of 81920.0
using the View Distance slider in the Detail tab of the Video panel of the Options menu.

field of view
-------------

:Type:		floating point
:Range:		1-179
:Default:	55.0

Sets the camera field of view in degrees. Recommended values range from 30 degrees to 110 degrees.
Small values provide a very narrow field of view that creates a "zoomed in" effect,
while large values cause distortion at the edges of the screen.
The "field of view" setting interacts with aspect ratio of your video resolution in that more square aspect ratios
(e.g. 4:3) need a wider field of view to more resemble the same field of view on a widescreen (e.g. 16:9) monitor.

This setting can be changed in game using the Field of View slider from the Video tab of the Video panel of the Options menu.

first person field of view
--------------------------

:Type:		floating point
:Range:		1-179
:Default:	55.0

This setting controls the field of view for first person meshes such as the player's hands and held objects.
It is not recommended to change this value from its default value
because the Bethesda provided Morrowind assets do not adapt well to large values,
while small values can result in the hands not being visible.

This setting can only be configured by editing the settings configuration file.
