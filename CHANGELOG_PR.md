*** PLEASE PUT YOUR ISSUE DESCRIPTION FOR DUMMIES HERE FOR REVIEW ***

- I'm just a placeholder description (#1337)
- I'm also just a placeholder description, but I'm a more recent one (#42)

***

0.47.0
------

The OpenMW team is proud to announce the release of version 0.47.0! Grab it from our Downloads Page for all operating systems. ***short summary: XXX ***

Check out the release video (***add link***) and the OpenMW-CS release video (***add link***) by the ***add flattering adjective*** Atahualpa, and see below for the full list of changes.

Known Issues:
- To use generic Linux binaries, Qt4 and libpng12 must be installed on your system
- On macOS, launching OpenMW from OpenMW-CS requires OpenMW.app and OpenMW-CS.app to be siblings

New Features:
- Dialogue to split item stacks now displays the name of the trapped soul for stacks of soul gems (#5362)
- Basics of Collada animations are now supported via osgAnimation plugin (#5456)

New Editor Features:
- Instance selection modes are now implemented (centred cube, corner-dragged cube, sphere) with four user-configurable actions (select only, add to selection, remove from selection, invert selection) (#3171)

Bug Fixes:
- NiParticleColorModifier in NIF files is now properly handled which solves issues regarding particle effects, e.g., smoke and fire (#1952, #3676)
- Targetting non-unique actors in scripts is now supported (#2311)
- Guards no longer ignore attacks of invisible players but rather initiate dialogue and flee if the player resists being arrested (#4774)
- Changing the dialogue window without closing it no longer clears the dialogue history in order to allow, e.g., emulation of three-way dialogue via ForceGreeting (#5358)
- Scripts which try to start a non-existent global script now skip that step and continue execution instead of breaking (#5364)
- Selecting already equipped spells or magic items via hotkey no longer triggers the equip sound to play (#5367)
- 'Scale' argument in levelled creature lists is now taken into account when spawning creatures from such lists (#5369)
- Morrowind legacy madness: Using a key on a trapped door/container now only disarms the trap if the door/container is locked (#5370)

Editor Bug Fixes:
- Deleted and moved objects within a cell are now saved properly (#832)
- Disabled record sorting in Topic and Journal Info tables, implemented drag-move for records (#4357)
- Topic and Journal Info records can now be cloned with a different parent Topic/Journal Id (#4363)
- Verifier no longer checks for alleged 'race' entries in clothing body parts (#5400)
- Cell borders are now properly redrawn when undoing/redoing terrain changes (#5473)
- Loading mods now keeps the master index (#5675)
- Flicker and crashing on XFCE4 fixed (#5703)
- Collada models render properly in the Editor (#5713)
- Terrain-selection grid is now properly updated when undoing/redoing terrain changes (#6022)
- Tool outline and select/edit actions in "Terrain land editing" mode now ignore references (#6023)
- Primary-select and secondary-select actions in "Terrain land editing" mode now behave like in "Instance editing" mode (#6024)
- Using the circle brush to select terrain in the "Terrain land editing" mode no longer selects vertices outside the circle (#6035)
- Vertices at the NW and SE corners of a cell can now also be selected in "Terrain land editing" mode if the adjacent cells aren't loaded yet (#6036)

Miscellaneous:
- Prevent save-game bloating by using an appropriate fog texture format (#5108)
- Ensure that 'Enchantment autocalc" flag is treated as flag in OpenMW-CS and in our esm tools (#5363)
