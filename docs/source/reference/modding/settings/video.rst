Video Settings
##############

.. omw-setting::
   :title: resolution x
   :type: int
   :range: > 0
   :default: 800
   :location: :bdg-success:`Launcher > Display` :bdg-info:`In Game > Options > Video`

   This setting determines the horizontal resolution of the OpenMW game window.
   Larger values produce more detailed images within the constraints of your graphics hardware,
   but may reduce the frame rate.

.. omw-setting::
   :title: resolution y
   :type: int
   :range: > 0
   :default: 600
   :location: :bdg-success:`Launcher > Display` :bdg-info:`In Game > Options > Video`

   This setting determines the vertical resolution of the OpenMW game window.
   Larger values produce more detailed images within the constraints of your graphics hardware,
   but may reduce the frame rate.

.. omw-setting::
   :title: window mode
   :type: int
   :range: 0, 1, 2
   :default: 2
   :location: :bdg-success:`Launcher > Display` :bdg-info:`In Game > Options > Video`

   This setting determines the window mode.

   .. list-table::
      :header-rows: 1

      * - Mode
        - Meaning
      * - 0
        - Exclusive fullscreen
      * - 1
        - Windowed fullscreen, borderless window that matches screen resolution
      * - 2
        - Windowed

.. omw-setting::
   :title: screen
   :type: int
   :range: ≥ 0
   :default: 0
   :location: :bdg-success:`Launcher > Display`

   This setting determines which screen the game will open on in multi-monitor configurations.
   This setting is particularly important when the fullscreen setting is true,
   since this is the only way to control which screen is used,
   but it can also be used to control which screen a normal window or a borderless window opens on as well.
   The screens are numbered in increasing order, beginning with 0.

.. omw-setting::
   :title: minimize on focus loss
   :type: boolean
   :range: true, false
   :default: true

   Minimize the OpenMW window if it loses cursor focus. This setting is primarily useful for single screen configurations,
   so that the OpenMW screen in full screen mode can be minimized
   when the operating system regains control of the mouse and keyboard.
   On multiple screen configurations, disabling this option makes it easier to switch between screens while playing OpenMW.

   .. note::
      A minimized game window consumes less system resources and produces less heat,
      since the game does not need to render in minimized state.
      It is therefore advisable to minimize the game during pauses
      (either via use of this setting, or by minimizing the window manually).

   This setting has no effect if the fullscreen setting is false.

   .. note::

      Corresponds to SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS.

.. omw-setting::
   :title: window border
   :type: boolean
   :range: true, false
   :default: true
   :location: :bdg-success:`Launcher > Display` :bdg-info:`In Game > Options > Video`

   This setting determines whether there's an operating system border drawn around the OpenMW window.
   If this setting is true, the window can be moved and resized with the operating system window controls.
   If this setting is false, the window has no operating system border.

   This setting has no effect if the fullscreen setting is true.

.. omw-setting::
   :title: antialiasing
   :type: int
   :range: ≥ 0
   :default: 0
   :location: :bdg-success:`Launcher > Display`

   This setting controls anti-aliasing. Anti-aliasing is a technique designed to improve the appearance of polygon edges,
   so they do not appear to be "jagged".
   Anti-aliasing can smooth these edges at the cost of a minor reduction in the frame rate.
   A value of 0 disables anti-aliasing.
   Other values are supported according to the capabilities of your graphics hardware.
   Higher values do a better job of smoothing out the image but have a greater impact on frame rate.

.. omw-setting::
   :title: vsync mode
   :type: int
   :range: 0, 1, 2
   :default: 0
   :location: :bdg-success:`Launcher > Display` :bdg-info:`In Game > Options > Video`

   This setting determines whether frame draws are synchronized with the vertical refresh rate of your monitor.
   Enabling this setting can reduce screen tearing,
   a visual defect caused by updating the image buffer in the middle of a screen draw.
   Enabling this option (1 or 2) typically implies limiting the framerate to the refresh rate of your monitor,
   but may also introduce additional delays caused by having to wait until the appropriate time
   (the vertical blanking interval) to draw a frame, and a loss in mouse responsiveness known as 'input lag'.
   Mode 2 of this option corresponds to the use of adaptive vsync. Adaptive vsync is turned off if the framerate
   cannot reach your display's refresh rate. This prevents the input lag from becoming unbearable but may lead to tearing.
   Some hardware might not support this mode, in which case traditional vsync will be used.


.. omw-setting::
   :title: framerate limit
   :type: float32
   :range: ≥ 0.0
   :default: 300

   This setting determines the maximum frame rate in frames per second.
   If this setting is 0.0, the frame rate is unlimited.

   There are several reasons to consider capping your frame rate,
   especially if you're already experiencing a relatively high frame rate (greater than 60 frames per second).
   Lower frame rates will consume less power and generate less heat and noise.
   Frame rates above 60 frames per second rarely produce perceptible improvements in visual quality,
   but may improve input responsiveness.
   Capping the frame rate may in some situations reduce the perception of choppiness
   (highly variable frame rates during game play) by lowering the peak frame rates.

   This setting interacts with the vsync setting in the Video section
   in the sense that enabling vertical sync limits the frame rate to the refresh rate of your monitor
   (often 60 frames per second).
   Choosing to limit the frame rate using this setting instead of vsync may reduce input lag
   due to the game not having to wait for the vertical blanking interval.


.. omw-setting::
   :title: contrast
   :type: float32
   :range: > 0.0
   :default: 1.0

   This setting controls the contrast correction for all video in the game.
   It has been reported to not work on some Linux systems.


.. omw-setting::
   :title: gamma
   :type: float32
   :range: > 0.0
   :default: 1.0
   :location: :bdg-success:`Launcher > Display` :bdg-info:`In Game > Options > Video > Detail Level`

   This setting controls the gamma correction for all video in the game.
   Gamma is an exponent that makes colors brighter if greater than 1.0 and darker if less than 1.0.

   .. warning::

      This setting is only supported on Windows platform. The setting will not be displayed in the in-game menu if not available.
