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

New Editor Features:
- ?

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
- Verifier no longer checks for alleged 'race' entries in clothing body parts (#5400)

Miscellaneous:
- Prevent save-game bloating by using an appropriate fog texture format (#5108)
- Ensure that 'Enchantment autocalc" flag is treated as flag in OpenMW-CS and in our esm tools (#5363)