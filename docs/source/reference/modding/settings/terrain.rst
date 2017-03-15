Terrain Settings
###############

distant terrain
---------------

:Type:		boolean
:Range:		True/False
:Default:	False

Controls whether the engine will use paging and LOD algorithms to render the terrain of the entire world at all times. Otherwise, only the terrain of the loaded cells is displayed. This setting is best used together with the 'viewing distance' setting in the camera section.

To avoid frame drops as the player moves around, nearby terrain pages are always preloaded in the background, regardless of the preloading settings in the 'Cells' section, but the preloading of terrain behind a door or a travel destination, for example, will still be controlled by cell preloading settings.

The distant terrain engine is currently considered experimental and may receive updates and/or further configuration options in the future. The glaring omission of non-terrain objects in the distance somewhat limits this setting's usefulness.
