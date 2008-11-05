OpenMW - the completely unofficial reimplementation of Morrowind
================================================================

Written by Nicolay Korslund
Email: korslund@gmail.com
WWW: http://openmw.snaptoad.com
License: See GPL3.txt
Current version: 0.5
Date: 2008 nov. 5


QUICK NOTE: You must own and install Morrowind before you can use
OpenMW. Let me repeat that: OpenMW will NOT run if you do not have
Morrowind installed on your system!



Installation
============

Currently supported platforms are Windows, Linux and FreeBSD. Most
testing is done on Ubuntu 8.04 and Windows XP Professional.

For instructions, see one of the following:

README-win32.txt    - instructions for binary Windows release
COMPILE-win32.txt   - instructions for building from source on Windows
COMPILE-linux.tx    - instructions for building from source on Linux / Unix

Linux 64 does NOT work, because of problems with the D compiler. We
hope to sort this out at some point, but right now it's not supported.



Programs included in this package:
==================================

openmw      - The main program. Run openmw -h for a list of options.

esmtool     - Used to inspect ES files (ESM, ESP, ESS). Run without
              arguments to get a list of options.

bsatool     - Tool for viewing and extracting files from BSA archives.
              (Can also be used to test the NIF parser on a BSA.)

niftool     - Decodes one or more NIF files and prints the details.



Acknowledgements
================

Thanks go out to:

- Bethesda Softworks for creating Morrowind!

- The NifTools group / NIFLA for their great work on decoding the NIF
  file format.

- Dmitry Marakasov for testing and porting to FreeBSD.
- Bastien Jansen for testing on 64 bit linux.
- Chris Robinson for OpenAL and MP3 support
- Jacob Essex for landscape code (still in progress)
- Many others for testing, ideas and patches



Changelog:
==========

0.5 (2008 nov. 5) - latest release

- Collision detection with Bullet
- Experimental walk & fall character physics
- New key bindings:
  t - toggle physics mode (walking, flying, ghost)
  n - night-eye: brightens the scene
- Fixed incompatability with DMD 1.032 and newer
- Various minor changes and updates


0.4 (2008 aug. 30)

- switched from Audiere to OpenAL (BIG thanks to Chris Robinson)
- added complete Makefile (again) as a alternative build tool
- more realistic lighting (thanks again to Chris Robinson)
- various localization fixes - tested with Russian and French versions
- temporary workaround for the Unicode issue: invalid UTF displayed as '?'
- added -ns option to disable sound, for debugging
- various bug-fixes
- cosmetic changes to placate gdc -Wall


0.3 (2008 jul. 10)

- built and tested on Windows XP
- partial support for FreeBSD (exceptions do not work)
- temporarily dropped DSSS and Monster as necessary dependencies
- renamed main program from 'morro' to 'openmw'
- made the config system more robust
- added -oc switch for showing Ogre config window on startup
- removed some config files, these are auto-generated when
  missing. Separated plugins.cfg into linux and windows versions.
- updated Makefile and sources for increased portability (thanks to
  Dmitry Marakasov.)
- tested against OIS 1.0.0 (Ubuntu repository package)


0.2 (2008 jun. 17)

- compiles with gdc
- switched to DSSS for building D code
- includes the program esmtool


0.1 (2008 jun. 03)

- first release
