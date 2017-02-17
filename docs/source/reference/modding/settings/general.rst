General Settings
################

anisotropy
----------

:Type:		integer
:Range:		0 to 16
:Default:	4

Set the maximum anisotropic filtering on textures. Anisotropic filtering is a method of enhancing the image quality of textures on surfaces that are at oblique viewing angles with respect to the camera. Valid values range from 0 to 16. Modern video cards can often perform 8 or 16 anisotropic filtering with a minimal performance impact. This effect of this setting can be seen in the Video panel of the Options menu by finding a location with straight lines (striped rugs and Balmora cobblestones work well) radiating into the distance, and adjusting the anisotropy slider.

The default value is 4. This setting can be changed in game using the Anisotropy slider in the Detail tab of the Video panel of the Options menu.

screenshot format
-----------------

:Type:		string
:Range:		jpg, png, tga
:Default:	png

Specify the format for screen shots taken by pressing the screen shot key (bound to F12 by default). This setting should be the file extension commonly associated with the desired format. The formats supported will be determined at compilation, but "jpg", "png", and "tga" should be allowed.

The default value is "png". This setting can only be configured by editing the settings configuration file.

texture filtering
-----------------

:Type:		string
:Range:		bilinear, trilinear
:Default:	trilinear

Set the isotropic texture filtering mode to bilinear or trilinear. Bilinear filtering is a texture filtering method used to smooth textures when displayed larger or smaller than they actually are. Bilinear filtering is reasonably accurate until the scaling of the texture gets below half or above double the original size of the texture. Trilinear filtering is an extension of the bilinear texture filtering method, which also performs linear interpolation between mipmaps. Both methods use mipmaps in OpenMW, and the corresponding OpenGL modes are LINEAR_MIPMAP_NEAREST and LINEAR_MIPMAP_LINEAR. Trilinear filtering produces better texturing at a minimal cost on modern video cards.

The default value is trilinear. This setting can be changed in game using the Texture filtering pull down in the Detail tab of the Video panel of the Options menu.