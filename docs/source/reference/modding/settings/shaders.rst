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
Note that groundcover shaders ignore this setting.

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

lighting method
---------------

:Type:		string
:Range:		legacy|shaders compatibility|shaders
:Default:	default

Sets the internal handling of light sources.

'legacy' is restricted to 8 lights per object and emulates fixed function
pipeline compatible lighting.

'shaders compatibility' removes the light limit controllable through :ref:`max
lights` and follows a modifed attenuation formula which can drastically reduce
light popping and seams. This mode also enables lighting on groundcover and a
configurable light fade. It is recommended to use this with older hardware and a
light limit closer to 8. Because of its wide range of compatibility it is set as
the default.

'shaders' carries all of the benefits that 'shaders compatibility' does, but
uses a modern approach that allows for a higher :ref:`max lights` count with
little to no performance penalties on modern hardware. It is recommended to use
this mode when supported and where the GPU is not a bottleneck. On some weaker
devices, using this mode along with :ref:`force per pixel lighting` can carry
performance penalties.

When enabled, groundcover lighting is forced to be vertex lighting, unless
normal maps are provided. This is due to some groundcover mods using the Z-Up
normals technique to avoid some common issues with shading. As a consequence,
per pixel lighting would give undesirable results.

Note that the rendering will act as if you have 'force shaders' option enabled
when not set to 'legacy'. This means that shaders will be used to render all objects and
the terrain.

light bounds multiplier
-----------------------

:Type:		float
:Range:		0.0-5.0
:Default:	1.65

Controls the bounding sphere radius of point lights, which is used to determine
if an object should receive lighting from a particular light source. Note, this
has no direct effect on the overall illumination of lights. Larger multipliers
will allow for smoother transitions of light sources, but may require an
increase in :ref:`max lights` and thus carries a performance penalty. This
especially helps with abrupt light popping with handheld light sources such as
torches and lanterns.

This setting has no effect if :ref:`lighting method` is 'legacy'.

maximum light distance
----------------------

:Type:		float
:Range:		The whole range of 32-bit floating point
:Default:	8192

The maximum distance from the camera that lights will be illuminated, applies to
both interiors and exteriors. A lower distance will improve performance. Set
this to a non-positive value to disable fading.

This setting has no effect if :ref:`lighting method` is 'legacy'.

light fade start
----------------

:Type:		float
:Range:		0.0-1.0
:Default:	0.85

The fraction of the maximum distance at which lights will begin to fade away.
Tweaking it will make the transition proportionally more or less smooth.

This setting has no effect if the :ref:`maximum light distance` is non-positive
or :ref:`lighting method` is 'legacy'.

max lights
----------

:Type:		integer
:Range:		2-64
:Default:	8

Sets the maximum number of lights that each object can receive lighting from.
Increasing this too much can cause significant performance loss, especially if
:ref:`lighting method` is not set to 'shaders' or :ref:`force per pixel
lighting` is on.

This setting has no effect if :ref:`lighting method` is 'legacy'.

minimum interior brightness
------------------------

:Type:		float
:Range:		0.0-1.0
:Default:	0.08

Sets the minimum interior ambient brightness for interior cells when
:ref:`lighting method` is not 'legacy'. A consequence of the new lighting system
is that interiors will sometimes be darker since light sources now have sensible
fall-offs. A couple solutions are to either add more lights or increase their
radii to compensate, but these require content changes. For best results it is
recommended to set this to 0.0 to retain the colors that level designers
intended. If brighter interiors are wanted, however, this setting should be
increased. Note, it is advised to keep this number small (< 0.1) to avoid the
aforementioned changes in visuals.

This setting has no effect if :ref:`lighting method` is 'legacy'.

antialias alpha test
---------------------------------------

:Type:		boolean
:Range:		True/False
:Default:	False

Convert the alpha test (cutout/punchthrough alpha) to alpha-to-coverage when :ref:`antialiasing` is on.
This allows MSAA to work with alpha-tested meshes, producing better-looking edges without pixelation.
When MSAA is off, this setting will have no visible effect, but might have a performance cost.
