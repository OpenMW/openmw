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

allow third person zoom
-----------------------

:Type:		boolean
:Range:		True/False
:Default:	False

Allow zooming in and out using the middle mouse wheel in third person view.
This feature may not work correctly if the mouse wheel is bound to other actions,
and may be triggered accidentally in some cases, so is disabled by default.
This setting can only be configured by editing the settings configuration file.

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

invert y axis
-------------

:Type:		boolean
:Range:		True/False
:Default:	False

Invert the vertical axis while not in GUI mode.
If this setting is true, moving the mouse away from the player will look down,
while moving it towards the player will look up. This setting does not affect cursor movement in GUI mode.

This setting can be toggled in game with the Invert Y Axis button in the Controls panel of the Options menu.
