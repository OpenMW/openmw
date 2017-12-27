Shadow Settings
###############

enable shadows
--------------

:Type:		boolean
:Range:		True/False
:Default:	False

Enable or disable the rendering of shadows.
Unlike in the original Morrowind engine, 'Shadow Mapping' is used, which can have a performance impact, but has more realistic results.

number of shadow maps
---------------------

:Type:		integer
:Range:		1 to 8, but higher values may conflict with other texture effects
:Default:	1

Control how many shadow maps to use - more of these means each shadow map texel covers less area, producing better-looking shadows, but may decrease performance.
Using too many shadow maps will lead to them overriding texture slots used for other effects, producing unpleasant artefacts.
A value of three is recommended in most cases, but other values may produce better results or performance.

enable debug hud
----------------

:Type:		boolean
:Range:		True/False
:Default:	False

Enable or disable the debug hud to see what the shadow map(s) contain.
This setting is only recommended for developers, bug reporting and advanced users performing fine-tuning of shadow settings.

compute tight scene bounds
--------------------------

:Type:		boolean
:Range:		True/False
:Default:	False

With this setting enabled, attempt to better use the shadow map(s) by making them cover a smaller area.
This can be especially helpful when looking downwards with a high viewing distance but will be less useful with the default value.
The performance impact of this may be very large.

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



Note: Right now, there is no setting allowing toggling of shadows for statics