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




IMPORTANT: Subversion notes
===========================

The subversion code is currently in the process of switching from
Audiere to OpenAL. This means that:

- you need to install OpenAL and ALUT
- you no longer need Audiere
- music does not (currently) work

Generally true for all SVN versions is that:

- a given SVN revision is not guaranteed to work or compile
- windows compilation scripts are unlikely to work since they are
  updated less often
- README and instructions might be out of date

See the changelog at the end for an up-to-date list of changes.

Note: if you are using a localized (non-English) version of Morrowind,
the default starting cell (Sud) might not exist, and the esmtool
program will probably fail with UTF errors. This will be fixed in a
future release - I have even added localized support as one of the
major goals on the web page.




On the immediate TODO list:
===========================

- read the data files in the correct code page
- switch audio to OpenAL
- support for Mac
- collision detection
- displaying creatures correcty, animation
- rendering NPCs
- GUI/HUD system
- rendering outdoor scenes (exterior cells)




Installation
============

Currently supported platforms are Windows and Linux. Tested on Windows
XP and Ubuntu 8.04.

For instructions, see one of the following:

README-win32.txt    - instructions for binary Windows release
COMPILE-win32.txt   - instructions for building source on Windows
COMPILE-linux.tx    - instructions for building source on Linux / Unix

FreeBSD has also been tested but is only partially supported, due to
sketchy D compiler support. It will run, but exceptions do not work
and will immediately abort the program.

Linux 64 bit is known NOT to work, also because of current compiler
deficiencies.




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

- Bastien Jansen for continued testing on 64 bit linux.

- Chris Robinson for OpenAL and MP3 support

- Various others for testing, ideas and patches




Changelog:
==========

0.4 (2008 jul. 20) - work in progress

- switched from Audiere to OpenAL (BIG thanks to Chris Robinson)
- added complete Makefile (again) as a alternative build tool
- much more realistic lighting (thanks again to Chris Robinson)
- should work with Russian version
- various bug-fixes
- cosmetic changes to placate gdc -Wall


0.3 (2008 jul. 10) - latest release

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
