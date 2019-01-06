Map Settings
############

global
------

:Type:		boolean
:Range:		True/False
:Default:	False

If this value is true, the map window will display the world map, otherwise the local map. 
The setting updates automatically when pressing the local/world map switch button on the map window.

global map cell size
--------------------

:Type:		integer
:Range:		>= 1
:Default:	18

This setting adjusts the scale of the world map in the GUI mode map window.
The value is the width in pixels of each cell in the map, so larger values result in larger more detailed world maps,
while smaller values result in smaller less detailed world maps.
However, the native resolution of the map source material appears to be 9 pixels per unexplored cell
and approximately 18 pixels per explored cell, so values larger than 36 don't produce much additional detail.
Similarly, the size of place markers is currently fixed at 12 pixels,
so values smaller than this result in overlapping place markers.
Values from 12 to 36 are recommended. For reference, Vvardenfell is approximately 41x36 cells.

.. Warning::
	Changing this setting affects saved games. The currently explored area is stored as an image
	in the save file that's overlayed on the default world map in game.
	When you increase the resolution of the map, the overlay of earlier saved games will be scaled up on load,
	and appear blurry. When you visit the cell again, the overlay for that cell is regenerated at the new resolution,
	so the blurry areas can be corrected by revisiting all the cells you've already visited.

This setting can not be configured except by editing the settings configuration file.

local map hud widget size
-------------------------

:Type:		integer
:Range:		>= 1
:Default:	256

This setting controls the zoom level for the HUD map widget (the map in the lower right corner of the window).
A value of 64 results in the HUD map widget displaying one entire exterior cell.
Since the GUI mode map displays 3x3 cells, a value of approximately 21 displays the same area as the GUI mode map.
Larger values increase the level of zoom,
while smaller values are wasteful since there's no map data to display beyond the 3x3 cell grid.

Note that the actual size of the widget is always the same on the screen
unless the scaling factor setting in the "GUI" section is changed.
Increasing both the scaling factor of the GUI and this setting does result in a higher resolution HUD map,
unfortunately with a scaled direction pointer on top of it.

This setting can not be configured except by editing the settings configuration file.

local map hud fog of war
------------------------

:Type:		boolean
:Range:		True/False
:Default:	False

This setting enables fog of war rendering on the HUD map.
Default is Off since with default settings the map is so small that the fog would not obscure anything,
just darken the edges slightly.

local map resolution
--------------------

:Type:		integer
:Range:		>= 1
:Default:	256

This setting controls the resolution of the GUI mode local map window.
Larger values generally increase the visible detail in map.
If this setting is half the local map widget size or smaller, the map will generally be be fairly blurry.
Setting both options to the same value results in a map with good detail.
Values that exceed the local map widget size setting by more than a factor of two
are unlikely to provide much of an improvement in detail since they're subsequently scaled back
to the approximately the map widget size before display.
The video resolution settings interacts with this setting in that regard.

.. warning::
	Increasing this setting can increase cell load times,
	because the map is rendered on demand each time you enter a new cell.
	Large values may exceed video card limits or exhaust VRAM.

This setting can not be configured except by editing the settings configuration file.

local map widget size
---------------------

:Type:		integer
:Range:		>= 1
:Default:	512

This setting controls the canvas size of the GUI mode local map window.
Larger values result in a larger physical map size on screen,
and typically require more panning to see all available portions of the map.
This larger size also enables an overall greater level of detail if the local map resolution setting is also increased.

This setting can not be configured except by editing the settings configuration file.

local map cell distance
-----------------------

:Type:		integer
:Range:		>= 1
:Default:	1

Similar to "exterior cell load distance" in the Cells section, controls how many cells are rendered on the local map. 
Please note that only loaded cells can be rendered,
so this setting must be lower or equal to "exterior cell load distance" to work properly.
