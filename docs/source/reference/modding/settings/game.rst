Game Settings
#############

show owned
----------

:Type:		integer
:Range:		0, 1, 2, 3
:Default:	0

Enable visual clues for items owned by NPCs when the crosshair is on the object.
If the setting is 0, no clues are provided which is the default Morrowind behaviour.
If the setting is 1, the background of the tool tip for the object is highlighted
in the colour specified by the colour background owned setting in the GUI Settings Section.
If the setting is 2, the crosshair is the colour of the colour crosshair owned setting in the GUI Settings section.
If the setting is 3, both the tool tip background and the crosshair are coloured.
The crosshair is not visible if crosshair is false.

This setting can be configured in Advanced tab of the launcher.

show projectile damage
----------------------

:Type:		boolean
:Range:		True/False
:Default:	False

If this setting is true, the damage bonus of arrows and bolts will show on their tooltip.

This setting can be toggled in Advanced tab of the launcher.

show melee info
---------------

:Type:		boolean
:Range:		True/False
:Default:	False

If this setting is true, the reach and speed of weapons will show on their tooltip.

This setting can be toggled in Advanced tab of the launcher.

show enchant chance
-------------------

:Type:		boolean
:Range:		True/False
:Default:	False

Whether or not the chance of success will be displayed in the enchanting menu.

This setting can be toggled in Advanced tab of the launcher.

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

If this setting is true, the player is allowed to loot actors (e.g. summoned creatures) during death animation, 
if they are not in combat. In this case we have to increment death counter and run disposed actor's script instantly.

If this setting is false, player has to wait until end of death animation in all cases.
Makes using of summoned creatures exploit (looting summoned Dremoras and Golden Saints for expensive weapons) a lot harder.
Conflicts with mannequin mods, which use SkipAnim to prevent end of death animation.

This setting can be toggled in Advanced tab of the launcher.

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

actors processing range
-----------------------

:Type:		integer
:Range:		3584 to 7168
:Default:	7168

This setting specifies the actor state update distance from the player in game units.
Actor state update includes AI, animations, and physics processing.
Actors close to this distance softly fade in and out instead of appearing or disappearing abruptly.
Keep in mind that actors running Travel AI packages are always active to avoid
issues in mods with long-range AiTravel packages (for example, patrols, caravans and travellers).

This setting can be controlled in game with the "Actors Processing Range" slider in the Prefs panel of the Options menu.

classic reflected absorb spells behavior
----------------------------------------

:Type:		boolean
:Range: 	True/False
:Default:	True

If this setting is true, effects of Absorb spells which were reflected by the target are not mirrored,
and the caster will absorb their own stat resulting in no effect on either the caster and the target.
This makes the gameplay as a mage easier, but these spells become imbalanced.
This is how Morrowind behaves.

This setting can be toggled in Advanced tab of the launcher.

use magic item animations
-------------------------

:Type:		boolean
:Range: 	True/False
:Default:	False

If this setting is true, the engine will use casting animations for magic items, including scrolls.
Otherwise, there will be no casting animations, just as in original engine

This setting can only be configured by editing the settings configuration file.

show effect duration
--------------------

:Type:		boolean
:Range:		True/False
:Default:	False

Show the remaining duration of magic effects and lights if this setting is true.
The remaining duration is displayed in the tooltip by hovering over the magical effect.

This setting can be toggled in Advanced tab of the launcher.

enchanted weapons are magical
-----------------------------

:Type:		boolean
:Range:		True/False
:Default:	True

Make enchanted weapons without Magical flag bypass normal weapons resistance (and weakness) certain creatures have.
This is how Morrowind behaves.

This setting can be toggled in Advanced tab of the launcher.

prevent merchant equipping
--------------------------

:Type:		boolean
:Range:		True/False
:Default:	False

Prevent merchants from equipping items that are sold to them.

This setting can be toggled in Advanced tab of the launcher.

followers attack on sight
-------------------------

:Type:		boolean
:Range:		True/False
:Default:	False

Make player followers and escorters start combat with enemies who have started combat with them or the player.
Otherwise they wait for the enemies or the player to do an attack first.
Please note this setting has not been extensively tested and could have side effects with certain quests.
This setting can be toggled in Advanced tab of the launcher.

shield sheathing
----------------

:Type:		boolean
:Range:		True/False
:Default:	False

If this setting is true, OpenMW will utilize shield sheathing-compatible assets to display holstered shields.

