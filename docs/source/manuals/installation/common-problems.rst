###############
Common Problems
###############

ERROR: Unknown fallback name: FontColor_color_header
####################################################

:Symptoms:
	OpenMW crashes at startup with
	``ERROR: Unknown fallback name: FontColor_color_header``
	message at the end of ``openmw.log``, located `here </docs/source/reference/modding/paths>`_.

:Cause:
	The OpenMW `configuration file </docs/source/reference/modding/paths>`_ ``openmw.cfg``
	is severely lacking and missing fallback values
	because "Settings Importer" was not run correctly.

:Fix:
	Re-run "Settings Importer" from the OpenMW launcher.

Installing game files via Steam on macOS: DISK WRITE ERROR
##########################################################

:Symptoms:
	Steam stages the download for Morrowind, but does not proceed.
	The download will read "Paused: DISK WRITE ERROR".

:Cause:
	The OpenMW `configuration file </docs/source/reference/modding/paths>`_ ``openmw.cfg``
	is severely lacking and missing fallback values
	because "Settings Importer" was not run correctly.

:Fix:
	Open appmanifest_22320.acf in your favorite text editor.
	Locate or create an entry under the "StateFlags" entry titled "installdir"
	and give it the value "Morrowind".
	Your file should now look something like this::

		"AppState"
		{
			"appid"         "22320"
			"Universe"              "1"
			"name"          "The Elder Scrolls III: Morrowind"
			"StateFlags"            "4"
			"installdir"            "Morrowind"

			[other entries]
		}

	Restart the Steam client. The download should now proceed.

In-game textures show up as pink
################################

:Symptoms:
    Some textures don't show up and are replaced by pink "filler" textures.

:Cause:
    Textures shipped with Morrowind are compressed with S3TC, a texture compression algorithm that was patented in
    the United States until October 2017. Software drivers and operating system distributions released before that date
    may not include support for this algorithm.

:Fix:
    Upgrade your graphics drivers and/or operating system to the latest version.

Music plays, but only a black screen is shown
#############################################

:Symptoms:
    When launching OpenMW, audio is heard but there is only a black screen.

:Cause:
    This can occur when you did not import the Morrowind.ini file when the launcher asked for it.

:Fix:
    To fix it, you need to delete the `launcher.cfg` file from your configuration path
    ([Path description on Wiki](https://wiki.openmw.org/index.php?title=Paths)), then start the launcher, select your
    Morrowind installation, and when the launcher asks whether the `Morrowind.ini` file should be imported, make sure
    to select "Yes".