Groundcover Settings
####################

.. omw-setting::
   :title: enabled
   :type: boolean
   :range: true, false
   :default: false
   

   Allows the engine to use groundcover.
   Groundcover objects are static and come from ESP files registered via "groundcover" entries in `openmw.cfg`,
   not via "content". These objects are assumed to have no collision and cannot be interacted with,
   allowing them to be merged and animated efficiently regardless of player distance.

.. omw-setting::
   :title: density
   :type: float32
   :range: 0.0 (0%) to 1.0 (100%)
   :default: 1.0
   

   Determines how many groundcover instances from content files are used in the game.
   Higher values increase density but may impact performance.

.. omw-setting::
   :title: rendering distance
   :type: float32
   :range: ≥ 0.0
   :default: 6144.0
   

   Sets the distance (in game units) at which grass pages are rendered.
   Larger values may reduce performance.

.. omw-setting::
   :title: stomp mode
   :type: int
   :range: 0, 1, 2
   :default: 2
   

   Determines how grass responds to player movement.

   .. list-table::
      :header-rows: 1

      * - Mode
        - Meaning
      * - 0
        - Grass cannot be trampled.
      * - 1
        - Only the player's XY position is taken into account.
      * - 2
        - Player's height above the ground is also considered.

   In MGE XE, grass responds to player jumping due to changes in XY position,
   even when levitating. OpenMW’s height-aware system avoids false triggers,
   but grass may snap back when the player exits it quickly.

   Avoid using MGE XE's intensity constants when this is set to 2;
   set :ref:`stomp intensity` to 0 or 1 in that case.

.. omw-setting::
   :title: stomp intensity
   :type: int
   :range: 0, 1, 2
   :default: 1
   

   Determines the distance from the player at which grass reacts to footsteps,
   and how far it moves in response.

   .. list-table::
      :header-rows: 1

      * - Preset
        - Range (Units)
        - Distance (Units)
        - Description
      * - 2
        - 150
        - 60
        - MGE XE levels — excessive/comical, matches legacy mods.
      * - 1
        - 80
        - 40
        - Reduced levels — visually balanced.
      * - 0
        - 50
        - 20
        - Gentle levels — subtle and restrained.
