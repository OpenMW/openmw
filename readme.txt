OpenMW: A reimplementation of The Elder Scrolls III: Morrowind

OpenMW is an attempt at recreating the engine for the popular role-playing game
Morrowind by Bethesda Softworks. You need to own and install the original game for OpenMW to work.

Version: 0.16.0
License: GPL (see GPL3.txt for more information)
Website: http://www.openmw.org

Font Licenses:
EBGaramond-Regular.ttf: OFL (see OFL.txt for more information)
VeraMono.ttf: custom (see Bitstream Vera License.txt for more information)



INSTALLATION

Windows:
Run the installer.

Linux:
Ubuntu (and most others)
Download the .deb file and install it in the usual way.

Arch Linux
There's an OpenMW package available in the AUR Repository:
http://aur.archlinux.org/packages.php?ID=21419

OS X:
Open DMG file, copy OpenMW folder anywhere, for example in /Applications

BUILD FROM SOURCE

TODO add description here


THE DATA PATH

The data path tells OpenMW where to find your Morrowind files. From 0.12.0 on OpenMW should be able to
pick up the location of these files on its own, if both Morrowind and OpenMW are installed properly
(installing Morrowind under WINE is considered a proper install).

If that does not work for you, please check if you have any leftover openmw.cfg files from versions earlier than 0.12.0. These can interfere with the configuration process, so try to remove then.

If you are running OpenMW without installing it, you still need to manually adjust the data path. Create a text file named openmw.cfg in the location of the binary and enter the following line:

data=path to your data directory

(where you replace "path to your data directory" with the actual location of your data directory)


COMMAND LINE OPTIONS

Syntax: openmw <options>
Allowed options:
  --help                           print help message
  --version                        print version information and quit
  --data arg (=data)               set data directories (later directories have
                                   higher priority)
  --data-local arg                 set local data directory (highest priority)
  --resources arg (=resources)     set resources directory
  --start arg (=Beshara)           set initial cell
  --master arg                     master file(s)
  --plugin arg                     plugin file(s)
  --anim-verbose [=arg(=1)] (=0)   output animation indices files
  --debug [=arg(=1)] (=0)          debug mode
  --nosound [=arg(=1)] (=0)        disable all sounds
  --script-verbose [=arg(=1)] (=0) verbose script output
  --new-game [=arg(=1)] (=0)       activate char gen/new game mechanics
  --script-all [=arg(=1)] (=0)     compile all scripts (excluding dialogue scri
                                   pts) at startup
  --fs-strict [=arg(=1)] (=0)      strict file system handling (no case folding
                                   )
  --encoding arg (=win1252)        Character encoding used in OpenMW game messa
                                   ges:

                                   win1250 - Central and Eastern European such
                                   as Polish, Czech, Slovak, Hungarian, Slovene
                                   , Bosnian, Croatian, Serbian (Latin script),
                                   Romanian and Albanian languages

                                   win1251 - Cyrillic alphabet such as Russian,
                                   Bulgarian, Serbian Cyrillic and other langua
                                   ges

                                   win1252 - Western European (Latin) alphabet,
                                   used by default
  --fallback arg                   fallback values


CHANGELOG

0.16.0

Bug #250: OpenMW launcher erratic behaviour
Bug #270: Crash because of underwater effect on OS X
Bug #277: Auto-equipping in some cells not working
Bug #294: Container GUI ignores disabled inventory menu
Bug #297: Stats review dialog shows all skills and attribute values as 0
Bug #298: MechanicsManager::buildPlayer does not remove previous bonuses
Bug #299: Crash in World::disable
Bug #306: Non-existent ~/.config/openmw "crash" the launcher.
Bug #307: False "Data Files" location make the launcher "crash"
Feature #81: Spell Window
Feature #85: Alchemy Window
Feature #181: Support for x.y script syntax
Feature #242: Weapon and Spell icons
Feature #254: Ingame settings window
Feature #293: Allow "stacking" game modes
Feature #295: Class creation dialog tooltips
Feature #296: Clicking on the HUD elements should show/hide the respective window
Feature #301: Direction after using a Teleport Door
Feature #303: Allow object selection in the console
Feature #305: Allow the use of = as a synonym for ==
Feature #312: Compensation for slow object access in poorly written Morrowind.esm scripts
Task #176: Restructure enabling/disabling of MW-references
Task #283: Integrate ogre.cfg file in settings file
Task #290: Auto-Close MW-reference related GUI windows

