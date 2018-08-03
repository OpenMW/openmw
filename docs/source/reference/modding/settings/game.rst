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

This setting can only be configured by editing the settings configuration file.

show projectile damage
----------------------

:Type:		boolean
:Range:		True/False
:Default:	False

If this setting is true, the damage bonus of arrows and bolts will show on their tooltip.

This setting can only be configured by editing the settings configuration file.

show melee info
---------------

:Type:		boolean
:Range:		True/False
:Default:	False

If this setting is true, the reach and speed of melee weapons will show on their tooltip.

This setting can only be configured by editing the settings configuration file.

show enchant chance
-------------------

:Type:		boolean
:Range:		True/False
:Default:	False

Whether or not the chance of success will be displayed in the enchanting menu.

This setting can only be configured by editing the settings configuration file.

best attack
-----------

:Type:		boolean
:Range:		True/False
:Default:	False

If this setting is true, the player character will always use the most powerful attack when striking with a weapon
(chop, slash or thrust). If this setting is false,
the type of attack is determined by the direction that the character is moving at the time the attack begins.

This setting can be toggled with the Always Use Best Attack button in the Prefs panel of the Options menu.

can loot during death animation
-------------------------------

:Type:		boolean
:Range:		True/False
:Default:	True

If this setting is true, the player is allowed to loot actors (e.g. summoned creatures) during death animation, if they are not in combat.
However disposing corpses during death animation is not recommended - death counter may not be incremented, and this behaviour can break quests.
This is how original Morrowind behaves.

If this setting is false, player has to wait until end of death animation in all cases.
This case is more safe, but makes using of summoned creatures exploit (looting summoned Dremoras and Golden Saints for expensive weapons) a lot harder.
Conflicts with mannequin mods, which use SkipAnim to prevent end of death animation.

This setting can only be configured by editing the settings configuration file.

difficulty
----------

:Type:		integer
:Range:		-500 to 500
:Default:	0

This setting adjusts the difficulty of the game and is intended to be in the range -100 to 100 inclusive.
Given the default game setting for fDifficultyMult of 5.0,
a value of -100 results in the player taking 80% of the usual damage, doing 6 times the normal damage.
A value of 100 results in the player taking 6 times as much damage, while inflicting only 80% of the usual damage.
Values below -500 will result in the player receiving no damage,
and values above 500 will result in the player inflicting no damage.

This setting can be controlled in game with the Difficulty slider in the Prefs panel of the Options menu.

classic reflect absorb attribute behavior
-----------------------------------------

:Type:		boolean
:Range: 	True/False
:Default:	True

If this setting is true, "Absorb Attribute" spells which were reflected by the target are not "mirrored",
and the caster will absorb their own attribute resulting in no effect on both the caster and the target.
This makes the gameplay as a mage easier, but these spells become imbalanced.
This is how the original Morrowind behaves.

This setting can only be configured by editing the settings configuration file.

show effect duration
--------------------

:Type:		boolean
:Range:		True/False
:Default:	False

Show the remaining duration of magic effects and lights if this setting is true.
The remaining duration is displayed in the tooltip by hovering over the magical effect.

This setting can only be configured by editing the settings configuration file.

enchanted weapons are magical
-----------------------------

:Type:		boolean
:Range:		True/False
:Default:	True

Make enchanted weapons without Magical flag bypass normal weapons resistance (and weakness) certain creatures have.
This is how original Morrowind behaves.

This setting can only be configured by editing the settings configuration file.

prevent merchant equipping
--------------------------

:Type:		boolean
:Range:		True/False
:Default:	False

Prevent merchants from equipping items that are sold to them.

This setting can only be configured by editing the settings configuration file.

followers attack on sight
-------------------------

:Type:		boolean
:Range:		True/False
:Default:	False

Make player followers and escorters start combat with enemies who have started combat with them or the player.
Otherwise they wait for the enemies or the player to do an attack first.
Please note this setting has not been extensively tested and could have side effects with certain quests.

This setting can only be configured by editing the settings configuration file.

use additional anim sources
---------------------------

:Type:		boolean
:Range:		True/False
:Default:	False

Allow to load additional animation sources when enabled.
For example, if the main animation mesh has name Meshes/x.nif, an engine will load all KF-files from Animations/x folder and its child folders.
Can be useful if you want to use several animation replacers without merging them.
Attention: animations from AnimKit have own format and are not supposed to be directly loaded in-game!
This setting can only be configured by editing the settings configuration file.

barter disposition change is permanent
--------------------------------------

:Type:		boolean
:Range:		True/False
:Default:	False

If this setting is true, disposition change of merchants caused by trading will be permanent and won't be discarded upon exiting dialogue with them.
This imitates the option Morrowind Code Patch offers.

This setting can be toggled with a checkbox in Advanced tab of the launcher.
