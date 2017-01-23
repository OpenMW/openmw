Water Settings
############

.. note::
	The settings for the water shader are difficult to describe, but can be seen immediately in the Water tab of the Video panel in the Options menu. Changes there will be saved to these settings. It is suggested to stand on the shore of a moderately broad body of water with trees or other objects on the far shore to test reflection textures, underwater plants in shallow water near by to test refraction textures, and some deep water visible from your location to test deep water visibility.

shader
------

:Type:		boolean
:Range:		True/False
:Default:	False

This boolean setting enables or disables the water shader, which results in much more realistic looking water surfaces, including reflected objects and a more detailed wavy surface.

The default value is false. This setting can be toggled with the Shader button in the Water tab of the Video panel of the Options menu.

rtt size
--------

:Type:		integer
:Range:		> 0
:Default:	512

The integer setting determines the size of the texture used for reflection and refraction (if enabled). For reflection, the texture size determines the detail of reflected images on the surface of the water. For refraction, the texture size determines the detail of the objects on the other side of the plane of water (which have a wavy appearance caused by the refraction). RTT is an acronym for Render to Texture which allows rendering of the scene to be saved as a texture.
Higher values produces better visuals and result in a marginally lower frame rate depending on your graphics hardware.

In the Water tab of the Video panel of the Options menu, the choices are Low (512), Medium (1024) and High (2048). This setting has no effect if the shader setting is false. It is recommended to use values that are a power of two because this results in more efficient use of video hardware.

This setting has no effect if the shader setting is false.

refraction
----------

:Type:		boolean
:Range:		True/False
:Default:	False

This boolean setting enables the refraction rendering feature of the water shader. Refraction causes deep water to be more opaque and objects seen through the plane of the water to have a wavy appearance. Enabling this feature results in better visuals, and a marginally lower frame rate depending on your graphics hardware.

This setting has no effect if the shader setting is false.

The default setting is false. This setting can be toggled with the Refraction button in the Water tab of the Video panel of the Options menu.