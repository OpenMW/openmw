Fog Settings
############

use distant fog
---------------

:Type:		boolean
:Range:		True/False
:Default:	False

This setting overhauls the behavior of fog calculations.

Normally the fog start and end distance are proportional to the viewing distance
and use the fog depth set in the fallback settings.

Enabling this setting separates the fog distance from the viewing distance and fallback settings and makes fog distance
and apparent density dependent on the weather and the current location according to the settings below.

Unfortunately specific weather-dependent fog factor and offset parameters are currently hard-coded.
They are based off the default settings of MGE XE.

+--------------+------------+--------+
| Weather Type | Fog Factor | Offset |
+==============+============+========+
| Clear        | 1.0        | 0.0    |
+--------------+------------+--------+
| Cloudy       | 0.9        | 0.0    |
+--------------+------------+--------+
| Foggy        | 0.2        | 0.3    |
+--------------+------------+--------+
| Overcast     | 0.7        | 0.0    |
+--------------+------------+--------+
| Rain         | 0.5        | 0.1    |
+--------------+------------+--------+
| Thunderstorm | 0.5        | 0.2    |
+--------------+------------+--------+
| Ashstorm     | 0.2        | 0.5    |
+--------------+------------+--------+
| Blight       | 0.2        | 0.6    |
+--------------+------------+--------+
| Snow         | 0.5        | 0.4    |
+--------------+------------+--------+
| Blizzard     | 0.16       | 0.7    |
+--------------+------------+--------+

Non-underwater fog start and end distance are calculated like this according to these parameters::

	fog start distance = fog factor * (base fog start distance - fog offset * base fog end distance)
	fog end distance = fog factor * (1.0 - fog offset) * base fog end distance

Underwater fog distance is used as-is.

A negative fog start distance means that the fog starts behind the camera
so the entirety of the scene will be at least partially fogged.

A negative fog end distance means that the fog ends behind the camera
so the entirety of the scene will be completely submerged in the fog.

Fog end distance should be larger than the fog start distance.

This setting and all further settings can only be configured by editing the settings configuration file.

distant land fog start
----------------------

:Type:		floating point
:Range:		The whole range of 32-bit floating point
:Default:	16384 (2 cells)

This is the base fog start distance used for distant fog calculations in exterior locations.

distant land fog end
--------------------

:Type:		floating point
:Range:		The whole range of 32-bit floating point
:Default:	40960 (5 cells)

This is the base fog end distance used for distant fog calculations in exterior locations.

distant underwater fog start
----------------------------

:Type:		floating point
:Range:		The whole range of 32-bit floating point
:Default:	-4096

This is the base fog start distance used for distant fog calculations in underwater locations.

distant underwater fog end
--------------------------

:Type:		floating point
:Range:		The whole range of 32-bit floating point
:Default:	2457.6

This is the base fog end distance used for distant fog calculations in underwater locations.

distant interior fog start
--------------------------

:Type:		floating point
:Range:		The whole range of 32-bit floating point
:Default:	0

This is the base fog start distance used for distant fog calculations in interior locations.

distant interior fog end
------------------------

:Type:		floating point
:Range:		The whole range of 32-bit floating point
:Default:	16384 (2 cells)

This is the base fog end distance used for distant fog calculations in interior locations.
