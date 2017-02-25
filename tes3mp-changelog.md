0.5.1
-----

* Fixes to server crashes caused by incorrect cell comparison added by bandwidth optimization changes
* Fixes to server browser freezes caused by connection failures and incomplete data received from master server

0.5.0
-----

* Server browser
* Synchronization of containers
* Reworked world packets allowing for the saving and loading of world state, including container state
* Bandwidth optimization by forwarding the most frequent player packets only to other players in the same loaded cells

0.4.1
-----

* Packet for saving and loading spellbooks

0.4.0
-----

* Synchronization of spells
* Packet for saving and loading inventories
* Being in a menu no longer prevents you from sending packets about your client
* Fixes to freezes and problems caused by players moving around in cells from expansions/plugins that are not loaded by others

0.3.0
-----

* Synchronization of world object removal, placement, scaling, locking and unlocking
* Synchronization of local, global and member script variables for specific scripts
* Synchronization for the setdelete, placeat, setscale, lock and unlock console commands
* Player markers on minimap
* Death reasons in chat
* Fixes to client freezes related to players logging in at the same time
* Fixes to server crashes related to sending information about invalid players

0.2.0
-----

* Packets for saving and loading classes, birthsigns, dynamic stats, levels, level progress, attribute bonuses from skill increases and progress towards skill increases
* Version checking to prevent mismatches between clients and servers

0.0.1b
------

* Synchronization of attributes and skills
* Fixes to memory leaks related to player initialization on Windows servers
* Fixes to various graphical glitches
* Main menu buttons for starting, saving and loading games are now disabled

0.0.1a
------

* Combat animation
* Synchronization of melee and ranged (bow, crossbow, throwable weapons) combat
* Synchronization of health, magicka, fatigue and death
* Synchronization of cell changes
* Server-side Lua scripting
* Chat

0.0.1
-----

* Initial networking and packet architecture
* Synchronization of player character generation
* Synchronization of player position
* Synchronization of player attack states (unarmed, armed with a weapon, using a spell)
* Synchronization of movement and jump animations
