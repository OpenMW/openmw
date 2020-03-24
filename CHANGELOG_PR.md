*** PLEASE PUT YOUR ISSUE DESCRIPTION FOR DUMMIES HERE FOR REVIEW ***

- I'm just a placeholder description (#1337)
- I'm also just a placeholder description, but I'm a more recent one (#42)

***

0.46.0
------

The OpenMW team is proud to announce the release of version 0.46.0! Grab it from our Downloads Page for all operating systems. ***short summary: shadows, recastnavigation, etc.***

Check out the release video (***add link***) and the OpenMW-CS release video (***add link***) by the ***add flattering adjective*** Atahualpa, and see below for the full list of changes.

Known Issues:
- There's currently no way to redirect the logging output to the command prompt on Windows Release builds -- this will be resolved in version 0.46.0
- To use generic Linux binaries, Qt4 and libpng12 must be installed on your system
- On macOS, launching OpenMW from OpenMW-CS requires OpenMW.app and OpenMW-CS.app to be siblings

New Features:
- NIF files which contain an "AvoidNode" are ignored by the pathfinding algorithm (#1724)
- Navmeshes are used for AI pathfinding which should resolve most related issues (#2229)
- Movement input from gamepad joysticks is transformed into analogue values (#3025)
- Sane default values for openmw.cfg file to overcome the original morrowind.ini file (#3442)
- Option to invert x-axis for controllers (#3610)
- Local variables of objects selected in the console can now be directly read and set without explicitly stating the object (#3893)
- In-game option to enable or disable controllers (#3980)
- Sneak mode can be toggled using a controller (#4001)
- Controllers use original engine's default key bindings (#4360)
- Support for sheathing animations, including weapon holstering, scabbards (except for throwing weapons), and quivers for projectiles (#4673)
- Support for "NiRollController" in NIF files to ensure correct rotation of models in "Weapon Sheathing" mod (#4675)
- Support for native animated containers (#4730)
- Support for VAO ("Vertex Array Objects") from OSG 3.5.6 or later (#4756)
- Support for "NiSwitchNode" in NIF files to allow future implementation of native support for extended features like harvestable plants or glowing - windows (#4812)
- Native support for glowing windows (and other daytime-dependent meshes) by adding internal day-night-mode switch (#4836)
- Shadows (#4851)
- More configuration options for in-game water reflections (#4859)
- Command line option to specify a random seed to be used by the game's random-number generator ("RNG") for debugging purposes (#4887)
- Configuration options for distant terrain to adjust quality and performance impact (#4890)

New Editor Features:
- "Faction Ranks" table for "Faction" records (#4209)
- Changes to height editing can be cancelled without changes to data (press esc to cancel) (#4840)
- Land heightmap/shape editing and vertex selection (#5170)
- Deleting instances with a keypress (#5172)
- Dropping objects with keyboard shortcuts (#5274)

Bug Fixes:
- The Mouse Wheel can now be used for key bindings (#2679)
- Scripted Items cannot be stacked anymore to avoid multiple script execution (#2969)
- Stray text after an "else" statement is now ignored, like in the original engine, to handle mods which erroneously use "else if" statements (#3006)
- "SetPos" and "SetPosition" commands now more closely replicate the original engine's behaviour (#3109)
- "Reserved keys [F3], [F4], [F10], and [F11] cannot be assigned to in-game controls anymore (#3282)
- Windows: Reserved [Windows] key cannot be assigned to in-game controls anymore (#3282)"
- Windows: Windows-internal display scaling no longer breaks main menu (#3623)
- Normal maps on mirrored UVs are no longer inverted (#3733)
- Teleporting attempts are now also detected if teleporting is disabled to ensure compatibility with certain mods (#3765)
- Throwing weapons are now correctly rotated during throwing animation when using the "Improved Thrown Weapon Projectiles" mod (#3778)
- Birthsign abilities are no longer restored upon loading to ensure mod compatibility (#4329)
- Player character's model is no longer scaled in first-person mode to prevent issues with arrows obscuring the crosshair (#4383)
- Optional: Ranged attacks now bypass normal weapon resistance or weakness if ammunition and/or bow are appropriate (#4384)
- Fall damage is now also applied when first reloading a savegame and when your character is near the ground in the loaded game (#4411)
- Rain drops are no longer delayed when your character emerges from water (#4540)
- ESM record for prison markers is now hardcoded like, e.g., door markers or temple markers (#4701)
- Loading a savegame which includes active messages no longer crashes the game (#4714)
- An empty pointer actor no longer throws an exception upon exiting the dialogue menu (#4715)
- Inventory paper doll no longer simultaneously displays shield and two-handed weapon during drawing and holstering animations (#4720)
- "Reset actors" command ("ra") no longer tries to reset actors originating from inactive cells, e.g., followers (#4723)
- "Reset actors" command ("ra")  now traces reset actors to the ground and also resets fall damage (#4723)"
- Land texture records can now be overwritten by content files to create mods like "Winter in Morrowind" (#4736)
- Disabling collision no longer forces your character to walking speed, but also allows them to run or sneak (#4746)
- NPCs now also use the skeleton associated with their specified model, not only the animations (#4747)
- Sneaking und swimming idle animations are no longer interrupted if your character is in attack-ready state in first-person view (#4750)
- Numerical fallback values with invalid values (e.g., stray text) in the openmw.cfg file no longer crash or break the game (#4768)
- Character's "jumping" flag is no longer unnecessarily reset to ensure compatibility with certain mods, e.g., "Sotha Sil Expanded" (#4775)
- Calling "GetSpellEffects", "GetEffect", or "GetSpell" function on non-actor objects now returns 0, fixing issues with "Sotha Sil Expanded" - (#4778)
- AI values for actors without AIDT ("AI Data") subrecord are now set to zero upon loading instead of filling in "random" values (#4778)
- Running and sneaking are now also considered in in-game checks when your character is in midair, fixing an issue with the "Reign of Fire" mod - (#4797)
- Collision checks are now immediately updated when an object is moved to ensure compatibility with "Sotha Sil Expanded" (#4800)
- Stray special characters before the "begin" statement of a script are now ignored to ensure, once again, compatibility with "Sotha Sil Expanded" - (#4803)
- Particle nodes with an empty "sizes" array are now correctly loaded and no longer cause an exception (#4804)
- Handling of "root bone" and "bip01" nodes in NIF files now matches the original engine's behaviour to ensure compatibility with "Skyrim: Home of - the Nords" (#4810)
- Creatures without specified sound files now fallback to the sounds of the first creature sharing the same model (#4813)
- "Journal" command now also closes a quest when the specified "finish quest" entry has a lower value than the current one for that quest (#4815)
- Spell effects are no longer applied when a spell is successfully absorbed (#4820)
- World state is no longer updated for every in-game hour your character is in jail but only once, which should significantly reduce loading times - (#4823)
- "NiUVController" in NIF files now only affects textures which use the specified "UV Set" index, usually 0; ensures compatibility with "Glow in the Dahrk" (#4827)
- Visual effects ("VFX") for magic effects are now played immediately after the effect is triggered to not accidentally skip the VFX, e.g., when actors drink potions in battle (#4828)
- Meshes with "NiLODNode" or "NiSwitchNode" no longer cause crashes when they contain particles (#4837)
- Localisations can now make use of implicit keywords to create hyperlinks in dialogue text (#4841)
- Actors outside of the processing range no longer appear for one frame when they are spawned (#4860)
- Stray text after a local-variable declaration is now ignored to ensure mod compatibility (#4867)
- Range and default values of AI data fields now match the original engine's ones (#4876)
- "Startup" scripts are now always run once upon starting OpenMW (#4877)
- Stray explicit reference calls for global variables are now ignored to ensure mod compatibility, e.g., with "Sotha Sil Expanded" (#4888)
- Title screen music now loops (#4896)
- "Specular power" is no longer hardcoded but uses the specified value in the shader code (#4916)
- Werewolves can now also attack if their transformation happened during an attack move (#4922)
- Plug-ins with valid empty subrecords are now correctly loaded, which fixes issues with the "DC - Return of Great House Dagoth" mod (#4938)
- Hand-to-hand attacks are now movement-based when the "always use best attack" option is turned off, like in the original engine (#4942)

Editor Bug Fixes:
- Certain numerical fields now only accept unsigned 8-bit integers to avoid overflows (#2987)
- Preview option is now disabled for levelled lists (#4703)
- Opening the "Scene" view from the "Instances" table now also works for exterior cells (#4705)
- Colour fields in interior-cell records now also use the colour picker widget (#4745)
- Cloned, added, or moved instances no longer disappear at load-time (#4748)
- "Clear" function in the content selector no longer tries to execute a "Remove" action on an empty file list (#4757)
- Terrain texture editing for plugins now correctly handles drags from base file (#4904)
- Engine no longer tries to swap buffers of windows which weren't exposed to Qt's window management system (#4911)
- Minimap doesn't get corrupted, when editing new omwgame (#5177)

Miscellaneous:
- Upgraded to FFMPEG3 for media decoding (#4686)
- Optimised terrain code to drastically increase performance with distant terrain enabled (#4695)
- Windows: Added support for NMake to the prebuild script (#4721)