To make use of this, you need to have an xbase_anim_sh.nif file with weapon bones that will be injected into the skeleton.
Also you can use additional _sh meshes for more precise shield placement.
Warning: this feature may conflict with mods that use pseudo-shields to emulate item in actor's hand (e.g. books, baskets, pick axes).
To avoid conflicts, you can use _sh mesh without "Bip01 Sheath" node for such "shields" meshes, or declare its bodypart as Clothing type, not as Armor.
Also you can use an _sh node with empty "Bip01 Sheath" node.
In this case the engine will use basic shield model, but will use transformations from the "Bip01 Sheath" node.

weapon sheathing
----------------

:Type:		boolean
:Range:		True/False
:Default:	False

If this setting is true, OpenMW will utilize weapon sheathing-compatible assets to display holstered weapons.

To make use of this, you need to have an xbase_anim_sh.nif file with weapon bones that will be injected into the skeleton.
Additional _sh suffix models are not essential for weapon sheathing to work but will act as quivers or scabbards for the weapons they correspond to.

use additional anim sources
---------------------------

:Type:		boolean
:Range:		True/False
:Default:	False

Allow the engine to load additional animation sources when enabled.
For example, if the main animation mesh has name Meshes/x.nif, 
the engine will load all KF-files from Animations/x folder and its child folders.
This can be useful if you want to use several animation replacers without merging them.
Attention: animations from AnimKit have their own format and are not supposed to be directly loaded in-game!
This setting can only be configured by editing the settings configuration file.

barter disposition change is permanent
--------------------------------------

:Type:		boolean
:Range:		True/False
:Default:	False

If this setting is true, 
disposition change of merchants caused by trading will be permanent and won't be discarded upon exiting dialogue with them.
This imitates the option that Morrowind Code Patch offers.

This setting can be toggled in Advanced tab of the launcher.

only appropriate ammunition bypasses resistance
-----------------------------------------------

:Type:		boolean
:Range:		True/False
:Default:	False

If this setting is true, you will have to use the appropriate ammunition to bypass normal weapon resistance (or weakness).
An enchanted bow with chitin arrows will no longer be enough for the purpose, while a steel longbow with glass arrows will still work.
This was previously the default engine behavior that diverged from Morrowind design.

This setting can be toggled in Advanced tab of the launcher.

strength influences hand to hand
--------------------------------

:Type:		integer
:Range:		0, 1, 2
:Default:	0

This setting controls the behavior of factoring of Strength attribute into hand-to-hand damage, which is using the formula
Morrowind Code Patch uses for its equivalent feature: damage is multiplied by its value divided by 40.

0: Strength attribute is ignored
1: Strength attribute is factored in damage from any actor
2: Strength attribute is factored in damage from any actor except werewolves

This setting can be controlled in Advanced tab of the launcher.

normalise race speed
--------------------

:Type:		boolean
:Range:		True/False
:Default:	False

By default race weight is factored into horizontal movement speed like in Morrowind.
For example, an NPC which has 1.2 race weight is faster than an NPC with the exact same stats and weight 1.0 by a factor of 120%.
If this setting is true, race weight is ignored in the calculations which allows for a movement behavior
equivalent to the one introduced by the equivalent Morrowind Code Patch feature.
This makes the movement speed behavior more fair between different races.

This setting can be controlled in Advanced tab of the launcher.

projectiles enchant multiplier
------------------------------

:Type:		floating point
:Range:		0.0 to 1.0
:Default:	0.0

The value of this setting determines how many projectiles (thrown weapons, arrows and bolts) you can enchant at once according to the following formula:

count = (soul gem charge * projectiles enchant multiplier) / enchantment strength

A value of 0 means that you can only enchant one projectile.
If you want to have Morrowind Code Patch-like count of projectiles being enchanted at once, set this value to 0.25 (i.e. 25% of the charge).

This setting can only be configured by editing the settings configuration file.

uncapped damage fatigue
-----------------------

:Type:		boolean
:Range:		True/False
:Default:	False

There are four ways to decrease an actor's Fatigue stat in Morrowind gameplay mechanics:
Drain, Absorb, Damage Fatigue magic effects and hand-to-hand combat.
However, in Morrowind you can't knock down an actor with a Damage Fatigue spell or an Absorb Fatigue spell.
Morrowind Code Patch adds an option to make it possible for Damage Fatigue spells. This is the equivalent of that option.

Setting the value of this setting to true will remove the 0 lower cap from the value,
allowing Damage Fatigue to reduce Fatigue to a value below zero.

This setting can be controlled in Advanced tab of the launcher.
