Fonts
#####

Default UI font and font used in magic scrolls are defined in ``openmw.cfg``:

			fallback=Fonts_Font_0,MysticCards
			fallback=Fonts_Font_2,DemonicLetters

When there are no ``Fonts_Font_*`` lines in user's ``openmw.cfg``, built-in TrueType fonts are used.
Font used by console and another debug windows is not configurable (so ``Fonts_Font_1`` is unused).

Morrowind .fnt fonts
--------------------

Morrowind uses a custom ``.fnt`` file format. It is not compatible with the Windows Font File ``.fnt`` format.
To our knowledge, the format is undocumented. OpenMW can load this format and convert it on the fly into something usable
(see font loader `source code <https://gitlab.com/OpenMW/openmw/blob/master/components/fontloader/fontloader.cpp>`_).
You can use --export-fonts command line option to write the converted font
(a PNG image and an XML file describing the position of each glyph in the image) to the current directory.

They can be used instead of TrueType fonts if needed by specifying their ``.fnt`` files names in the ``openmw.cfg``. For example:

			fallback=Fonts_Font_0,magic_cards_regular
			fallback=Fonts_Font_2,daedric_font

In this example OpenMW will search for ``magic_cards_regular.fnt`` and ``daedric_font.fnt`` in the ``Fonts`` folder in data directories.
If they are not found, built-in TrueType fonts will be used as a fallback.
Note that an import wizard copies values from ``Morrowind.ini``, so bitmap fonts will be used after import.
If such behaviour is undesirable, ``Fonts_Font*`` entries should be removed from ``openmw.cfg``.

TrueType fonts
--------------

Unlike vanilla Morrowind, OpenMW directly supports TrueType (``.ttf``) fonts. This is the recommended fonts format.
OpenMW has build-in TrueType fonts: MysticCards, DemonicLetters and DejaVuLGCSansMono, which are used by default.
TrueType fonts are configured via ``openmw.cfg`` too:

			fallback=Fonts_Font_0,MysticCards
			fallback=Fonts_Font_2,DemonicLetters

In this example, OpenMW will scan ``Fonts`` folder in data directories for ``.omwfont`` files.
These files are XML files with schema provided by MyGUI. OpenMW uses ``.omwfont`` files which name (without extension) matches ``openmw.cfg`` entries.

It is also possible to adjust the font size via ``settings.cfg`` file::

			[GUI]
			font size = 16

The ``font size`` setting accepts clamped values in range from 12 to 18.

Any Size property in the ``.omwfont`` file has no effect because the engine overrides it.

The engine automatically takes UI scaling factor into account, so don't account for it when tweaking the settings.
