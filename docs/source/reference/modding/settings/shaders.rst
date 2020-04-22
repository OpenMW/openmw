Shaders Settings
################

force shaders
-------------

:Type:		boolean
:Range:		True/False
:Default:	False

Force rendering with shaders. By default, only bump-mapped objects will use shaders.
Enabling this option may cause slightly different visuals if the "clamp lighting" option is set to false.
Otherwise, there should not be a visual difference.

Please note enabling shaders has a significant performance impact on most systems.

force per pixel lighting
------------------------

:Type:		boolean
:Range:		True/False
:Default:	False

Force the use of per pixel lighting. By default, only bump mapped objects use per-pixel lighting.
Has no effect if the 'force shaders' option is false.
Enabling per-pixel lighting results in visual differences to the original MW engine.
It is not recommended to enable this option when using vanilla Morrowind files,
because certain lights in Morrowind rely on vertex lighting to look as intended.

clamp lighting
--------------

:Type:		boolean
:Range:		True/False
:Default:	True

Restrict the amount of lighting that an object can receive to a maximum of (1,1,1).
Only affects objects that render with shaders (see 'force shaders' option).
Always affects terrain.

Leaving this option at its default makes the lighting compatible with Morrowind's fixed-function method,
but the lighting may appear dull and there might be colour shifts. 
Setting this option to 'false' results in more dynamic lighting.

auto use object normal maps
---------------------------

:Type:		boolean
:Range:		True/False
:Default:	False

If this option is enabled, normal maps are automatically recognized and used if they are named appropriately
(see 'normal map pattern', e.g. for a base texture foo.dds, the normal map texture would have to be named foo_n.dds).
If this option is disabled,
normal maps are only used if they are explicitly listed within the mesh file (.nif or .osg file). Affects objects.

auto use object specular maps
-----------------------------

:Type:		boolean
:Range:		True/False
:Default:	False

If this option is enabled, specular maps are automatically recognized and used if they are named appropriately
(see 'specular map pattern', e.g. for a base texture foo.dds,
the specular map texture would have to be named foo_spec.dds).
If this option is disabled, normal maps are only used if they are explicitly listed within the mesh file
(.osg file, not supported in .nif files). Affects objects.

auto use terrain normal maps
----------------------------

:Type:		boolean
:Range:		True/False
:Default:	False

See 'auto use object normal maps'. Affects terrain.

auto use terrain specular maps
------------------------------

:Type:		boolean
:Range:		True/False
:Default:	False

If a file with pattern 'terrain specular map pattern' exists, use that file as a 'diffuse specular' map.
The texture must contain the layer colour in the RGB channel (as usual), and a specular multiplier in the alpha channel.

normal map pattern
------------------

:Type:		string
:Range:
:Default:	_n

The filename pattern to probe for when detecting normal maps
(see 'auto use object normal maps', 'auto use terrain normal maps')

normal height map pattern
-------------------------

:Type:		string
:Range:
:Default:	_nh

Alternative filename pattern to probe for when detecting normal maps.
Files with this pattern are expected to include 'height' in the alpha channel.
This height is used for parallax effects. Works for both terrain and objects.

specular map pattern
--------------------

:Type:		string
:Range:
:Default:	_spec

The filename pattern to probe for when detecting object specular maps (see 'auto use object specular maps')

terrain specular map pattern
----------------------------

:Type:		string
:Range:
:Default:	_diffusespec

The filename pattern to probe for when detecting terrain specular maps (see 'auto use terrain specular maps')

apply lighting to environment maps
----------------------------------

:Type:		boolean
:Range:		True/False
:Default:	False

Normally environment map reflections aren't affected by lighting, which makes environment-mapped (and thus bump-mapped objects) glow in the dark.
Morrowind Code Patch includes an option to remedy that by doing environment-mapping before applying lighting, this is the equivalent of that option.
Affected objects will use shaders.

radial fog
----------

:Type:		boolean
:Range:		True/False
:Default:	False

By default, the fog becomes thicker proportionally to your distance from the clipping plane set at the clipping distance, which causes distortion at the edges of the screen.
This setting makes the fog use the actual eye point distance (or so called Euclidean distance) to calculate the fog, which makes the fog look less artificial, especially if you have a wide FOV.
Note that the rendering will act as if you have 'force shaders' option enabled with this on, which means that shaders will be used to render all objects and the terrain.
