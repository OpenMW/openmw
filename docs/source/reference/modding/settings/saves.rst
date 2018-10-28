Saves Settings
##############

character
---------

:Type:		string
:Range:
:Default:	""

This contains the default character for the Load Game menu and is automatically updated when a different character is selected.

autosave
--------

:Type:		boolean
:Range:		True/False
:Default:	True

This setting determines whether the game will be automatically saved when the character rests.

This setting can be toggled in game with the Auto-Save when Rest button in the Prefs panel of the Options menu.

timeplayed
----------

:Type:		boolean
:Range:		True/False
:Default:	False

This setting determines whether the amount of the time the player has spent playing will be displayed
for each saved game in the Load menu. Currently, the counter includes time spent in menus, including the pause menu, 
but does not include time spent with the game window minimized.

This setting can only be configured by editing the settings configuration file.

max quicksaves
--------------

:Type:		integer
:Range:		>0
:Default:	1

This setting determines how many quicksave and autosave slots you can have at a time.  If greater than 1, 
quicksaves will be sequentially created each time you quicksave.  Once the maximum number of quicksaves has been reached, 
the oldest quicksave will be recycled the next time you perform a quicksave.

This setting can only be configured by editing the settings configuration file.
