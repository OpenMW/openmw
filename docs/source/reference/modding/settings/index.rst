###############################
Advanced Settings Configuration
###############################

This part of the guide will cover how to make modifications to the more arcane settings in OpenMW,
most of which are not available from in-game menus, to optimize or customize your OpenMW experience.
If you are familiar with ``.ini`` tweaks in Morrowind or the other games, this will be quite similar.
All settings described in this section are changed in ``settings.cfg``, located in your OpenMW user directory.
See :doc:`../paths` for this location.

When creating a new game based on the OpenMW engine, it may be desirable to change the default settings - the defaults are chosen for compatibility with Morrowind.
This can be done by editing ``defaults.bin`` in the OpenMW installation directory without rebuilding OpenMW itself.
If you're using a custom fork of OpenMW, ``files/settings-default.cfg`` in the source repository should be edited instead.
To edit ``defaults.bin``, base64 decode it, make any changes, and then base64 encode it again.

If you feel a need to edit the default settings for any other reason than when creating a new OpenMW-based game, you should not.
We may be able to accommodate your desired workflow some other way if you make a feature request.

Changing Settings
#################

#.	Once you have located your ``settings.cfg`` file, open it in a plain text editor.
#.	Find the setting(s) you wish to change in the following pages.
#.	If the setting is not already in ``settings.cfg``,
	add it by copy and pasting the name exactly as written in this guide.
#.	Set the value of the setting by typing ``= <value>`` after the setting on the same line,
	using an appropriate value in place of ``<value>``.
#.	If this is the first setting from it's category that you're adding,
	be sure to add the heading in square brackets ``[]`` above it using just the setting type,
	i.e. without the word "Settings".

	For example, to delay tooltips popping up by 1 second, add the line ``tooltip delay = 1.0``.
	Then to the line above, type ``[GUI]``, as the tooltip delay setting comes from the "GUI Settings" section.

Although this guide attempts to be comprehensive and up to date,
you will always be able to find the full list of settings available and their default values in ``settings-default.cfg``,
available in the ``files`` directory of our source repo, or by base64 decoding ``defaults.bin`` in your main OpenMW installation directory.
This has changed compared to previous versions of OpenMW as more users were confused by the existence of a file they weren't supposed to edit
than were helped by the existence of a file listing settings they could edit in a different file.
The ranges included with each setting are the physically possible ranges, not recommendations.

.. warning::
	As the title suggests, these are advanced settings.
	If digging around plain text files and manually editing settings sounds scary to you,
	you may want to steer clear of altering these files. That being said,
	this guide should be plenty clear enough that you can find the setting you want to change and safely edit it.

.. toctree::
	:caption: Table of Contents
	:maxdepth: 2

	camera
	cells
	fog
	groundcover
	map
	GUI
	HUD
	game
	general
	shaders
	shadows
	input
	saves
	sound
	terrain
	video
	water
	windows
	navigator
	physics
	models
