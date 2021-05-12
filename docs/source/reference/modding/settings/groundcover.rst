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

rendering distance
------------------

:Type:		floating point
:Range:		>= 0.0
:Default:	6144.0

Determines on which distance in game units grass pages are rendered.
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

stomp mode
----------

:Type:		integer
:Range:		0, 1, 2
:Default:	2

Determines whether grass should respond to the player treading on it.

.. list-table:: Modes
	:header-rows: 1
	* - Mode number
	  - Meaning
	* - 0
	  - Grass cannot be trampled.
	* - 1
	  - The player's XY position is taken into account.
	* - 2
	  - The player's height above the ground is taken into account, too.

In MGE XE, which existing grass mods were made for, only the player's XY position was taken into account.
However, presumably due to a bug, jumping straight up would change the XY position, so grass *does* respond to the player jumping.
Levitating above grass in MGE XE still considers it stood-on, which can look bad.
OpenMW's height-aware system ensures grass does not act as if it's being stood on when the player levitates above it, but that means grass rapidly snaps back to its original position when the player jumps out of it.
Therefore, it is not recommended to use MGE XE's intensity constants if this setting is set to 2, i.e. :ref:`stomp intensity` should be 0 or 1 when :ref:`stomp mode` is 2.

stomp intensity
---------------

:Type:		integer
:Range:		0, 1, 2
:Default:	1

How far away from the player grass can be before it's unaffected by being trod on, and how far it moves when it is.

.. list-table:: Presets
	:header-rows: 1
	* - Preset number
	  - Range (Units)
	  - Distance (Units)
	  - Description
	* - 2
	  - 150
	  - 60
	  - MGE XE levels. Generally excessive/comical, but what existing mods were made with in mind.
	* - 1
	  - 80
	  - 40
	  - Reduced levels. Usually looks better.
	* - 0
	  - 50
	  - 20
	  - Gentle levels.
