Camera Settings
###############

near clip
---------

:Type:		floating point
:Range:		> 0
:Default:	3.0

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
:Default:	7168.0

This value controls the maximum visible distance (also called the far clipping plane).
Larger values significantly improve rendering in exterior spaces,
but also increase the amount of rendered geometry and significantly reduce the frame rate.
Note that when cells are visible before loading, the geometry will "pop-in" suddenly,
creating a jarring visual effect. To prevent this effect, this value should not be greater than:

	CellSizeInUnits * CellGridRadius - 1024

The CellSizeInUnits is the size of a game cell in units (8192 by default), CellGridRadius determines how many
neighboring cells to current one to load (1 by default - 3x3 grid), and 1024 is the threshold distance for loading a new cell.
The field of view setting also interacts with this setting because the view frustum end is a plane,
so you can see further at the edges of the screen than you should be able to.
This can be observed in game by looking at distant objects
and rotating the camera so the objects are near the edge of the screen.
As a result, this distance is recommended to further be reduced to avoid pop-in for wide fields of view
and long viewing distances near the edges of the screen if distant terrain and object paging are not used.

Reductions of up to 25% or more can be required to completely eliminate this pop-in.
Such situations are unusual and probably not worth the performance penalty introduced
by loading geometry obscured by fog in the center of the screen.
See RenderingManager::configureFog for the relevant source code.

This setting can be adjusted in game from the ridiculously low value of 2500 units to a maximum of 7168 units
using the View Distance slider in the Detail tab of the Video panel of the Options menu, unless distant terrain is enabled,
in which case the maximum is increased to 81920 units.

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

third person camera distance
----------------------------

:Type:		floating point
:Range:		30-800
:Default:	192.0

Distance from the camera to the character in third person mode.

This setting can be changed in game using "Zoom In" / "Zoom Out" controls.

view over shoulder
------------------

:Type:		boolean
:Range:		True/False
:Default:	False

This setting controls third person view mode.
False: View is centered on the character's head. Crosshair is hidden.
True: In non-combat mode camera is positioned behind the character's shoulder. Crosshair is visible in third person mode as well.

This setting can be controlled in Advanced tab of the launcher.

view over shoulder offset
-------------------------

:Type:		2D vector floating point
:Range:		Any
:Default:	30 -10

This setting makes sense only if 'view over shoulder' is enabled. Controls horizontal (first number) and vertical (second number) offset of the camera in third person mode.
Recommened values: 30 -10 for the right shoulder, -30 -10 for the left shoulder.

This setting can only be configured by editing the settings configuration file.

auto switch shoulder
--------------------

:Type:		boolean
:Range:		True/False
:Default:	True

This setting makes difference only in third person mode if 'view over shoulder' is enabled.
When player is close to an obstacle, automatically switches camera to the shoulder that is farther away from the obstacle.

This setting can be controlled in Advanced tab of the launcher.

zoom out when move coef
-----------------------

:Type:		floating point
:Range:		Any
:Default:	20

This setting makes difference only in third person mode if 'view over shoulder' is enabled.
Slightly pulls camera away (or closer in case of negative value) when the character moves. To disable set it to zero.

This setting can only be configured by editing the settings configuration file.

preview if stand still
----------------------

:Type:		boolean
:Range:		True/False
:Default:	False

Makes difference only in third person mode.
If enabled then the character rotation is not synchonized with the camera rotation while the character doesn't move and not in combat mode.

This setting can be controlled in Advanced tab of the launcher.

deferred preview rotation
-------------------------

:Type:		boolean
:Range:		True/False
:Default:	True

Makes difference only in third person mode.
If enabled then the character smoothly rotates to the view direction after exiting preview or vanity mode.
If disabled then the camera rotates rather than the character.

This setting can be controlled in Advanced tab of the launcher.

head bobbing
------------

:Type:		boolean
:Range:		True/False
:Default:	False

Enables head bobbing when move in first person mode.

This setting can be controlled in Advanced tab of the launcher.

head bobbing step
-----------------

:Type:		floating point
:Range:		>0
:Default:	90.0

Makes diffence only in first person mode if 'head bobbing' is enabled.
Length of each step.

This setting can only be configured by editing the settings configuration file.

head bobbing height
-------------------

:Type:		floating point
:Range:		Any
:Default:	3.0

Makes diffence only in first person mode if 'head bobbing' is enabled.
Amplitude of the head bobbing.

This setting can only be configured by editing the settings configuration file.

head bobbing roll
-----------------

:Type:		floating point
:Range:		0-90
:Default:	0.2

Makes diffence only in first person mode if 'head bobbing' is enabled.
Maximum roll angle in degrees.

This setting can only be configured by editing the settings configuration file.

