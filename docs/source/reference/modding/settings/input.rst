Input Settings
##############

.. omw-setting::
   :title: grab cursor
   :type: boolean
   :range: true, false
   :default: true
   :location: :bdg-success:`Launcher > Settings > Testing`

   If true, OpenMW captures the mouse cursor.
   In GUI mode, prevents cursor from leaving the window.
   In look mode, cursor is always centered.
   Useful to disable when debugging or using multi-monitor setups.

.. omw-setting::
   :title: toggle sneak
   :type: boolean
   :range: true, false
   :default: false
   :location: :bdg-info:`Lua` :bdg-info:`In Game > Options > Scripts > OpenMW Camera`

   Sneak key toggles sneaking on/off instead of needing to be held.
   Useful for extended sneaking sessions.

.. omw-setting::
   :title: always run
   :type: boolean
   :range: true, false
   :default: false
   :location: :bdg-info:`Lua` :bdg-info:`In Game > Options > Scripts > OpenMW Controls`

   If true, character runs by default.
   Shift inverts behavior temporarily; CapsLock toggles it persistently.

.. omw-setting::
   :title: camera sensitivity
   :type: float32
   :range: > 0
   :default: 1.0
   :location: :bdg-info:`In Game > Options > Controls > Mouse/Keyboard`

   Controls mouse sensitivity outside of GUI mode.
   Does not affect GUI cursor speed.

.. omw-setting::
   :title: camera y multiplier
   :type: float32
   :range: > 0
   :default: 1.0


   Adjusts vertical camera sensitivity relative to horizontal.
   Multiplied with the camera sensitivity value.

.. omw-setting::
   :title: invert x axis
   :type: boolean
   :range: true, false
   :default: false
   :location: :bdg-info:`In Game > Options > Controls`

   Inverts horizontal camera movement outside of GUI mode.

.. omw-setting::
   :title: invert y axis
   :type: boolean
   :range: true, false
   :default: false
   :location: :bdg-info:`In Game > Options > Controls`

   Inverts vertical camera movement outside of GUI mode.

.. omw-setting::
   :title: enable controller
   :type: boolean
   :range: true, false
   :default: true
   :location: :bdg-info:`In Game > Options > Controls`

   Enables controller input (if present).
   Disable to avoid controller interference or for split-screen setups.

.. omw-setting::
   :title: gamepad cursor speed
   :type: float32
   :range: >0
   :default: 1.0

   Controls joystick-based cursor speed in GUI mode.
   Does not affect camera movement.

.. omw-setting::
   :title: joystick dead zone
   :type: float32
   :range: 0.0 to 0.5
   :default: 0.1

   Sets radius for joystick dead zones.
   Values inside the zone are ignored.
   Can be set to 0.0 when using third-party dead zone tools.

.. omw-setting::
   :title: enable gyroscope
   :type: boolean
   :range: true, false
   :default: false

   Enables camera control via gyroscope (built-in or controller-based).
   Controller gyroscopes require SDL 2.0.14+ (tested on Windows).

.. omw-setting::
   :title: gyro horizontal axis
   :type: string
   :range: x, y, z, -x, -y, -z
   :default: -x

   Sets gyroscope axis used for horizontal camera movement.
   Minus sign reverses direction.
   Axis is landscape-aligned and auto-corrects for portrait.

.. omw-setting::
   :title: gyro vertical axis
   :type: string
   :range: x, y, z, -x, -y, -z
   :default: y

   Sets gyroscope axis used for vertical camera movement.
   Minus sign reverses direction.
   Axis is landscape-aligned and auto-corrects for portrait.

.. omw-setting::
   :title: gyro input threshold
   :type: float32
   :range: ≥0
   :default: 0.0

   Sets minimum gyroscope movement value to avoid noise-based crosshair jitter.

.. omw-setting::
   :title: gyro horizontal sensitivity
   :type: float32
   :range: >0
   :default: 1.0

   Controls horizontal sensitivity for gyroscope input.
   A value of X means 1° of real-world rotation causes X° of in-game rotation.

.. omw-setting::
   :title: gyro vertical sensitivity
   :type: float32
   :range: >0
   :default: 1.0

   Controls vertical sensitivity for gyroscope input.
   A value of X means 1° of real-world rotation causes X° of in-game rotation.
