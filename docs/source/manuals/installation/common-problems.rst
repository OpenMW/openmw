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