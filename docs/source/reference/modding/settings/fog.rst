Fog Settings
############

.. omw-setting::
   :title: use distant fog
   :type: boolean
   :range: true, false
   :default: false
   
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

.. omw-setting::
   :title: distant land fog start
   :type: float32
   :range: full float32 range
   :default: 16384 (2 cells)
   

   This is the base fog start distance used for distant fog calculations in exterior locations.

.. omw-setting::
   :title: distant land fog end
   :type: float32
   :range: full float32 range
   :default: 40960 (5 cells)
   

   This is the base fog end distance used for distant fog calculations in exterior locations.

.. omw-setting::
   :title: distant underwater fog start
   :type: float32
   :range: full float32 range
   :default: -4096
   

   This is the base fog start distance used for distant fog calculations in underwater locations.

.. omw-setting::
   :title: distant underwater fog end
   :type: float32
   :range: full float32 range
   :default: 2457.6
   

   This is the base fog end distance used for distant fog calculations in underwater locations.

.. omw-setting::
   :title: distant interior fog start
   :type: float32
   :range: full float32 range
   :default: 0
   

   This is the base fog start distance used for distant fog calculations in interior locations.

.. omw-setting::
   :title: distant interior fog end
   :type: float32
   :range: full float32 range
   :default: 16384 (2 cells)
   

   This is the base fog end distance used for distant fog calculations in interior locations.

.. omw-setting::
   :title: radial fog
   :type: boolean
   :range: true, false
   :default: false
   :location: :bdg-success:`Launcher > Settings > Visuals > Fog`

   By default, the fog becomes thicker proportionally to your distance from the clipping plane set at the clipping distance, which causes distortion at the edges of the screen.
   This setting makes the fog use the actual eye point distance (or so called Euclidean distance) to calculate the fog, which makes the fog look less artificial, especially if you have a wide FOV.
   Note that the rendering will act as if you have 'force shaders' option enabled with this on, which means that shaders will be used to render all objects and the terrain.

.. omw-setting::
   :title: exponential fog
   :type: boolean
   :range: true, false
   :default: false
   :location: :bdg-success:`Launcher > Settings > Visuals > Fog`

   Similar to "radial fog" but uses an exponential formula for the fog.
   Note that the rendering will act as if you have 'force shaders' option enabled with this on, which means that shaders will be used to render all objects and the terrain.

.. omw-setting::
   :title: sky blending
   :type: boolean
   :range: true, false
   :default: false
   :location: :bdg-success:`Launcher > Settings > Visuals > Fog`

   Whether to use blending with the sky for everything that is close to the clipping plane.
   If enabled the clipping plane becomes invisible.
   Note that the rendering will act as if you have 'force shaders' option enabled with this on, which means that shaders will be used to render all objects and the terrain.

.. omw-setting::
   :title: sky blending start
   :type: float32
   :range: [0.0, 1.0)
   :default: 0.8
   :location: :bdg-success:`Launcher > Settings > Visuals > Fog`

   The fraction of the maximum distance at which blending with the sky starts.

.. omw-setting::
   :title: sky rtt resolution
   :type: float32|float32
   :default: 512 256

   The sky RTT texture size, used only for sky blending. Smaller values
   reduce quality of the sky blending, but can have slightly better performance.
