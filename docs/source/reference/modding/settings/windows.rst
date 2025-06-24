Windows Settings
################

This section controls the location and size of each window in GUI mode.
Each setting is a floating point number representing a *fraction*
of the resolution x or resolution y setting in the Video Settings Section.
The X and Y values locate the top left corner of the window,
while the W value determines the width of the window and the H value determines the height of the window.

Each window in the GUI mode remembers it's previous location when exiting the game.
By far the easiest way to configure these settings is to simply move the windows around in game.
Hand editing the configuration file might result in some fine tuning for alignment,
but the settings will be overwritten if a window is moved.

.. note::
	To scale the windows, making the widgets proportionally larger, 
	see the scaling factor setting in the GUI section instead.

.. omw-setting::
   :title: stats window x
   :type: float32
   :range: [0, 1]
   :default: 0.015
   :location: GUI mode window position

   X coordinate (top-left corner) of the stats window.
   Displays level, race, class, skills, and stats.
   Activated by clicking any of the three bars in the lower left HUD corner.

.. omw-setting::
   :title: stats window y
   :type: float32
   :range: [0, 1]
   :default: 0.015
   :location: GUI mode window position

   Y coordinate (top-left corner) of the stats window.
   Displays level, race, class, skills, and stats.
   Activated by clicking any of the three bars in the lower left HUD corner.

.. omw-setting::
   :title: stats window w
   :type: float32
   :range: [0, 1]
   :default: 0.45
   :location: GUI mode window size

   Width of the stats window.
   Displays level, race, class, skills, and stats.
   Activated by clicking any of the three bars in the lower left HUD corner.

.. omw-setting::
   :title: stats window h
   :type: float32
   :range: [0, 1]
   :default: 0.4275
   :location: GUI mode window size

   Height of the stats window.
   Displays level, race, class, skills, and stats.
   Activated by clicking any of the three bars in the lower left HUD corner.

.. omw-setting::
   :title: stats window pin
   :type: boolean
   :range: true, false
   :default: false
   :location: GUI mode window pin state

   Whether the stats window is pinned.
   Activated by clicking any of the three bars in the lower left HUD corner.

.. omw-setting::
   :title: spells window x
   :type: float32
   :range: [0, 1]
   :default: 0.63
   :location: GUI mode window position

   X coordinate of the spells window.
   Displays powers, spells, and magical items.
   Activated by clicking the spells widget (third from left) in the lower left HUD corner.

.. omw-setting::
   :title: spells window y
   :type: float32
   :range: [0, 1]
   :default: 0.39
   :location: GUI mode window position

   Y coordinate of the spells window.
   Displays powers, spells, and magical items.
   Activated by clicking the spells widget (third from left) in the lower left HUD corner.

.. omw-setting::
   :title: spells window w
   :type: float32
   :range: [0, 1]
   :default: 0.36
   :location: GUI mode window size

   Width of the spells window.
   Displays powers, spells, and magical items.
   Activated by clicking the spells widget (third from left) in the lower left HUD corner.

.. omw-setting::
   :title: spells window h
   :type: float32
   :range: [0, 1]
   :default: 0.51
   :location: GUI mode window size

   Height of the spells window.
   Displays powers, spells, and magical items.
   Activated by clicking the spells widget (third from left) in the lower left HUD corner.

.. omw-setting::
   :title: spells window pin
   :type: boolean
   :range: true, false
   :default: false
   :location: GUI mode window pin state

   Whether the spells window is pinned.
   Activated by clicking the spells widget (third from left) in the lower left HUD corner.

.. omw-setting::
   :title: map window x
   :type: float32
   :range: [0, 1]
   :default: 0.63
   :location: GUI mode window position

   X coordinate of the map window.
   Displays local and world map.
   Activated by clicking the map widget in the lower right HUD corner.

.. omw-setting::
   :title: map window y
   :type: float32
   :range: [0, 1]
   :default: 0.015
   :location: GUI mode window position

   Y coordinate of the map window.
   Displays local and world map.
   Activated by clicking the map widget in the lower right HUD corner.

.. omw-setting::
   :title: map window w
   :type: float32
   :range: [0, 1]
   :default: 0.36
   :location: GUI mode window size

   Width of the map window.
   Displays local and world map.
   Activated by clicking the map widget in the lower right HUD corner.

.. omw-setting::
   :title: map window h
   :type: float32
   :range: [0, 1]
   :default: 0.37
   :location: GUI mode window size

   Height of the map window.
   Displays local and world map.
   Activated by clicking the map widget in the lower right HUD corner.

.. omw-setting::
   :title: map window pin
   :type: boolean
   :range: true, false
   :default: false
   :location: GUI mode window pin state

   Whether the map window is pinned.
   Activated by clicking the map widget in the lower right HUD corner.

.. omw-setting::
   :title: inventory window x
   :type: float32
   :range: [0, 1]
   :default: 0.015
   :location: GUI mode window position

   X coordinate of the inventory window.
   Displays paper doll and possessions.
   Activated by clicking the inventory widget (second from left) in the lower left HUD corner.

