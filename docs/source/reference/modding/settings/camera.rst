Camera Settings
###############

.. note::
    Some camera settings are available only in the in-game settings menu. See the tab "Scripts/OpenMW Camera".

.. omw-setting::
   :title: near clip
   :type: float32
   :range: ≥ 0.005
   :default: 1
   

   This setting controls the distance to the near clipping plane. The value must be greater than zero.
   Values greater than approximately 18.0 will occasionally clip objects in the world in front of the character.
   Values greater than approximately 8.0 will clip the character's hands in first person view
   and/or the back of their head in third person view.

.. omw-setting::
   :title: small feature culling
   :type: boolean
   :range: true, false
   :default: true
   

   This setting determines whether objects that render to a few pixels or smaller will be culled (not drawn).
   It generally improves performance to enable this feature,
   and by definition the culled objects will be very small on screen.
   The size in pixels for an object to be considered 'small' is controlled by a separate setting.


.. omw-setting::
   :title: small feature culling pixel size
   :type: float32
   :range: > 0
   :default: 2
   

   Controls the cutoff in pixels for the 'small feature culling' setting,
   which will have no effect if 'small feature culling' is disabled.

.. omw-setting::
   :title: viewing distance
   :type: float32
   :range: ≥ 0
   :default: 7168
   :location: :bdg-success:`Launcher > Settings > Visuals > Terrain` :bdg-info:`In Game > Options > Detail`

   This value controls the maximum visible distance (also called the far clipping plane).
   Larger values significantly improve rendering in exterior spaces,
   but also increase the amount of rendered geometry and significantly reduce the frame rate.
   Note that when cells are visible before loading, the geometry will "pop-in" suddenly,
   creating a jarring visual effect. To prevent this effect, this value should not be greater than:

   .. math::

   	\text{CellSizeInUnits} \times \text{CellGridRadius} - 1024

   The CellSizeInUnits is the size of a game cell in units (8192 by default), CellGridRadius determines how many
   neighboring cells to current one to load (1 by default - 3x3 grid), and 1024 is the threshold distance for loading a new cell.
   The field of view setting also interacts with this setting because the view frustum end is a plane,
   so you can see further at the edges of the screen than you should be able to.
   This can be observed in game by looking at distant objects
   and rotating the camera so the objects are near the edge of the screen.
   As a result, this distance is recommended to further be reduced to avoid pop-in for wide fields of view
   and long viewing distances near the edges of the screen if distant terrain and object paging are not used.

   Reductions of up to 25% or more can be required to completely eliminate this pop-in.
   Such situations are unusual and probably not worth the performance penalty introduced
   by loading geometry obscured by fog in the center of the screen.
   See RenderingManager::configureFog for the relevant source code.

   This setting can be adjusted in game from the ridiculously low value of 2500 units to a maximum of 7168 units
   using the View Distance slider in the Detail tab of the Video panel of the Options menu, unless distant terrain is enabled,
   in which case the maximum is increased to 81920 units.

.. omw-setting::
   :title: field of view
   :type: float32
   :range: [1,179]
   :default: 60
   :location: :bdg-info:`In Game > Options > Video`

   Sets the camera field of view in degrees. Recommended values range from 30 degrees to 110 degrees.
   Small values provide a very narrow field of view that creates a "zoomed in" effect,
   while large values cause distortion at the edges of the screen.
   The "field of view" setting interacts with aspect ratio of your video resolution in that more square aspect ratios
   (e.g. 4:3) need a wider field of view to more resemble the same field of view on a widescreen (e.g. 16:9) monitor.

.. omw-setting::
   :title: first person field of view
   :type: float32
   :range: [1,179]
   :default: 60
   

   This setting controls the field of view for first person meshes such as the player's hands and held objects.
   It is not recommended to change this value from its default value
   because the Bethesda provided Morrowind assets do not adapt well to large values,
   while small values can result in the hands not being visible.

.. omw-setting::
   :title: reverse z
   :type: boolean
   :range: true, false
   :default: true
   

   Enables a reverse-z depth buffer in which the depth range is reversed. This
   allows for small :ref:`near clip` values and removes almost all z-fighting with
   terrain and even tightly coupled meshes at extreme view distances. For this to
   be useful, a floating point depth buffer is required. These features require
   driver and hardware support, but should work on any semi-modern desktop hardware
   through OpenGL extensions. The exception is macOS, which has since dropped
   development of OpenGL drivers. If unsupported, this setting has no effect.

   Note, this will force OpenMW to use shaders as if :ref:`force shaders` was enabled.
   The performance impact of this feature should be negligible.
