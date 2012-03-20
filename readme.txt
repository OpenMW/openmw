OpenMW: A reimplementation of The Elder Scrolls III: Morrowind

OpenMW is an attempt at recreating the engine for the popular role-playing game
Morrowind by Bethesda Softworks. You need to own and install the original game for OpenMW to work.

Version: 0.13.0
License: GPL (see GPL3.txt for more information)
Website: http://www.openmw.org


THIS IS A WORK IN PROGRESS


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
  --fps [=arg(=1)] (=0)            fps counter detail (0 = off, 1 = fps counter
                                   , 2 = full detail)
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
  --report-focus [=arg(=1)] (=0)   write name of focussed object to cout


CREDITS

Current Developers:
Alexander “Ace” Olofsson
athile
Cris “Mirceam” Mihalache
gugus / gus
Jacob “Yacoby” Essex
Jannik “scrawl” Heller
Jason “jhooks” Hooks
Karl-Felix “k1ll” Glatzer
Lukasz “lgro” Gromanowski
Marc “Zini” Zinnschlag
Michael “werdanith” Papageorgiou
Nikolay “corristo” Kasyanov
Pieter “pvdk” van der Kloet
Roman "Kromgart" Melnik
Sebastian “swick” Wick

Retired Developers:
Ardekantur
Armin Preiml
Diggory Hardy
Jan Borsodi
Jan-Peter “peppe” Nilsson
Josua Grawitter
Nicolay Korslund
sergoz
Star-Demon
Yuri Krupenin

OpenMW:
Thanks to DokterDume for kindly providing us with the Moon and Star logo used as the application icon and project logo.

Launcher:
Thanks to Kevin Ryan for kindly providing us with the icon used for the Data Files tab.


CHANGELOG

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

TODO add old changelog (take pre 0.11.1 changelog from wiki)
