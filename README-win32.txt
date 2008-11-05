OpenMW - the completely unofficial reimplementation of Morrowind
================================================================

OpenMW is an open source reimplementation of the Morrowind game
engine. For more information, see README.txt or

    http://openmw.snaptoad.com/




Running OpenMW
==============

OpenMW consists of three separate downloads:

openmw-0.X_win32.zip     - binary package (with EXE file)
openmw-0.X.zip           - source code
openmw-dll-pack.zip      - library pack


You only need the binary or the source package, not both. The DLL pack
is needed in both cases.

NOTE: If downloaded the SOURCE release, please read COMPILE-win32.txt
before reading the rest of this file.


Configuration
-------------------

OpenMW assumes you have the Morrowind data files in c:\Program
Files\Bethesda Softworks\Morrowind\Data Files\ . If this is not where
you have installed Morrowind, you should edit the file openmw.ini.


Running
-------

Just run openmw.exe and enjoy! ;-)

The first time you run OpenMW, you will be asked to set screen
resolution and other graphics settings. To be safe, I don't recommend
setting fullscreen mode on the first run. You can bring up the
dialogue at any time by using the -oc switch.

Move around with WASD or arrow keys, change physics mode (walking,
flying, ghost) with 't', exit with 'q' or escape.

You start in a cell called "Assu". You can change this in openmw.ini or
by specifying a cell name on the command line.

Write openmw -h on the command line to see a complete list of options.