0.15.0

Bug #5: Physics reimplementation (fixes various issues)
Bug #258: Resizing arrow's background is not transparent
Bug #268: Widening the stats window in X direction causes layout problems
Bug #269: Topic pane in dialgoue window is too small for some longer topics
Bug #271: Dialog choices are sorted incorrectly
Bug #281: The single quote character is not rendered on dialog windows
Bug #285: Terrain not handled properly in cells that are not predefined
Bug #289: Dialogue filter isn't doing case smashing/folding for item IDs
Feature #15: Collision with Terrain
Feature #17: Inventory-, Container- and Trade-Windows
Feature #44: Floating Labels above Focussed Objects
Feature #80: Tooltips
Feature #83: Barter Dialogue
Feature #90: Book and Scroll Windows
Feature #156: Item Stacking in Containers
Feature #213: Pulsating lights
Feature #218: Feather & Burden
Feature #256: Implement magic effect bookkeeping
Feature #259: Add missing information to Stats window
Feature #260: Correct case for dialogue topics
Feature #280: GUI texture atlasing
Feature #291: Ability to use GMST strings from GUI layout files
Task #255: Make MWWorld::Environment into a singleton

0.14.0

Bug #1: Meshes rendered with wrong orientation
Bug #6/Task #220: Picking up small objects doesn't always work
Bug #127: tcg doesn't work
Bug #178: Compablity problems with Ogre 1.8.0 RC 1
Bug #211: Wireframe mode (toggleWireframe command) should not apply to Console & other UI
Bug #227: Terrain crashes when moving away from predefined cells
Bug #229: On OS X Launcher cannot launch game if path to binary contains spaces
Bug #235: TGA texture loading problem
Bug #246: wireframe mode does not work in water
Feature #8/#232: Water Rendering
Feature #13: Terrain Rendering
Feature #37: Render Path Grid
Feature #66: Factions
Feature #77: Local Map
Feature #78: Compass/Mini-Map
Feature #97: Render Clothing/Armour
Feature #121: Window Pinning
Feature #205: Auto equip
Feature #217: Contiainer should track changes to its content
Feature #221: NPC Dialogue Window Enhancements
Feature #233: Game settings manager
Feature #240: Spell List and selected spell (no GUI yet)
Feature #243: Draw State
Task #113: Morrowind.ini Importer
Task #215: Refactor the sound code
Task #216: Update MyGUI

0.13.0

Bug #145: Fixed sound problems after cell change
Bug #179: Pressing space in console triggers activation
Bug #186: CMake doesn't use the debug versions of Ogre libraries on Linux
Bug #189: ASCII 16 character added to console on it's activation on Mac OS X
Bug #190: Case Folding fails with music files
Bug #192: Keypresses write Text into Console no matter which gui element is active
Bug #196: Collision shapes out of place
Bug #202: ESMTool doesn't not work with localised ESM files anymore
Bug #203: Torch lights only visible on short distance
Bug #207: Ogre.log not written
Bug #209: Sounds do not play
Bug #210: Ogre crash at Dren plantation
Bug #214: Unsupported file format version
Bug #222: Launcher is writing openmw.cfg file to wrong location
Feature #9: NPC Dialogue Window
Feature #16/42: New sky/weather implementation
Feature #40: Fading
Feature #48: NPC Dialogue System
Feature #117: Equipping Items (backend only, no GUI yet, no rendering of equipped items yet)
Feature #161: Load REC_PGRD records
Feature #195: Wireframe-mode
Feature #198/199: Various sound effects
Feature #206: Allow picking data path from launcher if non is set
Task #108: Refactor window manager class
Task #172: Sound Manager Cleanup
Task #173: Create OpenEngine systems in the appropriate manager classes
Task #184: Adjust MSVC and gcc warning levels
Task #185: RefData rewrite
Task #201: Workaround for transparency issues
Task #208: silenced esm_reader.hpp warning

0.12.0

