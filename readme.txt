OpenMW: A reimplementation of The Elder Scrolls III: Morrowind

OpenMW is an attempt at recreating the engine for the popular role-playing game
Morrowind by Bethesda Softworks. You need to own and install the original game for OpenMW to work.

Version: 0.12.0
License: GPL (see GPL3.txt for more information)
Website: http://www.openmw.org


THIS IS A WORK IN PROGRESS


INSTALLATION

Windows:
Just unpack to a location of your choice. Currently there is no installer.

Linux:
Ubuntu (and most others)
Download the .deb file and install it in the usual way.

Arch Linux
There's an OpenMW package available in the AUR Repository:
http://aur.archlinux.org/packages.php?ID=21419

OS X:
TODO add description for OS X


BUILD FROM SOURCE

TODO add description here


THE DATA PATH

After the installation OpenMW needs to be told where to find the Morrowind data directory. Create a text file named openmw.cfg (location depends on platform) and enter the following line:

data=path to your data directory

(where you replace "path to your data directory" with the actual location of your data directory)

On Windows a suitable location for the cfg file is alongside the binary. Currently the binary release comes with such a file pre-generated, but you still need to adjust the data setting.

On Linux and Mac the default location will be ~/.config/openmw/openmw.cfg.


COMMAND LINE OPTIONS
TODO add description of command line options


CREDITS

Developers:
TODO add list of developers

OpenMW:
Thanks to DokterDume for kindly providing us with the Moon and Star logo used as the application icon and project logo.

Launcher:
Thanks to Kevin Ryan for kindly providing us with the icon used for the Data Files tab.


CHANGELOG

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

TODO add old changelog (take pre 0.11.0 changelog from wiki)
