Map Settings
############

.. omw-setting::
   :title: global
   :type: boolean
   :range: true, false
   :default: false

   If true, the map window displays the world map; otherwise, it displays the local map.
   Updates automatically when switching map views in-game.

.. omw-setting::
   :title: global map cell size
   :type: int
   :range: 1 to 50
   :default: 18

   Sets the width in pixels of each cell on the world map.
   Larger values show more detail, smaller values less.
   Recommended values: 12 to 36.
   Changing this affects saved games due to scaling of explored area overlays.

.. omw-setting::
   :title: local map hud fog of war
   :type: boolean
   :range: true, false
   :default: false

   Enables fog of war rendering on the HUD map.
   Default off since map size limits fog impact.

.. omw-setting::
   :title: local map resolution
   :type: int
   :range: ≥ 1
   :default: 256

   Controls resolution of the GUI local map window.
   Larger values increase detail but may cause load time and VRAM issues.

.. omw-setting::
   :title: local map widget size
   :type: int
   :range: ≥ 1
   :default: 512

   Sets the canvas size of the GUI local map window.
   Larger sizes increase on-screen map size and panning.

.. omw-setting::
   :title: allow zooming
   :type: boolean
   :range: true, false
   :default: false
   :location: :bdg-success:`Launcher > Settings > Interface`

   Enables zooming on local and global maps via mouse wheel.

.. omw-setting::
   :title: max local viewing distance
   :type: int
   :range: > 0
   :default: 10

   Controls viewing distance on the local map when 'distant terrain' is enabled.
   Increasing this may increase cell load times.