Bug #154: FPS Drop
Bug #169: Local scripts continue running if associated object is deleted
Bug #174: OpenMW fails to start if the config directory doesn't exist
Bug #187: Missing lighting
Bug #188: Lights without a mesh are not rendered
Bug #191: Taking screenshot causes crash when running installed
Feature #28: Sort out the cell load problem
Feature #31: Allow the player to move away from pre-defined cells
Feature #35: Use alternate storage location for modified object position
Feature #45: NPC animations
Feature #46: Creature Animation
Feature #89: Basic Journal Window
Feature #110: Automatically pick up the path of existing MW-installations
Feature #183: More FPS display settings
Task #19: Refactor engine class
Task #109/Feature #162: Automate Packaging
Task #112: Catch exceptions thrown in input handling functions
Task #128/#168: Cleanup Configuration File Handling
Task #131: NPC Activation doesn't work properly
Task #144: MWRender cleanup
Task #155: cmake cleanup

0.11.1

Bug #2: Resources loading doesn't work outside of bsa files
Bug #3: GUI does not render non-English characters
Bug #7: openmw.cfg location doesn't match
Bug #124: The TCL alias for ToggleCollision is missing.
Bug #125: Some command line options can't be used from a .cfg file
Bug #126: Toggle-type script instructions are less verbose compared with original MW
Bug #130: NPC-Record Loading fails for some NPCs
Bug #167: Launcher sets invalid parameters in ogre config
Feature #10: Journal
Feature #12: Rendering Optimisations
Feature #23: Change Launcher GUI to a tabbed interface
Feature #24: Integrate the OGRE settings window into the launcher
Feature #25: Determine openmw.cfg location (Launcher)
Feature #26: Launcher Profiles
Feature #79: MessageBox
Feature #116: Tab-Completion in Console
Feature #132: --data-local and multiple --data
Feature #143: Non-Rendering Performance-Optimisations
Feature #150: Accessing objects in cells via ID does only work for objects with all lower case IDs
Feature #157: Version Handling
Task #14: Replace tabs with 4 spaces
Task #18: Move components from global namespace into their own namespace
Task #123: refactor header files in components/esm

0.10.0

* NPC dialogue window (not functional yet)
* Collisions with objects
* Refactor the PlayerPos class
* Adjust file locations
* CMake files and test linking for Bullet
* Replace Ogre raycasting test for activation with something more precise
* Adjust player movement according to collision results
* FPS display
* Various Portability Improvements
* Mac OS X support is back!

0.9.0

* Exterior cells loading, unloading and management
* Character Creation GUI
* Character creation
* Make cell names case insensitive when doing internal lookups
* Music player
* NPCs rendering

0.8.0

* GUI
* Complete and working script engine
* In game console
* Sky rendering
* Sound and music
* Tons of smaller stuff

0.7.0

* This release is a complete rewrite in C++.
* All D code has been culled, and all modules have been rewritten.
* The game is now back up to the level of rendering interior cells and moving around, but physics, sound, GUI, and scripting still remain to be ported from the old codebase.

0.6.0

* Coded a GUI system using MyGUI
* Skinned MyGUI to look like Morrowind (work in progress)
* Integrated the Monster script engine
* Rewrote some functions into script code
* Very early MyGUI < > Monster binding
* Fixed Windows sound problems (replaced old openal32.dll)

0.5.0

* Collision detection with Bullet
* Experimental walk & fall character physics
* New key bindings:
  * t toggle physics mode (walking, flying, ghost),
  * n night eye, brightens the scene
* Fixed incompatability with DMD 1.032 and newer compilers
* * (thanks to tomqyp)
* Various minor changes and updates

0.4.0

* Switched from Audiere to OpenAL
* * (BIG thanks to Chris Robinson)
* Added complete Makefile (again) as a alternative build tool
* More realistic lighting (thanks again to Chris Robinson)
* Various localization fixes tested with Russian and French versions
* Temporary workaround for the Unicode issue: invalid UTF displayed as '?'
* Added ns option to disable sound, for debugging
* Various bug fixes
* Cosmetic changes to placate gdc Wall

0.3.0

* Built and tested on Windows XP
* Partial support for FreeBSD (exceptions do not work)
* You no longer have to download Monster separately
* Made an alternative for building without DSSS (but DSSS still works)
* Renamed main program from 'morro' to 'openmw'
* Made the config system more robust
* Added oc switch for showing Ogre config window on startup
* Removed some config files, these are auto generated when missing.
* Separated plugins.cfg into linux and windows versions.
* Updated Makefile and sources for increased portability
* confirmed to work against OIS 1.0.0 (Ubuntu repository package)

0.2.0

* Compiles with gdc
* Switched to DSSS for building D code
* Includes the program esmtool

0.1.0

first release
