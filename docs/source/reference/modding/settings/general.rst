General Settings
################

anisotropy
----------

:Type:		integer
:Range:		0 to 16
:Default:	4

Set the maximum anisotropic filtering on textures.
Anisotropic filtering is a method of enhancing the image quality of textures
on surfaces that are at oblique viewing angles with respect to the camera. Valid values range from 0 to 16.
Modern video cards can often perform 8 or 16 anisotropic filtering with a minimal performance impact.
This effect of this setting can be seen in the Video panel of the Options menu by finding a location with straight lines
(striped rugs and Balmora cobblestones work well) radiating into the distance, and adjusting the anisotropy slider.

This setting can be changed in game
using the Anisotropy slider in the Detail tab of the Video panel of the Options menu.

screenshot format
-----------------

:Type:		string
:Range:		jpg, png, tga
:Default:	png

Specify the format for screen shots taken by pressing the screen shot key (bound to F12 by default).
This setting should be the file extension commonly associated with the desired format.
The formats supported will be determined at compilation, but "jpg", "png", and "tga" should be allowed.

This setting can be controlled in the Settings tab of the launcher.

texture mag filter
------------------

:Type:		string
:Range:		nearest, linear
:Default:	linear

Set the texture magnification filter type.

texture min filter
------------------

:Type:		string
:Range:		nearest, linear
:Default:	linear

Set the texture minification filter type.

texture mipmap
--------------

:Type:		string
:Range:		none, nearest, linear
:Default:	nearest

Set the texture mipmap type to control the method mipmaps are created.
Mipmapping is a way of reducing the processing power needed during minification
by pregenerating a series of smaller textures.

notify on saved screenshot
--------------------------

:Type:		boolean
:Range:		True/False
:Default:	False

Show message box when screenshot is saved to a file.

This setting can be controlled in the Settings tab of the launcher.

preferred locales
-----------------

:Type:		string
:Default:	en

List of the preferred locales separated by comma.
For example "de,en" means German as the first priority and English as a fallback.

Each locale must consist of a two-letter language code (e.g. "de" or "en") and
can also optionally include a two-letter country code (e.g. "en_US", "fr_CA").
Locales with country codes can match locales without one (e.g. specifying "en_US"
will match "en"), so is recommended that you include the country codes where possible,
since if the country code isn't specified the generic language-code only locale might
refer to any of the country-specific variants.

Two highest priority locales may be assigned via the Localization tab of the in-game options.

gmst overrides l10n
-------------------

:Type:		boolean
:Range:		True/False
:Default:	True

If true, localization GMSTs in content have priority over l10n files.
Setting to false can be useful if selected preferred locale doesn't
match the language of content files.

Can be changed via the Localization tab of the in-game options.

log buffer size
---------------

:Type:		platform dependant unsigned integer
:Range:		>= 0
:Default:	65536

Buffer size for the in-game log viewer (press F10 to toggle the log viewer).
When the log doesn't fit into the buffer, only the end of the log is visible in the log viewer.
Zero disables the log viewer.

This setting can only be configured by editing the settings configuration file.

console history buffer size 
---------------------------

:Type:		platform dependant unsigned integer
:Range:		>= 0
:Default:	4096

Number of console history objects to retrieve from previous session. If the number of history 
objects in the file exceeds this value, history objects will be erased starting from the oldest. 
This operation runs on every new session. See :doc:`../paths` for location of the history file.

This setting can only be configured by editing the settings configuration file.

