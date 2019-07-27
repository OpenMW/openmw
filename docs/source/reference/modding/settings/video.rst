Video Settings
##############

resolution x
------------

:Type:		integer
:Range:		> 0
:Default:	800

This setting determines the horizontal resolution of the OpenMW game window.
Larger values produce more detailed images within the constraints of your graphics hardware,
but may reduce the frame rate.

The window resolution can be selected from a menu of common screen sizes
in the Video tab of the Video Panel of the Options menu, or in the Graphics tab of the OpenMW Launcher.
The horizontal resolution can also be set to a custom value in the Graphics tab of the OpenMW Launcher.

resolution y
------------

:Type:		integer
:Range:		> 0
:Default:	600

This setting determines the vertical resolution of the OpenMW game window.
Larger values produce more detailed images within the constraints of your graphics hardware,
but may reduce the frame rate.

The window resolution can be selected from a menu of common screen sizes
in the Video tab of the Video Panel of the Options menu, or in the Graphics tab of the OpenMW Launcher.
The vertical resolution can also be set to a custom value in the Graphics tab of the OpenMW Launcher.

fullscreen
----------

:Type:		boolean
:Range:		True/False
:Default:	False

This setting determines whether the entire screen is used for the specified resolution.

This setting can be toggled in game using the Fullscreen button in the Video tab of the Video panel in the Options menu.
It can also be toggled with the Full Screen check box in the Graphics tab of the OpenMW Launcher.

screen
------

:Type:		integer
:Range:		>= 0
:Default:	0

This setting determines which screen the game will open on in multi-monitor configurations.
This setting is particularly important when the fullscreen setting is true,
since this is the only way to control which screen is used,
but it can also be used to control which screen a normal window or a borderless window opens on as well.
The screens are numbered in increasing order, beginning with 0.

This setting can be selected from a pull down menu in the Graphics tab of the OpenMW Launcher,
but cannot be changed during game play.

minimize on focus loss
----------------------

:Type:		boolean
:Range:		True/False
:Default:	True

Minimize the OpenMW window if it loses cursor focus. This setting is primarily useful for single screen configurations,
so that the OpenMW screen in full screen mode can be minimized
when the operating system regains control of the mouse and keyboard.
On multiple screen configurations, disabling this option makes it easier to switch between screens while playing OpenMW.

.. Note::
	A minimized game window consumes less system resources and produces less heat,
	since the game does not need to render in minimized state.
	It is therefore advisable to minimize the game during pauses
	(either via use of this setting, or by minimizing the window manually).

This setting has no effect if the fullscreen setting is false.

Developer note: corresponds to SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS.

This setting can only be configured by editing the settings configuration file.

window border
-------------

:Type:		boolean
:Range:		True/False
:Default:	True

This setting determines whether there's an operating system border drawn around the OpenMW window.
If this setting is true, the window can be moved and resized with the operating system window controls.
If this setting is false, the window has no operating system border.

This setting has no effect if the fullscreen setting is true.

This setting can be toggled in game using the Window Border button
in the Video tab of the Video panel in the Options menu.
It can also be toggled with the Window Border check box in the OpenMW Launcher.

antialiasing
------------

:Type:		integer
:Range:		0, 2, 4, 8, 16
:Default:	0

This setting controls anti-aliasing. Anti-aliasing is a technique designed to improve the appearance of polygon edges,
so they do not appear to be "jagged".
Anti-aliasing can smooth these edges at the cost of a minor reduction in the frame rate.
A value of 0 disables anti-aliasing.
Other powers of two (e.g. 2, 4, 8, 16) are supported according to the capabilities of your graphics hardware.
Higher values do a better job of smoothing out the image but have a greater impact on frame rate.

This setting can be configured from a list of valid choices in the Graphics panel of the OpenMW Launcher,
but cannot be changed during game play
due to a technical limitation that may be addressed in a future version of OpenMW.

vsync
-----

:Type:		boolean
:Range:		True/False
:Default:	False

This setting determines whether frame draws are synchronized with the vertical refresh rate of your monitor.
Enabling this setting can reduce screen tearing,
a visual defect caused by updating the image buffer in the middle of a screen draw.
Enabling this option typically implies limiting the framerate to the refresh rate of your monitor,
but may also introduce additional delays caused by having to wait until the appropriate time
(the vertical blanking interval) to draw a frame, and a loss in mouse responsiveness known as 'input lag'.

This setting can be adjusted in game using the VSync button in the Video tab of the Video panel in the Options menu.
It can also be changed by toggling the Vertical Sync check box in the Graphics tab of the OpenMW Launcher.

framerate limit
---------------

:Type:		floating point
:Range:		>= 0.0
:Default:	300

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

contrast
--------

:Type:		floating point
:Range:		> 0.0
:Default:	1.0

This setting controls the contrast correction for all video in the game.

This setting can only be configured by editing the settings configuration file. 
It has been reported to not work on some Linux systems.

gamma
-----

:Type:		floating point
:Range:		> 0.0
:Default:	1.0

This setting controls the gamma correction for all video in the game.
Gamma is an exponent that makes colors brighter if greater than 1.0 and darker if less than 1.0.

This setting can be changed in the Detail tab of the Video panel of the Options menu.
It has been reported to not work on some Linux systems, 
and therefore the in-game setting in the Options menu has been disabled on Linux systems.
