#############
Sound Effects
#############

Every game needs sound effects to create the best possible player experience.
Audio files are played during a variety of in-game situations and this is how
they are set up in OpenMW. Some are user-definable, while a lot of them are
hardcoded to specific names.


Supported Formats
*****************

OpenMW uses `FFmpeg <https://ffmpeg.org/>`_ and thus supports many formats. 
For sound effects we suggest using 16-bit ``.wav`` files.


Sound File, Sound, Sound Gen
****************************

* `Sound File` record in OpenMW-CS is the audio file on the disk. When files are placed in ``data/sound`` and its subfolders, OpenMW-CS will list them in a table. `Sound File` records aren't used directly in OpenMW-CS but must first be assigned to a `Sound` record. 
* `Sound` is the record that can be assigned to other records in OpenMW-CS. It needs a `Sound File` assigned, has volume, min range, and max range properties.
* `Sound Generator` record is used for creature animation events (groan, get hit, die, footsteps). It takes the `Sound` record and defines what creature uses this effect and when.

.. note:: Newly created `Sound` records in OpenMW-CS have their `Max Range` value set to 255.
    This can cause the sound to not be audible in-game. To fix this, set the mentioned value to 0.


Hardcoded Effects
*****************

The following sound effects are hardcoded. They require a properly named `Sound` record and will then be used in their relevant in-game situations. 

Ambient
=======

.. list-table:: 
   :widths: 35 65
   :header-rows: 1

   * - Sound name
     - Situation used
   * - ``ashstorm``
     - During an ash storm
   * - ``blight``
     - During the red blight storm
   * - ``blizzard``
     - During a blizzard
   * - ``rain``
     - During rainy weather
   * - ``rain heavy``
     - During stormy weather
   * - ``thunder0``
     - Played randomly during stormy weather
   * - ``thunder1``
     - Played randomly during stormy weather
   * - ``thunder2``
     - Played randomly during stormy weather
   * - ``thunder3``
     - Played randomly during stormy weather
   * - ``underwater``
     - When the camera is under the water level
   * - ``water layer``
     - When the player is near the water level

.. note:: Names for these sounds can be changed in ``openmw.cfg`` but we strongly suggest they stay the same.
    Another issue is with ``ashstorm``, ``blight``, ``blizzard`` which have missing or erroneous fallback lines,
    unless they were imported from Morrowind.
    These will be fixed with the planned dehardcoding of weather types and using Lua.


Equiping items and weapons
==========================

Sound names in this category follow a pattern.

* Each name contains the type of object this sound applies to.
* Names ending with ``up`` are used when the item type is picked up, equiped, readied, or grabbed in the inventory.
* Names ending with ``down`` are used when the item type is dropped, unequiped, put away, or released in the inventory.

.. list-table:: 
   :widths: 35
   :header-rows: 1

   * - Sound name
   * - ``item ammo up``
   * - ``item ammo down``
   * - ``item apparatus up`` 
   * - ``item apparatus down``
   * - ``item armor heavy up``
   * - ``item armor heavy down``
   * - ``item armor light up``
   * - ``item armor light down``
   * - ``item armor medium up``
   * - ``item armor medium down``
   * - ``item bodypart up``
   * - ``item bodypart down``
   * - ``item book up``
   * - ``item book down``
   * - ``item clothes up``
   * - ``item clothes down``
   * - ``item gold up``
   * - ``item gold down``
   * - ``item ingredient up``
   * - ``item ingredient down``
   * - ``item lockpick up``
   * - ``item lockpick down``
   * - ``item misc up``
   * - ``item misc down``
   * - ``item potion up``
   * - ``item potion down``
   * - ``item probe up``
   * - ``item probe down``
   * - ``item repair up``
   * - ``item repair down``
   * - ``item ring up``
   * - ``item ring down``
   * - ``item weapon blunt up``
   * - ``item weapon blunt down``
   * - ``item weapon bow up``
   * - ``item weapon bow down``
   * - ``item weapon crossbow up``
   * - ``item weapon crossbow down``
   * - ``item weapon longblade up``
   * - ``item weapon longblade down``
   * - ``item weapon shortblade up``
   * - ``item weapon shortblade down``
   * - ``item weapon spear up``
   * - ``item weapon spear down``


Combat
======

.. list-table:: 
   :widths: 35 65
   :header-rows: 1

   * - Sound name
     - Situation used
   * - ``bowpull``
     - Start of a bow attack 
   * - ``bowshoot``
     - A bow is released and fires an arrow
   * - ``critical damage``
     - Critial damage is dealt
   * - ``crossbowpull``
     - Start of a crossbow attack
   * - ``crossbowshoot``
     - A crossbow is released and fires a bolt
   * - ``hand to hand hit``
     - Hitting a valid target with fists
   * - ``hand to hand hit 2``
     - Hitting a valid target with fists
   * - ``heavy armor hit``
     - A character wearing heavy armour is hit   
   * - ``light armor hit``
     - A character wearing light armor is hit
   * - ``medium armor hit``
     - A character wearing medium armor is hit
   * - ``miss``
     - An attack misses
   * - ``weapon swish``
     - Melee or thrown weapon attack. The sound is modulated based on attack strength.


