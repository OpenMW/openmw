How To Install and Use Mods
###########################

The following is a detailed guide on how to install and enable mods in OpenMW using best practices.

Install
-------

#.	Your mod probably comes in some kind of archive, such as ``.zip``, ``.rar``, ``.7z``, or something along those lines. Unpack this archive into its own folder.
#.	Ensure the structure of this folder is correct.
	#.	Locate the plugin files, ``.esp`` or ``.omwaddon``. The folder containing the plugin files we will call your *data folder*
	#.	Check that all resource folders (``Meshes``, ``Textures``, etc.) containing additional resource files (the actual meshes, textures, etc.) are in the *data folder*.
.. note::
	There may be multiple levels of folders, but the location of the plugins must be the same as the resource folders.
#.	Open your ``openmw.cfg`` file in your preferred plain text editor. It is located as described in https://wiki.openmw.org/index.php?title=Paths and *not* in your OpenMW root directory.
#.	Find or search for ``data=``. This is located very near the bottom of the file. If you are using Morrowind, this first entry should already point to your Morrowind data directory, ``Data Files``; otherwise it will point to your game file, ``.omwgame``.
#.	Create a new line underneath and type: ``data="path/to/your/data folder"`` Remember, the *data folder* is where your mod's plugin files are. The double quotes around this path name are *required*.
#.	Save your ``openmw.cfg`` file.

You have now installed your mod. Any simple replacer mods that only contain resource files such as meshes or textures will now automatically be loaded in the order of their ``data=*`` entry. This is important to note because replacer mods that replace the same resource will overwrite previous ones as you go down the list.

Enable
------

Any mods that have plugin files must be enabled to work.

#.