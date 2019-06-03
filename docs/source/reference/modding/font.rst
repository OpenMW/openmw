Fonts
#####

Morrowind .fnt fonts
--------------------

Morrowind uses a custom ``.fnt`` file format. It is not compatible with the Windows Font File ``.fnt`` format.
To our knowledge, the format is undocumented.

OpenMW can load this format and convert it on the fly into something usable 
(see font loader `source code <https://github.com/OpenMW/openmw/blob/master/components/fontloader/fontloader.cpp#L210>`_). 
You can use --export-fonts command line option to write the converted font
(a PNG image and an XML file describing the position of each glyph in the image) to the current directory.

TrueType fonts
--------------

Unlike vanilla Morrowind, OpenMW directly supports TrueType (``.ttf``) fonts.

This is the recommended way to install replacement fonts.

	1.	Download `TrueType fonts for OpenMW <https://www.nexusmods.com/morrowind/mods/46854>`_
	2.	Place the ``Fonts`` folder from archive to the configuration folder. Use :doc:`paths` article to find the folder.

Now Fonts folder should include ``openmw_font.xml`` file and three ``.ttf`` files.

If desired, you can now delete the ``Data Files/Fonts`` directory.

It is also possible to adjust the font size and resolution::

			[GUI]
			font size = 16
			ttf resolution = 96

The ``font size`` setting accepts clamped values in range from 12 to 20 while ``ttf resolution`` setting accepts values from 48 to 960.

Any Resolution or Size properties in the XML file have no effect because the engine settings override them.

The engine automatically takes UI scaling factor into account, so don't account for it when tweaking the settings.

Bitmap fonts
------------

Morrowind ``.fnt`` files are essentially a bitmap font, but using them is discouraged because they don't have Unicode support. 
MyGUI has its own format for bitmap fonts. An example can be seen by using the --export-fonts command line option (see above), 
which converts Morrowind ``.fnt`` to a MyGUI bitmap font. 
This is the recommended format to use if you wish to edit Morrowind's bitmap font or create a new bitmap font.
