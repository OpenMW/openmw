Windows Settings
################

:Type:	floating point
:Range:	0.0 to 1.0

This section controls the location and size of each window in GUI mode. Each setting is a floating point number representing a *fraction* of the resolution x or resolution y setting in the Video Settings Section. The X and Y values locate the top left corner of the window, while the W value determines the width of the window and the H value determines the height of the window.

Unlike the documentation for most sections which lists the exact setting name, this page instead lists the names of the windows. For example, to configure the alchemy window, the actual settings would be::

	alchemy x = 0.25
	alchemy y = 0.25
	alchemy h = 0.5
	alchemy w = 0.5

Each window in the GUI mode remembers it's previous location when exiting the game. By far the easiest way to configure these settings is to simply move the windows around in game. Hand editing the configuration file might result in some fine tuning for alignment, but the settings will be overwritten if a window is moved.

.. note::
	To scale the windows, making the widgets proportionally larger, see the scaling factor setting instead.

stats
-----

:Default:	x = 0.0
			y = 0.0
			h = 0.375
			w = 0.4275

The stats window, displaying level, race, class, skills and stats. Activated by clicking on any of the three bars in the lower left corner of the HUD.

spells
------

:Default:	x = 0.625
			y = 0.5725
			h = 0.375
			w = 0.4275

The spells window, displaying powers, spells, and magical items. Activated by clicking on the spells widget (third from left) in the bottom left corner of the HUD.

map
---

:Default:	x = 0.625
			y = 0.0
			h = 0.375
			w = 0.5725

The local and world map window. Activated by clicking on the map widget in the bottom right corner of the HUD.

dialogue
--------

:Default:	x = 0.095
			y = 0.095
			h = 0.810
			w = 0.810

The dialog window, for talking with NPCs. Activated by clicking on a NPC.

alchemy
-------

:Default:	x = 0.25
			y = 0.25
			h = 0.5
			w = 0.5

The alchemy window, for crafting potions. Activated by dragging an alchemy tool on to the rag doll. Unlike most other windows, this window hides all other windows when opened.

console
-------

:Default:	x = 0.0
			y = 0.0
			h = 1.0
			w = 0.5

The console command window. Activated by pressing the tilde (~) key.

inventory
---------

:Default:	x = 0.0
			y = 0.4275
			h = 0.6225
			w = 0.5725

The inventory window, displaying the paper doll and possessions, when activated by clicking on the inventory widget (second from left) in the bottom left corner of the HUD.

inventory container
-------------------

:Default:	x = 0.0
			y = 0.4275
			h = 0.6225
			w = 0.5725

The player's inventory window while searching a container, showing the contents of the character's inventory. Activated by clicking on a container. The same window is used for searching dead bodies, and pickpocketing people.

inventory barter
----------------

:Default:	x = 0.0
			y = 0.4275
			h = 0.6225
			w = 0.5725

The player's inventory window while bartering. It displays goods owned by the character while bartering. Activated by clicking on the Barter choice in the dialog window for an NPC.

inventory companion
-------------------

:Default:	x = 0.0
			y = 0.4275
			h = 0.6225
			w = 0.5725

The player's inventory window while interacting with a companion. The companion windows were added in the Tribunal expansion, but are available everywhere in the OpenMW engine.

container
---------

:Default:	x = 0.25
			y = 0.0
			h = 0.75
			w = 0.375

The container window, showing the contents of the container. Activated by clicking on a container. The same window is used for searching dead bodies, and pickpocketing people.

barter
------

:Default:	x = 0.25
			y = 0.0
			h = 0.75
			w = 0.375

The NPC bartering window, displaying goods owned by the shopkeeper while bartering. Activated by clicking on the Barter choice in the dialog window for an NPC.

companion
---------

:Default:	x = 0.25
			y = 0.0
			h = 0.75
			w = 0.375

The NPC's inventory window while interacting with a companion. The companion windows were added in the Tribunal expansion, but are available everywhere in the OpenMW engine.