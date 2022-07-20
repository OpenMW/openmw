Fonts
#####

Default UI font and font used in magic scrolls are defined in ``openmw.cfg``:

			fallback=Fonts_Font_0,pelagiad
			fallback=Fonts_Font_2,ayembedt

By default, built-in TrueType fonts are used. Font used by console and another debug windows is not configurable (so ``Fonts_Font_1`` is unused).

Morrowind .fnt fonts
--------------------

Morrowind uses a custom ``.fnt`` file format. It is not compatible with the Windows Font File ``.fnt`` format.
To our knowledge, the format is undocumented. OpenMW can load this format and convert it on the fly into something usable
(see font loader `source code <https://gitlab.com/OpenMW/openmw/blob/master/components/fontloader/fontloader.cpp>`_).

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
OpenMW has build-in TrueType fonts: Pelagiad, OMWAyembedt and DejaVuLGCSansMono, which are used by default.
TrueType fonts are configured via ``openmw.cfg`` too:

			fallback=Fonts_Font_0,pelagiad
			fallback=Fonts_Font_2,ayembedt

In this example, OpenMW will scan ``Fonts`` folder in data directories for ``.omwfont`` files.
These files are XML files wich schema used by MyGUI. OpenMW uses files which ``name`` tag matches ``openmw.cfg`` entries:

			<Resource type="ResourceTrueTypeFont" name="pelagiad">

It is also possible to adjust the font size and resolution via ``settings.cfg`` file::

			[GUI]
			font size = 16
			ttf resolution = 96

The ``font size`` setting accepts clamped values in range from 12 to 20 while ``ttf resolution`` setting accepts values from 48 to 960.

Any Resolution or Size properties in the ``.omwfont`` file have no effect because the engine settings override them.

The engine automatically takes UI scaling factor into account, so don't account for it when tweaking the settings.
