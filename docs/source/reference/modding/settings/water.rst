Water Settings
##############

.. note::
   The settings for the water shader are difficult to describe,
   but can be seen immediately in the Water tab of the Video panel in the Options menu.
   Changes there will be saved to these settings.
   It is suggested to stand on the shore of a moderately broad body of water with trees or other objects
   on the far shore to test reflection textures,
   underwater plants in shallow water near by to test refraction textures,
   and some deep water visible from your location to test deep water visibility.

.. omw-setting::
   :title: shader
   :type: boolean
   :range: true, false
   :default: false
   :location: :bdg-info:`In Game > Options > Video > Water`

   This setting enables or disables the water shader, which results in much more realistic looking water surfaces,
   including reflected objects and a more detailed wavy surface.

.. omw-setting::
   :title: rtt size
   :type: int
   :range: > 0
   :default: 512
   :location: :bdg-info:`In Game > Options > Video > Water`

   The setting determines the size of the texture used for reflection and refraction (if enabled).
   For reflection, the texture size determines the detail of reflected images on the surface of the water.
   For refraction, the texture size determines the detail of the objects on the other side of the plane of water
   (which have a wavy appearance caused by the refraction).
   RTT is an acronym for Render To Texture which allows rendering of the scene to be saved as a texture.
   Higher values produces better visuals and result in a marginally lower frame rate depending on your graphics hardware.

   This setting has no effect if the shader setting is false.
   In the Water tab of the Video panel of the Options menu, the choices are Low (512), Medium (1024) and High (2048).
   It is recommended to use values that are a power of two because this results in more efficient use of video hardware.

.. omw-setting::
   :title: refraction
   :type: boolean
   :range: true, false
   :default: false
   :location: :bdg-info:`In Game > Options > Video > Water`

   This setting enables the refraction rendering feature of the water shader.
   Refraction causes deep water to be more opaque
   and objects seen through the plane of the water to have a wavy appearance.
   Enabling this feature results in better visuals, and a marginally lower frame rate depending on your graphics hardware.

   This setting has no effect if the shader setting is false.

.. omw-setting::
   :title: sunlight scattering
   :type: boolean
   :range: true, false
   :default: true
   :location: :bdg-info:`In Game > Options > Video > Water`

   This setting enables sunlight scattering.
   This makes incident sunlight seemingly spread through water, simulating the optical property.

   This setting has no effect if refraction is turned off.

.. omw-setting::
   :title: wobbly shores
   :type: boolean
   :range: true, false
   :default: true
   :location: :bdg-info:`In Game > Options > Video > Water`

   This setting makes shores wobbly.
   The water surface will smoothly fade into the shoreline and wobble based on water normal-mapping, which avoids harsh transitions.

   This setting has no effect if refraction is turned off.

.. omw-setting::
   :title: reflection detail
   :type: int
   :range: 0, 1, 2, 3, 4, 5
   :default: 2
   :location: :bdg-info:`In Game > Options > Video > Water`

   Controls what kinds of things are rendered in water reflections.

   .. list-table::
      :header-rows: 1

      * - Mode
        - Meaning
      * - 0
        - only sky is reflected
      * - 1
        - terrain is also reflected
      * - 2
        - statics, activators, and doors are also reflected
      * - 3
        - items, containers, and particles are also reflected
      * - 4
        - actors are also reflected
      * - 5
        - groundcover objects are also reflected

   In interiors the lowest level is 2.

.. omw-setting::
   :title: rain ripple detail
   :type: int
   :range: 0, 1, 2
   :default: 1
   :location: :bdg-info:`In Game > Options > Video > Water`

   Controls how detailed the raindrop ripples on water are.

   .. list-table::
      :header-rows: 1

      * - Mode
        - Meaning
      * - 0
        - single, non-normal-mapped ring per raindrop
      * - 1
        - normal-mapped raindrops, with multiple rings
      * - 2
        - same as 1, but with a greater number of raindrops

.. omw-setting::
   :title: small feature culling pixel size
   :type: float32
   :range: > 0
   :default: 20.0

   Controls the cutoff in pixels for small feature culling - see the 'Camera' section for more details,
   however this setting in the 'Water' section applies specifically to objects rendered in water reflection
   and refraction textures.

   The setting 'rtt size' interacts with this setting
   because it controls how large a pixel on the water texture (technically called a texel) is in pixels on the screen.

   This setting will have no effect if the shader setting is false,
   or the 'small feature culling' (in the 'Camera' section) is disabled.

.. omw-setting::
   :title: refraction scale
   :type: float32
   :range: 0 to 1
   :default: 1.0

   Simulates light rays refracting when transitioning from air to water, which causes the space under water look scaled down
   in height when viewed from above the water surface. Though adding realism, the setting can cause distortion which can
   make for example aiming at enemies in water more challenging, so it is off by default (i.e. set to 1.0). To get a realistic
   look of real-life water, set the value to 0.75.

   This setting only applies if water shader is on and refractions are enabled. Note that if refractions are enabled and this
   setting if off, there will still be small refractions caused by the water waves, which however do not cause such significant
   distortion.

   .. warning::
      The `refraction scale` is currently mutually exclusive to underwater shadows. Setting this to any value except 1.0
      will cause underwater shadows to be disabled. This will be addressed in issue https://gitlab.com/OpenMW/openmw/-/issues/5709
