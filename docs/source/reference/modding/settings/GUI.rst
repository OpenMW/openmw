GUI Settings
############

scaling factor
--------------

:Type:		floating point
:Range:		> 0.0
:Default:	1.0

This setting scales the GUI interface windows.
A value of 1.0 results in the normal scale. Larger values are useful to increase the scale of the GUI for high resolution displays.
This setting can only be configured by editing the settings configuration file.

menu transparency
-----------------

:Type:		floating point
:Range:		0.0 (transparent) to 1.0 (opaque)
:Default:	0.84

This setting controls the transparency of the GUI windows.
This setting can be adjusted in game with the Menu Transparency slider in the Prefs panel of the Options menu.

tooltip delay
-------------

:Type:		floating point
:Range:		> 0.0
:Default:	0.0

This value determines the number of seconds between when you begin hovering over an item and when its tooltip appears.
This setting only affects the tooltip delay for objects under the crosshair in GUI mode windows.

The tooltip displays context sensitive information on the selected GUI element,
such as weight, value, damage, armor rating, magical effects, and detailed description.

This setting can be adjusted between 0.0 and 1.0 in game
with the Menu Help Delay slider in the Prefs panel of the Options menu.

stretch menu background
-----------------------

:Type:		boolean
:Range:		True/False
:Default:	False

Stretch or shrink the main menu screen, loading splash screens, introductory movie,
and cut scenes to fill the specified video resolution, distorting their aspect ratio.
The Bethesda provided assets have a 4:3 aspect ratio, but other assets are permitted to have other aspect ratios.
If this setting is false, the assets will be centered in their correct aspect ratio,
with black bars filling the remainder of the screen.

This setting can only be configured by editing the settings configuration file.

subtitles
---------

:Type:		boolean
:Range:		True/False
:Default:	False

Enable or disable subtitles for NPC spoken dialog (and some sound effects).
Subtitles will appear in a tool tip box in the lower center of the screen.

This setting can be toggled in game with the Subtitles button in the Prefs panel of Options menu.

hit fader
---------

:Type:		boolean
:Range:		True/False
:Default:	True

This setting enables or disables the "red flash" overlay that provides a visual clue when the character has taken damage.

If this setting is disabled, the player will "bleed" like NPCs do.

This setting can only be configured by editing the settings configuration file.

werewolf overlay
----------------

:Type:		boolean
:Range:		True/False
:Default:	True

Enable or disable the werewolf visual effect in first-person mode.

This setting can only be configured by editing the settings configuration file.

color background owned
----------------------

:Type:		RGBA floating point
:Range:		0.0 to 1.0
:Default:	0.15 0.0 0.0 1.0 (dark red)

The following two settings determine the background color of the tool tip and the crosshair
when hovering over an item owned by an NPC.
The color definitions are composed of four floating point values between 0.0 and 1.0 inclusive,
representing the red, green, blue and alpha channels. The alpha value is currently ignored.
The crosshair color will have no effect if the crosshair setting in the HUD section is disabled.

This setting can only be configured by editing the settings configuration file.
This setting has no effect if the show owned setting in the Game Settings Section is false.

color crosshair owned
---------------------

:Type:		RGBA floating point
:Range:		0.0 to 1.0
:Default:	1.0 0.15 0.15 1.0 (bright red)

This setting sets the color of the crosshair when hovering over an item owned by an NPC.
The value is composed of four floating point values representing the red, green, blue and alpha channels.
The alpha value is currently ignored.

This setting can only be configured by editing the settings configuration file.
This setting has no effect if the crosshair setting in the HUD Settings Section is false.
This setting has no effect if the show owned setting in the Game Settings Section is false.
