GUI Settings
############

scaling factor
--------------

:Type:		floating point
:Range:		0.5 to 8.0
:Default:	1.0

This setting scales GUI windows.
A value of 1.0 results in the normal scale. Larger values are useful to increase the scale of the GUI for high resolution displays.

This setting can be configured in the Interface section of Advanced tab of the launcher.

font size
---------

:Type:		integer
:Range:		12 to 20
:Default:	16

Allows to specify glyph size for in-game fonts.
Note: default bitmap fonts are supposed to work with 16px size, otherwise glyphs will be blurry.
TrueType fonts do not have this issue.

ttf resolution
--------------

:Type:		integer
:Range:		48 to 960
:Default:	96

Allows to specify resolution for in-game TrueType fonts.
Note: actual resolution depends on "scaling factor" setting value, this value is for 1.0 scaling factor.

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

This setting can be configured in the Interface section of Advanced tab of the launcher.

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

color topic enable
------------------

:Type:      boolean
:Range:		True/False
:Default:	False

This setting controls whether the topics available in the dialogue topic list are coloured according to their state.
See 'color topic specific' and 'color topic exhausted' for details.

color topic specific
--------------------

:Type:		RGBA floating point
:Range:		0.0 to 1.0
:Default:	empty

This setting overrides the colour of dialogue topics that have a response unique to the actors speaking.
The value is composed of four floating point values representing the red, green, blue and alpha channels.
The alpha value is currently ignored.

A topic response is considered unique if its Actor filter field contains the speaking actor's object ID and hasn't yet been read.

color topic exhausted
---------------------

:Type:		RGBA floating point
:Range:		0.0 to 1.0
:Default:	empty

This setting overrides the colour of dialogue topics which have been "exhausted" by the player.
The value is composed of four floating point values representing the red, green, blue and alpha channels.
The alpha value is currently ignored.

A topic is considered "exhausted" if the response the player is about to see has already been seen.
