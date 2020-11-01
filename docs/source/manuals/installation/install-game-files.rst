##################
Install Game Files
##################

OpenMW is a complete game engine that can either run `Morrowind`_
or original projects created with OpenMW-CS, such as `Example Suite`_.

Morrowind
#########

Running the Morrowind Installation Wizard
=========================================

#.	Launch the OpenMW Launcher
#.	Launch the Installation Wizard

	.. note::
		If you are prompted with an error message stating
		"Could not find the Data Files location,"
		click the "Run Installation Wizard" button.
	.. note::
		If you arrive at the main screen, click the "Settings" tab,
		and then click the "Run Installation Wizard" button.

#.	Follow further instructions below
	to install Morrowind from either a retail CD or an existing installation.

	-	**Morrowind (from retail CD)**

		#.	Make sure that the retail CD is in your computer's CD/DVD drive
			and the Installation Wizard is running.
		#.	On the "Select Installation Method" screen of the Installation Wizard,
			choose "Install Morrowind to a New Location" and click the "Next" button.
		#.	Choose a location to install Morrowind to your hard drive
			(or accept the suggested location) and click the "Next" button.
		#.	Select your preferred language for the installation
			and click the "Next" button
		#.	Select which official expansions (Tribunal or Bloodmoon) should be installed.
			For best results, it is recommended to have both expansions installed.
		#.	Click the "Install" button.

	-	**Morrowind (from existing installation)**

		#.	On the "Select Installation Method" screen of the Installation Wizard,
			choose "Select an existing Morrowind installation" and click the "Next" button
		#.	Select an installation. If nothing is detected, click browse.
		#.	Navigate to the directory containing the file ``Morrowind.esm`` and select that file.

#.	You will be asked if you wish to import settings from ``Morrowind.ini``.
	Select "Import", otherwise OpenMW will not work.
	(You do not need to check the box "Include selected masters and plugins").
#.	The OpenMW launcher window should now open.
	Switch to the "Data Files" tab and check the box to the left of ``Morrowind.esm``.
#.	You are now ready to play!

Installing Morrowind
====================

-----------------
Retail CD and GOG
-----------------

Windows users can run the installer if they haven't already.
By default, both Bethesda's official installer on the retail CD
and the GOG installer install to ``C:\Program Files\Bethesda Softworks\Morrowind``.
You will find ``Morrowind.esm`` there.

Users of other platforms running Wine, will find it at
``~/.wine/drive_c/Program Files/Bethesda Softworks/Morrowind``

-----
Steam
-----

Windows
-------

Windows users can download Morrowind through Steam.
Afterwards, you can point OpenMW to the Steam install location at
``C:\Program Files\Steam\SteamApps\common\Morrowind\Data Files\``
and find ``Morrowind.esm`` there.

macOS
-----

If you are running macOS, you can also download Morrowind through Steam:

#.	Navigate to ``/Users/YOUR_USERNAME_HERE/Library/Application Support/Steam/steamapps/``
#.	Create a file called ``appmanifest_22320.acf``
	(the number is based on its `Steam App ID <https://steamdb.info/app/22320/>`_).
	If using TextEdit,
	make sure that your document is in plain text mode by going to the menu bar
	and choosing "Format" -> "Make Plain Text".
	Also, ensure that it's not named with the extension ``.acf.txt``.
	Add the following into that file::

		"AppState"
		{
			"AppID" "22320"
			"Universe" "1"
			"StateFlags" "1026"
			"installdir" "The Elder Scrolls III - Morrowind"
		}

#.	Launch the Steam client and let it download. You can then find ``Morrowind.esm`` at
	``~/Library/Application Support/Steam/steamapps/common/The Elder Scrolls III - Morrowind/Data Files/``

Linux  
----
Debian/Ubuntu - using "Steam Proton" & "OpenMW launcher". 
----
#. Install Steam from "Ubuntu Software" Center  
#. Enable Proton (basically WINE under the hood). This is done in the Steam client menu drop down. Select, "Steam | Settings" then in the "SteamPlay" section check the box next to "enable steam play for all other titles"  
#. Now Morrowind should be selectable in your game list (as long as you own it). You can install it like any other game, choose to install it and remember the directory path of the location you pick.
#. Once the game files are installed, we can now install the open OpenMW Engine. I used "OpenMW launcher" from "Ubuntu Software" Center this has a wizard to help with the basic setup of OpenMW.  
#. Launch "OpenMW launcher" and follow the setup wizard, when asked, point it at the location you installed Morrowind to, we will be looking for the directory that contains the Morrowing.esm file, for example '/steam library/steamapps/common/Morrowind/Data Files/'.
#. Everything should now be in place, click that big "PLAY" button and fire up OpenMW.

Nb. Bloodmoon.esm needs to be below Tribunal.esm in your datafiles list, if you dont have the right order a red "!" will apear next to the filename in the datafiles section of the OpenMW launcher, just drag bloodmoon below tribunal to fix it.

Wine
----

Users of other platforms running Wine can run Steam within it
and find ``Morrowind.esm`` at
``~/.wine/drive_c/Program Files/Steam/SteamApps/common/Morrowind/Data Files/``.

Example Suite
#############

Example Suite is a demo showing the capabilities of the OpenMW engine.
At this time, it requires Morrowind to be installed to run,
but does not use any assets from it.
In the future, it will be possible to run without installing Morrowind first.

#.	`Install Morrowind <Installing Morrowind_>`_
#.	`Download the latest version <https://github.com/OpenMW/example-suite/releases>`_
#.	Follow the platform-specific instructions in the zip file's ``Installation.md`` file.

