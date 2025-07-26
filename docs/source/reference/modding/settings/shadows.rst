Shadows Settings
################

.. omw-setting::
   :title: enable shadows
   :type: boolean
   :range: true, false
   :default: false
   :location: :bdg-success:`Launcher > Settings > Visuals > Shadows`

   Enable or disable shadow rendering using shadow mapping.
   More realistic but may reduce performance.
   Forces shaders usage like :ref:`force shaders`.

.. omw-setting::
   :title: number of shadow maps
   :type: int
   :range: 1 to 8
   :default: 3
   :location: :bdg-success:`Launcher > Settings > Visuals > Shadows`

   Number of shadow maps used.
   More maps improve shadow quality but may reduce performance or cause texture conflicts.

.. omw-setting::
   :title: maximum shadow map distance
   :type: float32
   :range: full 32-bit float range
   :default: 8192
   :location: :bdg-success:`Launcher > Settings > Visuals > Shadows`

   Maximum distance shadows cover from the camera.
   Set ≤ 0 to disable distance limit.

.. omw-setting::
   :title: shadow fade start
   :type: float32
   :range: [0, 1]
   :default: 0.9
   :location: :bdg-success:`Launcher > Settings > Visuals > Shadows`

   Fraction of maximum shadow distance at which shadows start fading.
   No effect if distance limit disabled.

.. omw-setting::
   :title: enable debug hud
   :type: boolean
   :range: true, false
   :default: false

   Show debug HUD visualizing shadow map contents.
   Recommended for developers or advanced users.

.. omw-setting::
   :title: enable debug overlay
   :type: boolean
   :range: true, false
   :default: false

   Show debug overlay showing shadow map coverage areas.
   Recommended for advanced debugging.

.. omw-setting::
   :title: compute scene bounds
   :type: string
   :range: primitives | bounds | none
   :default: bounds
   :location: :bdg-success:`Launcher > Settings > Visuals > Shadows`

   Method to compute shadow map coverage:
   - `primitives`: better shadows, higher CPU cost
   - `bounds`: better performance, lower quality
   - `none`: disables computation

.. omw-setting::
   :title: shadow map resolution
   :type: int
   :range: dependent on GPU/driver
   :default: 1024
   :location: :bdg-success:`Launcher > Settings > Visuals > Shadows`

   Size of shadow maps.
   Higher values improve quality but increase GPU load.
   Powers of two may perform better on some hardware.

.. omw-setting::
   :title: actor shadows
   :type: boolean
   :range: true, false
   :default: false

   Enable shadows cast by NPCs and creatures.
   May reduce performance.

.. omw-setting::
   :title: player shadows
   :type: boolean
   :range: true, false
   :default: false
   :location: :bdg-success:`Launcher > Settings > Visuals > Shadows`

   Enable shadows cast by the player character.
   May reduce performance.

.. omw-setting::
   :title: terrain shadows
   :type: boolean
   :range: true, false
   :default: false
   :location: :bdg-success:`Launcher > Settings > Visuals > Shadows`

   Enable shadows cast by terrain.
   May reduce performance.

.. omw-setting::
   :title: object shadows
   :type: boolean
   :range: true, false
   :default: false
   :location: :bdg-success:`Launcher > Settings > Visuals > Shadows`

   Enable shadows cast by static objects.
   May reduce performance.

.. omw-setting::
   :title: enable indoor shadows
   :type: boolean
   :range: true, false
   :default: false
   :location: :bdg-success:`Launcher > Settings > Visuals > Shadows`

   Enable shadows indoors.
   Only actors cast shadows indoors without full ceiling shadows.
   Can cause shadows appearing through objects.

.. omw-setting::
   :title: polygon offset factor
   :type: float32
   :range: full 32-bit float range, sensibly >1.0
   :default: 1.1

   Polygon offset factor for shadow map rendering.
   Reduces shadow flicker but may increase Peter Panning.

.. omw-setting::
   :title: polygon offset units
   :type: float32
   :range: full 32-bit float range, sensibly 1 to 10
   :default: 4.0

   Polygon offset units for shadow map rendering.
   Works with offset factor to reduce artifacts.

.. omw-setting::
   :title: normal offset distance
   :type: float32
   :range: full 32-bit float range, sensibly 0 to 2
   :default: 1.0

   Distance along surface normal to project shadow coordinates.
   Reduces flicker with less Peter Panning than polygon offset.

.. omw-setting::
   :title: use front face culling
   :type: boolean
   :range: true, false
   :default: false

   Exclude front faces from shadow maps for performance.
   May increase Peter Panning artifacts.

.. omw-setting::
   :title: split point uniform logarithmic ratio
   :type: float32
   :range: [0, 1]
   :default: 0.5

   Controls balance between logarithmic and uniform split points for shadow splits.
   Adjust when using large view distances or distant terrain.

.. omw-setting::
   :title: split point bias
   :type: float32
   :range: full C++ float range
   :default: 0.0

   Bias parameter used in shadow split computation.
   Non-zero values can cause unusual behavior.

.. omw-setting::
   :title: minimum lispsm near far ratio
   :type: float32
   :range: > 0
   :default: 0.25

   Minimum near/far ratio for Light Space Perspective Shadow Map.
   Controls distribution of shadow detail near and far from the camera.
