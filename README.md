OpenMW
======

* License: GPLv3 (see [LICENSE](https://gitlab.com/OpenMW/openmw/-/raw/master/LICENSE) for more information)
* Website: https://www.openmw.org

Font Licenses:
* DejaVuLGCSansMono.ttf: custom (see [files/data/fonts/DejaVuFontLicense.txt](https://gitlab.com/OpenMW/openmw/-/raw/master/files/data/fonts/DejaVuFontLicense.txt) for more information)
* DemonicLetters.ttf: SIL Open Font License (see [files/data/fonts/DemonicLettersFontLicense.txt](https://gitlab.com/OpenMW/openmw/-/raw/master/files/data/fonts/DemonicLettersFontLicense.txt) for more information)
* MysticCards.ttf: SIL Open Font License (see [files/data/fonts/MysticCardsFontLicense.txt](https://gitlab.com/OpenMW/openmw/-/raw/master/files/data/fonts/MysticCardsFontLicense.txt) for more information)


About this fork
----------------

This is a fork of OpenMW 0.51 designed for extracting map images from Morrowind.
The tool generates a world height map at a resolution of 32x32 pixels per cell, local map tiles at 256x256 pixels per cell, and a world map composed of these tiles.


Usage
-----

1. In the `openmw-launcher` (from this fork or any OpenMW >= 0.49), select the content list you want to extract the map from.
2. On the Display tab, it is recommended to set the game to windowed mode with the minimum resolution (640x480) so the extraction can run in the background. Warning: minimizing the game window will pause the process.
3. Run the `openmw` executable from this fork (with or without command-line options) and wait for the generation to complete.

When extraction finishes, the message "Map extraction complete." will be shown; at that point, you can close the game.

By default textures are written to `./textures/advanced_world_map/` inside the application directory.


Note about scripts
------------------

Files with the `.omwscripts` extension are not loaded by this build. If a mod packages `.omwscripts` into other formats, those scripts may not work correctly because many standard API calls are removed.


Command-line options (fork-specific)
-----------------------------------
```
--world-map-output=""          Directory to save the world map texture. Default: `./textures/advanced_world_map/custom/`
--local-map-output=""          Directory to save local map textures. Default: `./textures/advanced_world_map/local/`
--overwrite-maps               Allow overwriting existing local map files. By default local maps are not overwritten.
--tilemap-downscale-factor=4   Downscale factor for tilemap generation (must be power of 2). Default: 4.
```

Notes
-----

This fork was created for personal use and is not intended for major development. Much of the code in this repository was generated with assistance of an LLM; the author does not know C++ at all.
