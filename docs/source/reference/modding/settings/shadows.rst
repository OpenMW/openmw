Shadows Settings
################

Main settings
*************

enable shadows
--------------

:Type:		boolean
:Range:		True/False
:Default:	False

Enable or disable the rendering of shadows.
Unlike in the original Morrowind engine, 'Shadow Mapping' is used, which can have a performance impact, but has more realistic results.
Bear in mind that this will force OpenMW to use shaders as if :ref:`force shaders` was enabled.
A keen developer may be able to implement compatibility with fixed-function mode using the advice of `this post <https://github.com/OpenMW/openmw/pull/1547#issuecomment-369657381>`_, but it may be more difficult than it seems.

number of shadow maps
---------------------

:Type:		integer
:Range:		1 to 8, but higher values may conflict with other texture effects
:Default:	3

Control how many shadow maps to use - more of these means each shadow map texel covers less area, producing better-looking shadows, but may decrease performance.
Using too many shadow maps will lead to them overriding texture slots used for other effects, producing unpleasant artefacts.
A value of three is recommended in most cases, but other values may produce better results or performance.

maximum shadow map distance
---------------------------

:Type:		float
:Range:		The whole range of 32-bit floating point
:Default:	8192

The maximum distance from the camera shadows cover, limiting their overall area coverage
and improving their quality and performance at the cost of removing shadows of distant objects or terrain.
Set this to a non-positive value to remove the limit.

shadow fade start
-------------------

:Type:		float
:Range:		0.0-1.0
:Default:	0.9

The fraction of the maximum shadow map distance at which the shadows will begin to fade away.
Tweaking it will make the transition proportionally more or less smooth.
This setting has no effect if the maximum shadow map distance is non-positive (infinite).

allow shadow map overlap
------------------------

:Type:		boolean
:Range:		True/False
:Default:	True

If true, allow shadow maps to overlap.
Counter-intuitively, will produce much better results when the light is behind the camera.
When enabled, OpenMW uses Cascaded Shadow Maps and when disabled, it uses Parallel Split Shadow Maps.

enable debug hud
----------------

:Type:		boolean
:Range:		True/False
:Default:	False

Enable or disable the debug hud to see what the shadow map(s) contain.
This setting is only recommended for developers, bug reporting and advanced users performing fine-tuning of shadow settings.

enable debug overlay
----------------

:Type:		boolean
:Range:		True/False
:Default:	False

Enable or disable the debug overlay to see the area covered by each shadow map.
This setting is only recommended for developers, bug reporting and advanced users performing fine-tuning of shadow settings.

compute tight scene bounds
--------------------------

:Type:		boolean
:Range:		True/False
:Default:	True

With this setting enabled, attempt to better use the shadow map(s) by making them cover a smaller area.
This can be especially helpful when looking downwards with a high viewing distance but will be less useful with the default value.
May have a minor to major performance impact.

shadow map resolution
---------------------

:Type:		integer
:Range:		Dependent on GPU/driver combination
:Default:	1024

Control How large to make the shadow map(s).
Higher values increase GPU load but can produce better-looking results.
Power-of-two values may turn out to be faster than smaller values which are not powers of two on some GPU/driver combinations.

actor shadows
-------------

:Type:		boolean
:Range:		True/False
:Default:	False

Allow actors to cast shadows.
Potentially decreases performance.

player shadows
--------------

:Type:		boolean
:Range:		True/False
:Default:	False

Allow the player to cast shadows.
Potentially decreases performance.

terrain shadows
---------------

:Type:		boolean
:Range:		True/False
:Default:	False

Allow terrain to cast shadows.
Potentially decreases performance.

object shadows
--------------

:Type:		boolean
:Range:		True/False
:Default:	False

Allow static objects to cast shadows.
Potentially decreases performance.

enable indoor shadows
---------------------

:Type:		boolean
:Range:		True/False
:Default:	False

Allow shadows indoors.
Due to limitations with Morrowind's data, only actors can cast shadows indoors without the ceiling casting a shadow everywhere.
Some might feel this is distracting as shadows can be cast through other objects, so indoor shadows can be disabled completely.

Expert settings
***************

These settings are probably too complicated for regular users to judge what might be good values to set them to.
If you've got a good understanding of how shadow mapping works, or you've got enough time to try a large set of values, you may get better results tuning these yourself.
Copying values from another user who's done careful tuning is the recommended way of arriving at an optimal value for these settings.

Understanding what some of these do might be easier for people who've read `this paper on Parallel Split Shadow Maps <https://pdfs.semanticscholar.org/15a9/f2a7cf6b1494f45799617c017bd42659d753.pdf>`_ and understood how they interact with the transformation used with Light Space Perspective Shadow Maps.

polygon offset factor
---------------------

:Type:		float
:Range:		Theoretically the whole range of 32-bit floating point, but values just above 1.0 are most sensible.
:Default:	1.1

Used as the factor parameter for the polygon offset used for shadow map rendering.
Higher values reduce shadow flicker, but risk increasing Peter Panning.
See `the OpenGL documentation for glPolygonOffset <https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glPolygonOffset.xhtml>`_ for details.

polygon offset units
---------------------

:Type:		float
:Range:		Theoretically the whole range of 32-bit floating point, but values between 1 and 10 are most sensible.
:Default:	4.0

Used as the units parameter for the polygon offset used for shadow map rendering.
Higher values reduce shadow flicker, but risk increasing Peter Panning.
See `the OpenGL documentation for glPolygonOffset <https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glPolygonOffset.xhtml>`_ for details.

normal offset distance
----------------------

:Type:		float
:Range:		Theoretically the whole range of 32-bit floating point, but values between 0 and 2 are most sensible.
:Default:	1.0

How far along the surface normal to project shadow coordinates.
Higher values significantly reduce shadow flicker, usually with a lower increase of Peter Panning than the Polygon Offset settings.
This value is in in-game units, so 1.0 is roughly 1.4 cm.

use front face culling
----------------------

:Type:		boolean
:Range:		True/False
:Default:	False

Excludes theoretically unnecessary faces from shadow maps, slightly increasing performance.
In practice, Peter Panning can be much less visible with these faces included, so if you have high polygon offset values, leaving this off may help minimise the side effects.

split point uniform logarithmic ratio
-------------------------------------

:Type:		float
:Range:		0.0-1.0 for sensible results. Other values may 'work' but could behave bizarrely.
:Default:	0.5

Controls the ratio of :math:`C_i^{log}` versus :math:`C_i^{uniform}` used to form the Practical Split Scheme as described in the linked paper.
When using a larger-than-default viewing distance and distant terrain, and you have `allow shadow map overlap`_ enabled, larger values will prevent nearby shadows losing quality.
It is therefore recommended that this isn't left at the default when the viewing distance is changed.

split point bias
----------------

:Type:		float
:Range:		Any value supported by C++ floats on your platform, although undesirable behaviour is more likely to appear the further the value is from zero.
:Default:	0.0

The :math:`\delta_{bias}` parameter used to form the Practical Split Scheme as described in the linked paper.

minimum lispsm near far ratio
-----------------------------

:Type:		float
:Range:		Must be greater than zero.
:Default:	0.25

Controls the minimum near/far ratio for the Light Space Perspective Shadow Map transformation.
Helps prevent too much detail being brought towards the camera at the expense of detail further from the camera.
Increasing this pushes detail further away by moving the frustum apex further from the near plane.
