OpenMW - the completely unofficial reimplementation of Morrowind
================================================================

Written by Nicolay Korslund
Email: korslund@gmail.com
WWW: http://openmw.snaptoad.com
License: See GPL3.txt
Current version: 0.4 (still very pre-alpha)
Date: 2008 jul. 20




QUICK NOTE: You must own and install Morrowind before you can use
OpenMW. Let me repeat that: OpenMW will NOT run if you do not have
Morrowind installed on your system!




IMPORTANT: OpenMW 0.4 notes
===========================

As of OpenMW 0.4, we have switched sound libraries from Audiere to
OpenAL + libavcodec. This means that:

- you need to install OpenAL
- you must install libavcodec / ffmpeg for mp3 playback
- you no longer need Audiere

See the changelog at the end for an up-to-date list of changes.

Also new in this release is the way files are distributed for
Windows. There is now a separate "DLL pack" that can be used with
either the binary version or the source version. You no longer need to
run around the internet to find various SDKs and such - everything
should be included in the DLL-pack, including the necessary C/C++
header files.

Note: if you are using a localized (non-English) version of Morrowind,
the default starting cell (Assu) might not exist. Try esmtool (see
below) to get a list of existing cells.




On the near-future TODO list:
===========================

- full support for localized versions (with character recoding)
- support for Mac
- collision detection + walking & fall physics
- displaying creatures correcty, animation
- rendering NPCs
- rendering outdoor scenes (exterior cells)
- choosing a GUI/HUD system that playes well with OGRE




Installation
============

Currently supported platforms are Windows, Linux and FreeBSD. Most
testing has been on Windows XP and Ubuntu 8.04.

For instructions, see one of the following:

README-win32.txt    - instructions for binary Windows release
COMPILE-win32.txt   - instructions for building source on Windows
COMPILE-linux.tx    - instructions for building source on Linux / Unix

Linux 64 bit is known NOT to work, because of compiler deficiencies.




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

Thanks goes out to:

- Bethesda Softworks for creating Morrowind!

- The NifTools group / NIFLA for their great work on decoding the NIF
  file format.

- Dmitry Marakasov for testing and porting to FreeBSD.

- Bastien Jansen for testing on 64 bit linux.

- Chris Robinson for OpenAL and MP3 support

- Various others for testing, ideas and patches




Changelog:
==========

0.4 (2008 aug. 30) - latest release

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
