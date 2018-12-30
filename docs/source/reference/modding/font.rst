Fonts
#####

Morrowind .fnt fonts
--------------------

Morrowind uses a custom ``.fnt`` file format. It is not compatible with the Windows Font File ``.fnt`` format.
To our knowledge, the format is undocumented.

OpenMW can load this format and convert it on the fly into something usable 
(see font loader `source code <https://github.com/OpenMW/openmw/blob/master/components/fontloader/fontloader.cpp#L210>`_). 
In OpenMW 0.32, an --export-fonts command line option was added to write the converted font 
(a PNG image and an XML file describing the position of each glyph in the image) to the current directory.

TrueType fonts
--------------

Unlike vanilla Morrowind, OpenMW directly supports TrueType (``.ttf``) fonts.

0.45.0+ way
-----------
This is the recommended way to install replacement fonts.

-	To replace the primary "Magic Cards" font:

	#.	Download `Pelagiad <https://isaskar.github.io/Pelagiad/>`_ by Isak Larborn (aka Isaskar).
	#.	Create ``Fonts`` folder at the location of your ``openmw.cfg``.
	#.	Copy ``openmw_font.xml`` and ``Pelagiad.ttf`` files into the folder.
	#.	If desired, you can now delete the original ``Magic_Cards.*`` files from your ``Data Files/Fonts`` directory.
	#.	Pelagiad glyphs may appear to be pretty small, so open ``openmw.cfg`` and adjust the following settings as you deem necessary::

			[GUI]
			font size = 17
			ttf resolution = 96

-	You can also replace the Daedric font:

	#.	Download `Ayembedt <https://github.com/georgd/OpenMW-Fonts>`_ by Georg Duffner.
	#.	Install ``OMWAyembedt.otf`` into the ``Fonts`` folder.
	#.	Add the following lines into ``openmw_font.xml``::

			<Resource type="ResourceTrueTypeFont" name="Daedric">
				<Property key="Source" value="OMWAyembedt.otf"/>
				<Property key="Antialias" value="false"/>
				<Property key="TabWidth" value="8"/>
				<Property key="OffsetHeight" value="0"/>
				<Codes>
					<Code range="32"/>
					<Code range="65 90"/>
					<Code range="97 122"/>
				</Codes>
			</Resource>

	#.	This font is missing a few glyphs (mostly punctuation), but it has all the primary glyphs. If desired, you can now delete the original ``daedric.*`` files from your ``Data Files/Fonts`` directory.

Any Resolution or Size properties in the file have no effect because the engine settings override them.

The engine automatically takes UI scaling factor into account, so don't account for it when tweaking the settings.

Pre-0.45.0 way
--------------

-	To replace the primary "Magic Cards" font:

	#.	Download `Pelagiad <https://isaskar.github.io/Pelagiad/>`_ by Isak Larborn (aka Isaskar).
	#.	Install the ``openmw_font.xml`` file into ``resources/mygui/openmw_font.xml`` in your OpenMW installation.
	#.	Copy ``Pelagiad.ttf`` into ``resources/mygui/`` as well.
	#.	If desired, you can now delete the original ``Magic_Cards.*`` files from your ``Data Files/Fonts`` directory.
-	You can also replace the Daedric font:

	#.	Download `Ayembedt <https://github.com/georgd/OpenMW-Fonts>`_ by Georg Duffner.
	#.	Install ``OMWAyembedt.otf`` into ``/resources/mygui/`` folder in your OpenMW installation.
	#.	Add the following lines to ``openmw_font.xml``::

			<Resource type="ResourceTrueTypeFont" name="Daedric">
				<Property key="Source" value="OMWAyembedt.otf"/>
				<Property key="Size" value="24"/>
				<Property key="Resolution" value="50"/>
				<Property key="Antialias" value="false"/>
				<Property key="TabWidth" value="8"/>
				<Property key="OffsetHeight" value="0"/>
				<Codes>
					<Code range="32"/>
					<Code range="65 90"/>
					<Code range="97 122"/>
				</Codes>
			</Resource>

	#.	This font is missing a few glyphs (mostly punctuation), but it has all the primary glyphs. If desired, you can now delete the original ``daedric.*`` files from your ``Data Files/Fonts`` directory.

-	Another replacement for the Daedric font is `Oblivion <http://www.uesp.net/wiki/File:Obliviontt.zip>`_ by Dongle.

	#.	Install the ``Oblivion.ttf`` file into ``resources/mygui/``.
	#.	The ``openmw_font.xml`` entry is::

			<Resource type="ResourceTrueTypeFont" name="Daedric">
				<Property key="Source" value="Oblivion.ttf"/>
				<Property key="Size" value="30"/>
				<Property key="Resolution" value="50"/>
				<Property key="Antialias" value="false"/>
				<Property key="TabWidth" value="8"/>
				<Property key="OffsetHeight" value="0"/>
				<Codes>
					<Code range="32 34"/>
					<Code range="39"/>
					<Code range="44 46"/>
					<Code range="48 59"/>
					<Code range="63"/>
					<Code range="65 90"/>
					<Code range="97 122"/>
					<Code range="172 173"/>
					<Code range="255"/>
					<Code range="376"/>
					<Code range="894"/>
					<Code range="8211 8212"/>
					<Code range="8216 8217"/>
					<Code range="8220 8221"/>
				</Codes>
			</Resource>

Bitmap fonts
------------

Morrowind ``.fnt`` files are essentially a bitmap font, but using them is discouraged because they don't have Unicode support. 
MyGUI has its own format for bitmap fonts. An example can be seen by using the --export-fonts command line option (see above), 
which converts Morrowind ``.fnt`` to a MyGUI bitmap font. 
This is the recommended format to use if you wish to edit Morrowind's bitmap font or create a new bitmap font.
