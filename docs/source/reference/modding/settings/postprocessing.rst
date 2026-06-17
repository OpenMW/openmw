Post-Processing Settings
########################

.. omw-setting::
   :title: enabled
   :type: boolean
   :range: true, false
   :default: false
   :location: :bdg-success:`Launcher > Settings> Visuals > Post Processing` :bdg-info:`In Game > Options > Video`

   Enable or disable post-processing effects.
   Requires post-processing shaders to be installed.

.. omw-setting::
   :title: chain
   :type: string
   :location: :bdg-info:`In Game > F2 Menu`

   List of active post-processing effects and their order.
   Recommended to configure via in-game HUD (default key: F2).
   Note: an empty chain does not disable post-processing.
   Has no effect if :ref:`enabled` is false.

.. omw-setting::
   :title: auto exposure speed
   :type: float32
   :range: > 0.0001
   :default: 0.9
   :location: :bdg-success:`Launcher > Settings> Visuals > Post Processing`

   Controls speed of eye adaptation (scene luminance changes between frames).
   Smaller values cause slower adaptation.
   Most noticeable when moving between drastically different lighting (e.g., dark cave to bright sun).
   Has no effect if HDR or :ref:`enabled` is false.

.. omw-setting::
   :title: transparent postpass
   :type: boolean
   :range: true, false
   :default: true
   :location: :bdg-success:`Launcher > Settings> Visuals > Post Processing`

   Re-renders transparent objects with alpha-clipping using a fixed threshold.
   Important for vanilla content where blended objects disable depth writes and have large alpha margins.

   .. warning::
      Can be performance heavy with vanilla assets.
      For better performance, use alpha-tested foliage mods (e.g., Morrowind Optimization Patch) and disable this setting.
      Disable if no shaders use the depth buffer.
