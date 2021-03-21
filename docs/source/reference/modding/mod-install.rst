How To Install and Use Mods
###########################

The following is a detailed guide on how to install and enable mods in OpenMW using best practices.

Install
-------

#.	Your mod probably comes in some kind of archive, such as ``.zip``, ``.rar``, ``.7z``, or something along those lines. Unpack this archive into its own folder.
#.	Ensure the structure of this folder is correct.

	#.	Locate the plugin files, ``.esp`` or ``.omwaddon``, or possibly ``.esm``. The folder containing the plugin files we will call your *data folder*
	#.	Check that all resource folders (``Meshes``, ``Textures``, etc.) containing additional resource files (the actual meshes, textures, etc.) are in the *data folder*.
	#.	Note that not all mods have a plugin, and not all mods have resources, but they must at minimum have one or the other.
	
	.. note::
		There may be multiple levels of folders, but the location of the plugins must be the same as the resource folders.

#.	Open your ``openmw.cfg`` file in your preferred plain text editor. It is located as described in :doc:`paths` and *not* in your OpenMW root directory.
#.	Find or search for ``data=``. This is located very near the bottom of the file. If you are using Morrowind, this first entry should already point to your Morrowind data directory, ``Data Files``; otherwise it will point to your game file, ``.omwgame``.
#.	Create a new line underneath and type: ``data="path/to/your/data folder"`` Remember, the *data folder* is where your mod's plugin files are. The double quotes around this path name are *required*.
#.	If your mod contains resources in a ``.bsa`` file, go to near the top of the file, locate the entries like ''fallback-archive=Morrowind.bsa'' and create a new line underneath and type: ``fallback-archive=<name of your bsa>.bsa''``.

.. note::
	Some text editors, such as TextEdit on Mac, will auto-correct your double quotes to typographical "curly"
	quotes instead of leaving them as the proper neutral vertical quotes ``""``.

#.	Save your ``openmw.cfg`` file.

You have now installed your mod. Any simple replacer mods that only contain resource files such as meshes or 
textures will now automatically be loaded in the order of their ``data=*`` entry. 
This is important to note because replacer mods that replace the same resource will overwrite previous ones as you go down the list.

Enable
------

Any mods that have plugin files must be enabled to work. 
Master game files and plugin files can only be enabled if they have been properly installed within a *data folder* as described above.

#.	Open the OpenMW Launcher.
#.	Click on the Data Files tab.
#.	In the Content List box, select the content list you wish to modify in the dropdown menu, or make a new one by:

	#.	Click the New Content List button and enter the name of your content list, then click OK. New lists are useful for keeping track of the mods used for different characters or for different games if you play more than one game using OpenMW.
	#.	In the Content box, select your game file (``.esm`` or ``.omwgame``) from the dropdown menu.
	
#.	Now you must activate the plugins you wish to use by checking the box next to their entry in the Content box list.
#.	Load order can be changed simply by dragging the entries around within the list. Mods are loaded from the top down, so if one plugin depends on another, it must be lower on the list.
#.	Click Play to run OpenMW with your game and enabled mods!