.. omw-setting::
   :title: inventory window y
   :type: float32
   :range: [0, 1]
   :default: 0.54
   :location: GUI mode window position

   Y coordinate of the inventory window.
   Displays paper doll and possessions.
   Activated by clicking the inventory widget (second from left) in the lower left HUD corner.

.. omw-setting::
   :title: inventory window w
   :type: float32
   :range: [0, 1]
   :default: 0.45
   :location: GUI mode window size

   Width of the inventory window.
   Displays paper doll and possessions.
   Activated by clicking the inventory widget (second from left) in the lower left HUD corner.

.. omw-setting::
   :title: inventory window h
   :type: float32
   :range: [0, 1]
   :default: 0.38
   :location: GUI mode window size

   Height of the inventory window.
   Displays paper doll and possessions.
   Activated by clicking the inventory widget (second from left) in the lower left HUD corner.

.. omw-setting::
   :title: inventory window pin
   :type: boolean
   :range: true, false
   :default: false
   :location: GUI mode window pin state

   Whether the inventory window is pinned.
   Displays paper doll and possessions.
   Activated by clicking the inventory widget (second from left) in the lower left HUD corner.

.. omw-setting::
   :title: inventory container window x
   :type: float32
   :range: [0, 1]
   :default: 0.015
   :location: GUI mode window position

   X coordinate of the inventory container window.
   Displays player inventory when searching a container.
   Activated by clicking a container, dead body, or during pickpocketing.

.. omw-setting::
   :title: inventory container window y
   :type: float32
   :range: [0, 1]
   :default: 0.54
   :location: GUI mode window position

   Y coordinate of the inventory container window.
   Displays player inventory when searching a container.
   Activated by clicking a container, dead body, or during pickpocketing.

.. omw-setting::
   :title: inventory container window w
   :type: float32
   :range: [0, 1]
   :default: 0.45
   :location: GUI mode window size

   Width of the inventory container window.
   Displays player inventory when searching a container.
   Activated by clicking a container, dead body, or during pickpocketing.

.. omw-setting::
   :title: inventory container window h
   :type: float32
   :range: [0, 1]
   :default: 0.38
   :location: GUI mode window size

   Height of the inventory container window.
   Displays player inventory when searching a container.
   Activated by clicking a container, dead body, or during pickpocketing.

.. omw-setting::
   :title: inventory barter window x
   :type: float32
   :range: [0, 1]
   :default: 0.015
   :location: GUI mode window position

   X coordinate of the inventory barter window.
   Displays character goods while bartering.
   Activated by clicking Barter in NPC dialog.

.. omw-setting::
   :title: inventory barter window y
   :type: float32
   :range: [0, 1]
   :default: 0.54
   :location: GUI mode window position

   Y coordinate of the inventory barter window.
   Displays character goods while bartering.
   Activated by clicking Barter in NPC dialog.

.. omw-setting::
   :title: inventory barter window w
   :type: float32
   :range: [0, 1]
   :default: 0.45
   :location: GUI mode window size

   Width of the inventory barter window.
   Displays character goods while bartering.
   Activated by clicking Barter in NPC dialog.

.. omw-setting::
   :title: inventory barter window h
   :type: float32
   :range: [0, 1]
   :default: 0.38
   :location: GUI mode window size

   Height of the inventory barter window.
   Displays character goods while bartering.
   Activated by clicking Barter in NPC dialog.

.. omw-setting::
   :title: inventory companion window x
   :type: float32
   :range: [0, 1]
   :default: 0.015
   :location: GUI mode window position

   X coordinate of the inventory companion window.
   Displays companion inventory while interacting.
   Added in Tribunal expansion, available in OpenMW.

.. omw-setting::
   :title: inventory companion window y
   :type: float32
   :range: [0, 1]
   :default: 0.54
   :location: GUI mode window position

   Y coordinate of the inventory companion window.
   Displays companion inventory while interacting.
   Added in Tribunal expansion, available in OpenMW.

.. omw-setting::
   :title: inventory companion window w
   :type: float32
   :range: [0, 1]
   :default: 0.45
   :location: GUI mode window size

   Width of the inventory companion window.
   Displays companion inventory while interacting.
   Added in Tribunal expansion, available in OpenMW.

.. omw-setting::
   :title: inventory companion window h
   :type: float32
   :range: [0, 1]
   :default: 0.38
   :location: GUI mode window size

   Height of the inventory companion window.
   Displays companion inventory while interacting.
   Added in Tribunal expansion, available in OpenMW.

.. omw-setting::
   :title: container window x
   :type: float32
   :range: [0, 1]
   :default: 0.49
   :location: GUI mode window position

   X coordinate of the container window.
   Shows contents of containers.
   Activated by clicking a container, dead body, or during pickpocketing.

