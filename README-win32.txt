OpenMW - the completely unofficial reimplementation of Morrowind
================================================================

OpenMW is an open source reimplementation of the Morrowind game
engine. For more information, see README.txt or

    http://openmw.snaptoad.com/




Running OpenMW
==============

If you downloaded one of the binary releases, keep on reading. If you
got the source release, read the file COMPILE-win32.txt first, and
come back here when you are done.

There are two binary packages for Windows: one that includes all the
Ogre DLLs, and one doesn't. The only reason to get the non-ogre
version is to save bandwidth when you already have the Ogre SDK.
 
If you have the version with Ogre, you can skip the next section.


Getting Ogre
------------

OpenMW is built and tested against Ogre 1.4.9. You can get it using
this direct link:

http://downloads.sourceforge.net/ogre/OgreSDKSetup1.4.9_CBMingW.exe?modtime=1213925466&big_mirror=1

Or you can get it from: http://ogre3d.org -> Select Download
-> Prebuilt SDK -> Code::Blocks + MinGW

Once you have installed it, copy the following files from
"Ogre/bin/debug/" into the OpenMW directory:

OgreMain_d.dll
Plugin_*.dll
RenderSystem_*.dll
cg.dll
OIS_d.dll


Final configuration
-------------------

The final DLL you need (not included for copyright reasons) is
d3dx9d_30.dll. If you have DirectX installed, you most likely have
d3dx9_30.dll in your \windows\system32 folder. Copy it to the OpenMW
directory and rename it to d3dx9d_30.dll (note the "d" after the "9".)

OpenMW assumes you have the Morrowind data files in c:\Program
Files\Bethesda Softworks\Morrowind\Data Files\ . If this is not the
case, you should edit openmw.ini first.


Running
-------

Just run openmw.exe and enjoy! ;-)

The first time you run OpenMW, you will be asked to set screen
resolution and other graphics settings. To be safe, it's not
recommended to select fullscreen mode on the first run. You can bring
up the dialogue at any time by using the -oc switch.

Move around with WASD or arrow keys, move up and down with left shift
and ctrl, exit with 'q' or escape.

You start in a cell called "Sud". You can change this in openmw.ini or
by specifying a cell name on the command line. Note that if you have a
localized (non-English) version, the cell "Sud" might not exist. I
will solve this issue in a later version.

Write openmw -h on the command line to see a complete list of options.
