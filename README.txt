OpenMW - the completely unofficial reimplementation of Morrowind
================================================================

Written by Nicolay Korslund
Email: korslund@gmail.com
WWW: http://openmw.snaptoad.com
License: See GPL3.txt
Current version: 0.3 (still very pre-alpha)
Date: 2008 jul. 6




QUICK NOTE: You must own and install Morrowind before you can use
OpenMW. Let me repeat that: OpenMW will NOT run if you do not have
Morrowind installed on your system!




Release notes for 0.3
=====================

This release adds support for building and running on Windows. As a
result, the installation instructions have been split into the files
INSTALL-win32.txt and INSTALL-linux.txt. These files cover both the
binary packages and installation from source.

See also the changelog at the end.

Note: if you are using a localized (non-English) version of Morrowind,
the default starting cell (Sud) might not exist, and the esmtool will
probably fail with UTF errors, since the system currently expects UTF8
input. This will be fixed in a future release - I have even added
localized support as one of the major goals on the web page.





On the immediate TODO list:
===========================

- read the input files in the correct code page
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

For instructions, see the files INSTALL-win32.txt or
INSTALL-linux.txt.

FreeBSD has also been tested but is only partially supported, due to
sketchy D compiler support. Linux 64 bit currently does NOT work, for
the same reason.




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

- The NifTools group / NIFLA for their great work on decoding the NIF
  file format.

- Dmitry Marakasov for testing and porting to FreeBSD.

- Nebelmann for continued testing on 64 bit linux.

- Bethesda Softworks for creating Morrowind!




Changelog:
==========

0.3 (work in progress)
- built and tested on Windows XP
- partial support for FreeBSD (exceptions do not work)
- removed some config files, since these are auto-generated when
  missing
- made the config system more robust (an Ogre config windows opened
  when needed)
- alternatively reads config file from ~/.openmw/ on unix systems.
- updated Makefile and sources for increased portability (thanks to
  Dmitry Marakasov.)

0.2 (2008 jun. 17) - latest release
- compiles with gdc
- switched to DSSS for building D code
- includes the program esmtool

0.1 (2008 jun. 03)
- first release