UI
==

.. list-table:: 
   :widths: 35 65
   :header-rows: 1

   * - Sound name
     - Situation used
   * - ``book close``
     - A book or journal is closed
   * - ``book open``
     - A book or journal is opened
   * - ``book page``
     - Go to the previous page of a book or journal
   * - ``book page2``
     - Go to the next page of a book or journal
   * - ``menu click``
     - Mouse click on a button
   * - ``scroll``
     - book object of a scroll type is opened or closed


Movement
========

Movement sounds apply to the player and NPCs, together reffered to as characters.
Each sound is played in a specific in-game situation and requires a textkey.

* When the sound name ends in ``left`` it plays when ``soundgen: left`` occurs.
* When the sound name ends in ``right`` it plays when ``soundgen: right`` occurs.
* Landing requires ``soundgen: land``

.. list-table:: 
   :widths: 25 75
   :header-rows: 1

   * - Sound name
     - Situation used
   * - ``defaultland``
     - When a character lands on the ground
   * - ``defaultlandwater``
     - When a character lands in water
   * - ``footbareleft``
     - While walking, running, or sneaking on land and not wearing armour
   * - ``footbareright``
     - While walking, running, or sneaking on land and not wearing armour
   * - ``footheavyleft``
     - While walking, running, or sneaking on land and wearing heavy armour boots
   * - ``footheavyright``
     - While walking, running, or sneaking on land and wearing heavy armour boots
   * - ``footlightleft``
     - While walking, running, or sneaking on land and wearing light armour boots
   * - ``footlightright``
     - While walking, running, or sneaking on land and wearing light armour boots
   * - ``footmedleft``
     - While walking, running, or sneaking on land and wearing medium armour boots
   * - ``footmedright``
     - While walking, running, or sneaking on land and wearing medium armour boots
   * - ``footwaterleft``
     - While walking, running, or sneaking in shallow water
   * - ``footwaterright``
     - While walking, running, or sneaking in shallow water
   * - ``swim left``
     - When swimming
   * - ``swim right``
     - When swimming


Interactions
============

.. list-table:: 
   :widths: 35 65
   :header-rows: 1

   * - Sound name
     - Situation used
   * - ``disarm trap``
     - A trap is successfully disarmed
   * - ``disarm trap fail``
     - Attempt to disarm a trap is unsuccessful
   * - ``enchant fail``
     - Enchanting or recharging an item fails
   * - ``enchant success``
     - Enchanting or recharging an item is successful
   * - ``lockedchest``
     - Trying to open a locked chest
   * - ``lockeddoor``
     - Trying to open locked doors
   * - ``open lock``
     - Unlock attempt succeeds
   * - ``open lock fail``
     - Unlock attempt fails 
   * - ``potion fail``
     - Brewing a potion fails
   * - ``potion success``
     - Brewing a potion succeeds
   * - ``repair``
     - Repair of an item is successful
   * - ``repair fail``
     - Repair attempt of an item fails
   * - ``spellmake fail``
     - Attempt at creating a new spell fails
   * - ``spellmake success``
     - Creating a new spell is successful

     
Misc
====

.. list-table:: 
   :widths: 35 65
   :header-rows: 1

   * - `Sound` name
     - Situation used 
   * - ``drink``
     - The player consumes a potion
   * - ``drown``
     - Looping while the player is underwater and out of breath
   * - ``health damage``
     - When a character or creature takes damage
   * - ``skillraise``
     - When a skill is raised
   * - ``swallow``
     - The player consumes an ingredient
   * - ``torch out``
     - When the currently equipped torch is extinguished


User-defined Sound Effects
**************************

Lights
======

Objects of `Light` type can be assigned a sound record that will be played repeatedly.


Activators
==========

`Activators` can play a sound effect through their assigned script. This method  
can be used to place unique, localized sound effects in the world.


Doors
=====

Objects of `Door` type can be assigned two sounds. One is played when the door
is used or opened. The other is played when the door is closed. Locked door
sound is hardcoded and listed in a prior table.


Magic effects
=============

Each `Magic Effect` has four slots where sounds are assigned.

* `Casting Sound` - when the spell with this magic effect is cast
* `Hit Sound` - when the spell hits a target
* `Area Sound` - when the spell hits an area target
* `Bolt Sound` - projectile from this magic effect


Regions
=======

A `Region` has a `Sounds` table where it can be assigned any number of sounds. 
These are played while the player is in this region and are given a chance how 
often they will be heard.


NPCs speaking
=============

NPCs can utter a `Sound` in specific in-game situations. These are assigned through `Topic Infos` and occur
depending on the `Topic` in use.


* Alarm
* Attack
* Flee
* Hello
* Hit
* Idle
* Intruder
* Thief


Creatures
=========

`Creatures` get sound effects through `Sound Generator` records. A `Sound Generator` is assigned
a creature it will affect and set a type.

* Land
* Left Foot
* Moan
* Right Foot
* Roar
* Scream
* Swim Left
* Swim Right

Each type relates to a specific in-game event or an event defined in the creature's animation textkey file.
