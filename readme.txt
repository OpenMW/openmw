OpenMW: A reimplementation of The Elder Scrolls III: Morrowind

OpenMW is an attempt at recreating the engine for the popular role-playing game
Morrowind by Bethesda Softworks. You need to own and install the original game for OpenMW to work.

Version: 0.11.1
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

On Windows you a suitable location for the cfg file is alongside the binary. Currently the binary release comes with such a file pre-generated, but you still need to adjust the data setting.

On Linux and Mac the default location will be ~/.config/openmw/openmw.cfg.

COMMAND LINE OPTIONS
TODO add description of command line options

CREDITS

Developers:
TODO add list of developers

OpenMW:
Thanks to DokterDume for kindly providing us with the Moon and Star logo
used as the application icon and project logo.

Launcher:
Thanks to Kevin Ryan for kindly providing us with the icon used for the Data Files tab.


CHANGELOG

TODO add changelog (take pre 0.11.0 changelog from wiki when it is up again; take 0.11.0 and later changelog from tracker)