.. omw-setting::
   :title: container window y
   :type: float32
   :range: [0, 1]
   :default: 0.54
   :location: GUI mode window position

   Y coordinate of the container window.
   Shows contents of containers.
   Activated by clicking a container, dead body, or during pickpocketing.

.. omw-setting::
   :title: container window w
   :type: float32
   :range: [0, 1]
   :default: 0.39
   :location: GUI mode window size

   Width of the container window.
   Shows contents of containers.
   Activated by clicking a container, dead body, or during pickpocketing.

.. omw-setting::
   :title: container window h
   :type: float32
   :range: [0, 1]
   :default: 0.38
   :location: GUI mode window size

   Height of the container window.
   Shows contents of containers.
   Activated by clicking a container, dead body, or during pickpocketing.

.. omw-setting::
   :title: barter window x
   :type: float32
   :range: [0, 1]
   :default: 0.6
   :location: GUI mode window position

   X coordinate of the barter window.
   Displays goods owned by shopkeeper while bartering.
   Activated by clicking Barter in NPC dialog.

.. omw-setting::
   :title: barter window y
   :type: float32
   :range: [0, 1]
   :default: 0.27
   :location: GUI mode window position

   Y coordinate of the barter window.
   Displays goods owned by shopkeeper while bartering.
   Activated by clicking Barter in NPC dialog.

.. omw-setting::
   :title: barter window w
   :type: float32
   :range: [0, 1]
   :default: 0.38
   :location: GUI mode window size

   Width of the barter window.
   Displays goods owned by shopkeeper while bartering.
   Activated by clicking Barter in NPC dialog.

.. omw-setting::
   :title: barter window h
   :type: float32
   :range: [0, 1]
   :default: 0.63
   :location: GUI mode window size

   Height of the barter window.
   Displays goods owned by shopkeeper while bartering.
   Activated by clicking Barter in NPC dialog.

.. omw-setting::
   :title: companion window x
   :type: float32
   :range: [0, 1]
   :default: 0.6
   :location: GUI mode window position

   X coordinate of the companion window.
   Displays NPC's inventory while interacting with companion.
   Added in Tribunal expansion, available in OpenMW.

.. omw-setting::
   :title: companion window y
   :type: float32
   :range: [0, 1]
   :default: 0.27
   :location: GUI mode window position

   Y coordinate of the companion window.
   Displays NPC's inventory while interacting with companion.
   Added in Tribunal expansion, available in OpenMW.

.. omw-setting::
   :title: companion window w
   :type: float32
   :range: [0, 1]
   :default: 0.38
   :location: GUI mode window size

   Width of the companion window.
   Displays NPC's inventory while interacting with companion.
   Added in Tribunal expansion, available in OpenMW.

.. omw-setting::
   :title: companion window h
   :type: float32
   :range: [0, 1]
   :default: 0.63
   :location: GUI mode window size

   Height of the companion window.
   Displays NPC's inventory while interacting with companion.
   Added in Tribunal expansion, available in OpenMW.

.. omw-setting::
   :title: dialogue window x
   :type: float32
   :range: [0, 1]
   :default: 0.15
   :location: GUI mode window position

   X coordinate of the dialogue window.
   Used for talking with NPCs.
   Activated by clicking an NPC.

.. omw-setting::
   :title: dialogue window y
   :type: float32
   :range: [0, 1]
   :default: 0.5
   :location: GUI mode window position

   Y coordinate of the dialogue window.
   Used for talking with NPCs.
   Activated by clicking an NPC.

.. omw-setting::
   :title: dialogue window w
   :type: float32
   :range: [0, 1]
   :default: 0.7
   :location: GUI mode window size

   Width of the dialogue window.
   Used for talking with NPCs.
   Activated by clicking an NPC.

.. omw-setting::
   :title: dialogue window h
   :type: float32
   :range: [0, 1]
   :default: 0.45
   :location: GUI mode window size

   Height of the dialogue window.
   Used for talking with NPCs.
   Activated by clicking an NPC.

.. omw-setting::
   :title: alchemy window x
   :type: float32
   :range: [0, 1]
   :default: 0.25
   :location: GUI mode window position

   X coordinate of the alchemy window.
   Used for crafting potions.
   Activated by dragging an alchemy tool onto the rag doll.

.. omw-setting::
   :title: alchemy window y
   :type: float32
   :range: [0, 1]
   :location: GUI mode window position

   Y coordinate of the alchemy window.
   Used for crafting potions.
   Activated by dragging an alchemy tool onto the rag doll.

.. omw-setting::
   :title: alchemy window w
   :type: float32
   :range: [0, 1]
   :location: GUI mode window size

   Width of the alchemy window.
   Used for crafting potions.
   Activated by dragging an alchemy tool onto the rag doll.

.. omw-setting::
   :title: alchemy window h
   :type: float32
   :range: [0, 1]
   :location: GUI mode window size

   Height of the alchemy window.
   Used for crafting potions.
   Activated by dragging an alchemy tool onto the rag doll.
