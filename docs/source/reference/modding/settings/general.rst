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

This setting can be configured in Advanced tab of the launcher.

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
