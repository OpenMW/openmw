Input Settings
##############

grab cursor
-----------

:Type:		boolean
:Range:		True/False
:Default:	True

OpenMW will capture control of the cursor if this setting is true.

In "look mode", OpenMW will center the cursor regardless of the value of this setting
(since the cursor/crosshair is always centered in the OpenMW window).
However, in GUI mode, this setting determines the behavior when the cursor is moved outside the OpenMW window.
If true, the cursor movement stops at the edge of the window preventing access to other applications.
If false, the cursor is allowed to move freely on the desktop.

This setting does not apply to the screen where escape has been pressed, where the cursor is never captured.
Regardless of this setting "Alt-Tab" or some other operating system dependent key sequence can be used
to allow the operating system to regain control of the mouse cursor.
This setting interacts with the minimize on focus loss setting by affecting what counts as a focus loss.
Specifically on a two-screen configuration it may be more convenient to access the second screen with setting disabled.

Note for developers: it's desirable to have this setting disabled when running the game in a debugger,
to prevent the mouse cursor from becoming unusable when the game pauses on a breakpoint.

This setting can only be configured by editing the settings configuration file.

toggle sneak
------------

:Type:		boolean
:Range:		True/False
:Default:	False

This setting causes the behavior of the sneak key (bound to Ctrl by default)
to toggle sneaking on and off rather than requiring the key to be held down while sneaking.
Players that spend significant time sneaking may find the character easier to control with this option enabled.

This setting can only be configured by editing the settings configuration file.

always run
----------

:Type:		boolean
:Range:		True/False
:Default:	False

If this setting is true, the character is running by default, otherwise the character is walking by default.
The shift key will temporarily invert this setting, and the caps lock key will invert this setting while it's "locked".
This setting is updated every time you exit the game,
based on whether the caps lock key was on or off at the time you exited.

This settings can be toggled in game by pressing the CapsLock key and exiting.

camera sensitivity
------------------

:Type:		floating point
:Range:		> 0
:Default:	1.0

This setting controls the overall camera/mouse sensitivity when not in GUI mode.
The default sensitivity is 1.0, with smaller values requiring more mouse movement,
and larger values requiring less.
This setting does not affect mouse speed in GUI mode,
which is instead controlled by your operating system mouse speed setting.

This setting can be changed with the Camera Sensitivity slider in the Controls panel of the Options menu.

camera y multiplier
-------------------

:Type:		floating point
:Range:		> 0
:Default:	1.0

This setting controls the vertical camera/mouse sensitivity relative to the horizontal sensitivity
(see camera sensitivity above). It is multiplicative with the previous setting,
meaning that it should remain set at 1.0 unless the player desires to have different sensitivities in the two axes.

This setting can only be configured by editing the settings configuration file.

invert x axis
-------------

:Type:      boolean
:Range:     True/False
:Default:   False


Invert the horizontal axis while not in GUI mode.
If this setting is true, moving the mouse to the left will cause the view to rotate counter-clockwise,
while moving it to the right will cause the view to rotate clockwise. This setting does not affect cursor movement in GUI mode.

This setting can be toggled in game with the Invert X Axis button in the Controls panel of the Options menu.

invert y axis
-------------

:Type:		boolean
:Range:		True/False
:Default:	False

Invert the vertical axis while not in GUI mode.
If this setting is true, moving the mouse away from the player will look down,
while moving it towards the player will look up. This setting does not affect cursor movement in GUI mode.

This setting can be toggled in game with the Invert Y Axis button in the Controls panel of the Options menu.

enable controller
-----------------

:Type:		boolean
:Range:		True/False
:Default:	True

Enable support of controller input — or rather not ignore controller events,
which are always sent if a controller is present and detected.
Disabling this setting can be useful for working around controller-related issues or for setting up split-screen gameplay configurations.

This setting can be toggled in game with the Enable Joystick button in the Controls panel of the Options menu.

gamepad cursor speed
--------------------

:Type: float
:Range: >0
:Default: 1.0

This setting controls the speed of the cursor within GUI mode when using the joystick.
This setting has no effect on the camera rotation speed, which is controlled by the
camera sensitivity setting.

This setting can only be configured by editing the settings configuration file.

enable gyroscope
----------------

:Type:		boolean
:Range:		True/False
:Default:	False

Enable the support of camera rotation based on the information supplied from the gyroscope through SDL.

This setting can only be configured by editing the settings configuration file.

gyro horizontal axis
--------------------

:Type:      string
:Range:     x, y, z, -x, -y, -z
:Default:   -x

This setting sets up an axis of the gyroscope as the horizontal camera axis.
Minus sign swaps the positive and negative direction of the axis.
Keep in mind that while this setting corresponds to the landscape mode of the display,
the portrait mode or any other mode will have this axis corrected automatically.

This setting can only be configured by editing the settings configuration file.

gyro vertical axis
------------------

:Type:      string
:Range:     x, y, z, -x, -y, -z
:Default:   y

This setting sets up an axis of the gyroscope as the vertical camera axis.
Minus sign swaps the positive and negative direction of the axis.
Keep in mind that while this setting corresponds to the landscape mode of the display,
the portrait mode or any other mode will have this axis corrected automatically.

This setting can only be configured by editing the settings configuration file.

gyro input threshold
--------------------

:Type:		floating point
:Range:		> 0
:Default:	0.01

This setting determines the minimum value of the rotation that will be accepted.
It allows to avoid crosshair oscillation due to gyroscope "noise".

This setting can only be configured by editing the settings configuration file.

gyro horizontal sensitivity
---------------------------

:Type: float
:Range: >0
:Default: 1.0

This setting controls the overall gyroscope horizontal sensitivity.
The smaller this sensitivity is, the less visible effect the device rotation
will have on the horizontal camera rotation, and vice versa.

This setting can only be configured by editing the settings configuration file.

gyro vertical sensitivity
-------------------------

:Type: float
:Range: >0
:Default: 1.0

This setting controls the overall gyroscope vertical sensitivity.
The smaller this sensitivity is, the less visible effect the device
rotation will have on the vertical camera rotation, and vice versa.

This setting can only be configured by editing the settings configuration file.
