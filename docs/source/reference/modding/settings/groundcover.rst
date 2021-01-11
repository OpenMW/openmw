Groundcover Settings
####################

enabled
-------

:Type:		boolean
:Range:		True/False
:Default:	False

Allows the engine to use groundcover.
Groundcover objects are static objects which come from ESP files, registered via
"groundcover" entries from openmw.cfg rather than "content" ones.
We assume that groundcover objects have no collisions, can not be moved or interacted with,
so we can merge them to pages and animate them indifferently from distance from player.

This setting can only be configured by editing the settings configuration file.

density
-------

:Type:		floating point
:Range:		0.0 (0%) to 1.0 (100%)
:Default:	1.0

Determines how many groundcover instances from content files
are used in the game. Can affect performance a lot.

This setting can only be configured by editing the settings configuration file.

distance
--------

:Type:		integer
:Range:		> 0
:Default:	1

Determines on which distance in cells grass pages are rendered.
Default 1 value means 3x3 cells area (active grid).
May affect performance a lot.

This setting can only be configured by editing the settings configuration file.

min chunk size
--------------

:Type:		floating point
:Range:		0.125, 0.25, 0.5, 1.0
:Default:	0.5

Determines a minimum size of groundcover chunks in cells. For example, with 0.5 value
chunks near player will have size 4096x4096 game units. Larger chunks reduce CPU usage
(Draw and Cull bars), but can increase GPU usage (GPU bar) since culling becomes less efficient.
Smaller values do an opposite.

This setting can only be configured by editing the settings configuration file.
