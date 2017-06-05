Game Settings
#############

show owned
----------

:Type:		integer
:Range:		0, 1, 2, 3
:Default:	0

Enable visual clues for items owned by NPCs when the crosshair is on the object.
If the setting is 0, no clues are provided which is the default Morrowind behavior.
If the setting is 1, the background of the tool tip for the object is highlighted
in the color specified by the color background owned setting in the GUI Settings Section.
If the setting is 2, the crosshair is the color of the color crosshair owned setting in the GUI Settings section.
If the setting is 3, both the tool tip background and the crosshair are colored.
The crosshair is not visible if crosshair is false.

The default value is 0 (no clues). This setting can only be configured by editing the settings configuration file.

show projectile damage
----------------------

:Type:		boolean
:Range:		True/False
:Default:	False

If this setting is true, damage bonus of arrows and bolts will be showed on item tooltip.

The default value is false. This setting can only be configured by editing the settings configuration file.

show melee info
---------------

:Type:		boolean
:Range:		True/False
:Default:	False

If this setting is true, melee weapons reach and speed will be showed on item tooltip.

The default value is false. This setting can only be configured by editing the settings configuration file.

show enchant chance
-------------------

:Type:		boolean
:Range:		True/False
:Default:	False

Whether or not the chance of success will be displayed in the enchanting menu.

The default value is false. This setting can only be configured by editing the settings configuration file.

best attack
-----------

:Type:		boolean
:Range:		True/False
:Default:	False

If this setting is true, the player character will always use the most powerful attack when striking with a weapon
(chop, slash or thrust). If this setting is false,
the type of attack is determined by the direction that the character is moving at the time the attack begins.

The default value is false.
This setting can be toggled with the Always Use Best Attack button in the Prefs panel of the Options menu.

difficulty
----------

:Type:		integer
:Range:		-500 to 500
:Default:	0

This setting adjusts the difficulty of the game and is intended to be in the range -100 to 100 inclusive.
Given the default game setting for fDifficultyMult of 5.0,
a value of -100 results in the player taking 80% of the usual damage, doing 6 times the normal damage.
A value of 100 results in the player taking 6 times as much damage, but inflicting only 80% of the usual damage.
Values less than -500 will result in the player receiving no damage,
and values greater than 500 will result in the player inflicting no damage.

This setting can be controlled in game with the Difficulty slider in the Prefs panel of the Options menu.

show effect duration
--------------------

:Type:		boolean
:Range:		True/False
:Default:	False

Show the remaining duration of magic effects and lights if this setting is true.
The remaining duration is displayed in the tooltip by hovering over the magical effect.

The default value is false. This setting can only be configured by editing the settings configuration file.

prevent merchant equipping
--------------------------

:Type:		boolean
:Range:		True/False
:Default:	False

Prevents merchants from equipping items that are sold to them.

followers attack on sight
-------------------------

:Type:		boolean
:Range:		True/False
:Default:	False

Makes player followers and escorters start combat with enemies who have started combat with them or the player.
Otherwise they wait for the enemies or the player to do an attack first.