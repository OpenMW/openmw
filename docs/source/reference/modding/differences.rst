Modding OpenMW vs Morrowind
#################################

A brief overview of the differences between the two engines.
============================================================

OpenMW is designed to be able to use all the normal Morrowind mod files such as ESM/ESP plugins, texture replacers,
mesh replacers, etc.

.. warning::
	All external programs and libraries that depend on ``morrowind.exe`` cannot function with OpenMW. 
	This means you should assume mods dependent on Morrowind Graphics Extender, Morrowind Code Patch, 
	Morrowind Script Extender, etc, will *not* work correctly, nor will the tools themselves.

Multiple Data Folders
---------------------

The largest difference between OpenMW and Morrowind in terms of data structure is OpenMW's support of multiple data folders. 
This has many advantages, especially when it comes to uninstalling mods and preventing unintentional overwrites of files.

.. warning::
	Most mods can still be installed into the root OpenMW data folder, but this is not recommended.

To install mods via this new feature:

#.	Open ``openmw.cfg`` with your preferred text editor. It is located as described in :doc:`paths` and *not* in your OpenMW root directory.
#.	Find or search for ``data=``. This is located very near the bottom of the file.
#.	Add a new line below this line and make a new entry of the format ``data="path/to/your/mod"``
#.	Make as many of these entries as you need for each mod folder you want to include.
#.	Save ``openmw.cfg``

.. note::
	All mod folders must adhere to the same file structure as ``~/Morrowind/Data Files/``.

.. TODO create a PATHS ReST file that I can reference instead of the Wiki.

To uninstall these mods simply delete that mod's respective ``data=`` entry.
The mods are loaded in the order of these entries, with the top being overwritten by mods added towards the bottom.

.. note::
	Mods that depend on ESM/ESP plugins can be rearranged within the OpenMW Launcher, 
	but mesh/texture replacer mods can only be reordered by moving their ``data=`` entry.

OpenMW Launcher
---------------

The launcher included with OpenMW is similar to the original Morrowind Launcher. 
Go to the Data Files tab to enable and disable plugins. You can also drag list items to modify the load order. 
Content lists can be created at the bottom by clicking the New Content List button, creating a list name, 
then setting up a new modlist. This is helpful for different player profiles and testing out different load orders.

.. TODO use a substitution image for the New Content List button.

Settings.cfg
------------

The ``settings.cfg`` file is essentially the same as the INI files for Morrowind. 
It is located in the same directory as ``openmw.cfg``. This is where many video, audio, GUI, input, etc. 
settings can be modified. Some are available in-game, but many are only available in this configuration file. 
Please see https://wiki.openmw.org/index.php?title=Settings for the complete listing.

.. TODO Create a proper ReST document tree for all the settings rather than Wiki.

Open Source Resources Support
-----------------------------

While OpenMW supports all of the original files that Morrowind supported, 
we've expanded support to many open source file formats. These are summarized below:

<this will be a table of the type of file, the morrowind supported file, and the OpenMW supported file formats>
