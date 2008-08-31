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


You only need to download either the binary package or the source
code, not both. The DLL pack is needed in both cases. The DLL pack
does not change from version to version unless specified, so you
should only need to download it once.

If you downloaded the binary release, keep reading. If you got the
source release, please read COMPILE-win32.txt first, then return here
for final configuration instructions.


Configuration
-------------------

OpenMW assumes you have the Morrowind data files in c:\Program
Files\Bethesda Softworks\Morrowind\Data Files\ . If this is not the
case, you should edit openmw.ini first.


Running
-------

Just run openmw.exe and enjoy! ;-)

We have just updated the sound system, so it is possible things won't
work. If you get error messages related to sound, try disabling sound
with the -ns option. We are working on flattening out the remaining
bugs.

The first time you run OpenMW, you will be asked to set screen
resolution and other graphics settings. To be safe, it's not
recommended to select fullscreen mode on the first run. You can bring
up the dialogue at any time by using the -oc switch.

Move around with WASD or arrow keys, move up and down with left shift
and ctrl, exit with 'q' or escape.

You start in a cell called "Assu". You can change this in openmw.ini or
by specifying a cell name on the command line.

Write openmw -h on the command line to see a complete list of options.
