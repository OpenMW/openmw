Extended modding feature overview
#################################

OpenMW supports some extended modding features out of the box.
These features can conflict with mods, and such situations should be handled as any other mod conflict by patching the mods themselves.
The engine itself does not provide any kind of blacklisting of incompatible mods.


Native herbalism support
------------------------

In OpenMW it is possible to add one or more NiSwitchNodes with a ``HerbalismSwitch`` name.
Every switch node should have at least two child nodes (the first one represents the unharvested container, the second one - the harvested container).
If an organic container's mesh has such nodes, it is considered to be a plant. During activation, a window with the plant's content is not shown,
OpenMW transfers the contents directly into the player's inventory, triggers a theft crime event if the plant is owned and toggles the harvested model state.

It is also possible to use scripts with ``OnActivate`` command for such containers. For example, when player needs a tool to harvest a plant (e.g. a pickaxe for ore).

Keep in mind that the collision detection system ignores switch nodes, so add a ``RootCollisionNode`` or ``NCO`` NiStringExtraData to harvestable meshes.

Advantages of described approach over old herbalism mods:

1. There is no need to spawn separate "harvested" objects

2. There is no need to attach a script to every container

3. It supports an ownership check (the original engine without MWSE does not)

4. It does not alter original respawn mechanics

An example of mod which uses this feature is `Graphic Herbalism`_.

Animated containers support
---------------------------

It is possible to attach opening/closing animations for containers. To do this, you need to create a KF-file for the container with the following groups:

1. ``ContainerOpen`` (with ``Start``, ``Loot`` and ``Stop`` keys)

2. ``ContainerClose`` (with ``Start`` and ``Stop`` keys)

The ``Loot`` key for ``ContainerOpen`` allows to play a part of opening animation in the background.

For example, with the following setup, the opening animation has 1.0 sec duration and shows the container window in the middle of opening animation:

::

    0.0: ContainerOpen: start
    0.5: ContainerOpen: loot
    1.0: ContainerOpen: stop

It is also possible to attach opening/closing sounds to container's animations:

::

    1.0: ContainerClose: start
    1.01: Sound: AC_dw_drawer_close
    2.0: ContainerClose: stop

The text key approach is the same as the one used for sound playback in animations in general
Note that the sound starting time value is slightly higher than the closing animation start, otherwise sound will be played at the end of opening animation in this example.

It is important to assign a RootCollisionNode to the container mesh -- the collision shape will depend on the animation state otherwise, and this can have a performance impact.

Advantages of described approach over old animated containers mods:

1. There is no need to attach a script to every container

2. Part of the opening animation can be played in the background, so we do not waste the player's time

An example of a mod which uses this feature is `OpenMW Containers Animated`_.


Day/night state support
-----------------------

It is possible to add one or more NiSwitchNodes named ``NightDaySwitch``.
Every such switch should have at least two child nodes
(the first node represents the normal node state, the second one represents the node state during night,
the optional third node represents the node state during daytime in interior cells).

The behavior of such a model:

1. During the day in exteriors, it is in the "normal" mode (child 0).

2. During the night in exteriors, it is in the "night" mode (child 1).

3. During the day in interiors, it is in the "normal" mode or "interior day" mode (child 2) depending on weather.

4. During the night in interiors, it is in the "normal" mode.

The actual state toggling time depends on the sunrise/sunset time settings in `openmw.cfg`:

::

    fallback=Weather_Sunrise_Time,6
    fallback=Weather_Sunset_Time,18
    fallback=Weather_Sunrise_Duration,2
    fallback=Weather_Sunset_Duration,2

These settings lead to the "night" starting at 20:00 and ending at 6:00.

The engine checks if the weather is bright enough to support the "interior day" mode using the Glare_View setting. If it is >= 0.5, the engine considers the weather bright.

::

    fallback=Weather_Clear_Glare_View,1
    fallback=Weather_Foggy_Glare_View,0.25

With these settings, the "interior day" mode would be used for Clear weather, but would not be used for Foggy weather.

Keep in mind that the engine will not update the weather type after a teleportation to a different region if the player did not move to an exterior cell in the new region yet.

This feature can be used to implement street illumination, glowing windows, etc.

Advantages of the described approach over old mods with glowing windows:

1. There is no need to spawn additional objects for day and night mode

2. There is no need to attach a script to every switchable object

An example of a mod which uses this feature is `Glow in the Dahrk`_.


Per-group animation files support
---------------------------------

In the original engine it is possible to add a custom animation file to NPC to override some animations (usually idle ones).
In OpenMW it is possible to override animations via the same file for all actors which use a given basic animation file.

If you want to override animations for all biped actors (which use the xbase_anim.nif skeleton), you can put your animations in the
``Animations/xbase_anim`` folder in your ``Data Files``. You can also have them in a data folder with a higher priority.
In this case any biped actor without a custom animation will use your animations, but – if he has additional animations – they have a higher priority.

For example, all biped actors in Morrowind normally use the same spellcasting animations, so overriding xbase_anim spellcasting animations is sufficient.
If you want to override walking animations, you should override ``xbase_anim_female`` and ``xbase_anim_kna`` animations -- these are used for women and beast races, and
– because they have their own walking animations – they override ones which come from ``xbase_anim`` and its loose overrides.

To enable this feature, you should have this line in your settings.cfg:

::

    [Game]
    use additional anim sources = true

An example of a mod which uses this feature is `Almalexia's Cast for Beasts`_.


Weapon sheathing support
------------------------

In OpenMW it is possible to display equipped, but not currently wielded weapons on the actor's model, including quivers and scabbards.

This feature conflicts with old mods which use scripted scabbards, arrows with particles or decorative quivers (attached to the left pauldron, for example).

1. Basics

The minimum you need is the ``xbase_anim_sh.nif`` file from the `Weapon Sheathing`_ mod and this line in your settings.cfg:

::

    [Game]
    weapon sheathing = true

The ``xbase_anim_sh.nif`` contains default placement points for different weapon types.
That way you'll get Gothic-style weapon sheathing for all biped actors (without quivers and scabbards).

2. Scabbards

For a scabbard to be displayed, you need a mesh with an ``_sh`` suffix. For example, if the weapon has a model named foo.nif, the scabbard model must be named foo_sh.nif.

There should be an least two nodes in the sheath file:

``Bip01 Weapon`` - represents the weapon itself (may be just a grip for sword, for example). It is not shown when the weapon is drawn.

``Bip01 Sheath`` - represents scabbards, quivers, etc. It is shown always when the weapon is equipped.

You can move or rotate nodes if the default placement from the ``xbase_anim_sh.nif`` does not look good for your weapon.

If you want to exempt a specific weapon from using this feature, you can create a stub sheath mesh with just one root node.

If you want to use the common weapon mesh, but with custom placement, you can create a sheath mesh with an empty ``Bip01 Weapon`` node and move it as you want.

3. Quivers

To show the quiver for a ranged weapon, you need these nodes in the sheath file:

``Bip01 Sheath`` node, as for scabbards

``Bip01 Ammo`` node to show ammunition in the quiver

``Bip01 Weapon`` to show the weapon itself (not needed for throwing weapons)

The ``Bip01 Ammo`` should have some empty child nodes, to which the engine will attach ammunition nodes.

The appearance and count of shown ammunition depends on type and count of equipped ammunition. If the ammunition has a wrong type (e.g. bolts for bow), it won't be shown.

It is important to make sure the names of empty nodes start with ``"Bip01 "``, or the engine will optimize them out.

An example of a mod which uses this feature is `Weapon Sheathing`_.


Skeleton extensions
-------------------

It is possible to inject custom bones into actor skeletons:

::

    [Game]
    use additional anim sources = true

If this setting is enabled, OpenMW will seek for modified skeletons in the ``Animations/[skeleton name]`` folder in your ``Data Files``.
For example, the biped creature skeleton folder is ``Animations/xbase_anim``, the female NPCs skeleton folder is ``Animations/xbase_anim_female``,
the beast race skeleton folder is ``Animations/xbase_anim_kna``.
Note that these are the third person view skeletons, and the first person view skeleton will have a different name.

OpenMW scans every NIF file in such a folder for nodes which have "BONE" NiStringExtraData.
It is recommended to give such nodes names that start with "Bip01 " so that the mesh optimizer doesn't try to optimize them out.
Then OpenMW copies all found nodes to related skeleton. To determine the bone to which the new node should be attached,
OpenMW checks the name of the parent node of the new node in the original NIF file.
For example, to attach a custom weapon bone, you'll need to follow this NIF record hierarchy:

::

NiNode "root"
    NiNode "Bip01 L Hand"
        NiNode "Weapon Bone Left"
            NiStringExtraData "BONE"

OpenMW will detect ``Weapon Bone Left`` node and attach it to ``Bip01 L Hand`` bone of the target skeleton.

An example of a mod which uses this feature is `Weapon Sheathing`_.


Extended weapon animations
--------------------------

It is possible to use unique animation groups for different weapon types.
They are not mandatory, and the currently hardcoded weapon types will fall back to existing generic animations.
Every weapon type has an attack animation group and a suffix for the movement animation groups.
For example, long blades use ``weapononehand`` attack animation group, ``idle1h`` idle animation group, ``jump1h`` jumping animation group, etc.
This is the full table of supported animation groups:

+---------------+-------------------+------------------+----------------------+-----------------------+
|  Weapon type  |  Animation group  |  Movement suffix |   Attack (fallback)  |   Suffix (fallback)   |
+===============+===================+==================+======================+=======================+
|  Short blade  | shortbladeonehand |        1s        |    weapononehand     |          1h           |
+---------------+-------------------+------------------+----------------------+-----------------------+
| Long blade 1H |   weapononehand   |        1h        |                      |                       |
+---------------+-------------------+------------------+----------------------+-----------------------+
| Long blade 2H |   weapontwohand   |        2c        |                      |                       |
+---------------+-------------------+------------------+----------------------+-----------------------+
|   Blunt 1H    |   bluntonehand    |        1b        |    weapononehand     |          1h           |
+---------------+-------------------+------------------+----------------------+-----------------------+
|   Blunt 2H    |   blunttwohand    |        2b        |    weapontwohand     |          2c           |
+---------------+-------------------+------------------+----------------------+-----------------------+
|    Axe 1H     |   bluntonehand    |        1b        |    weapononehand     |          1h           |
+---------------+-------------------+------------------+----------------------+-----------------------+
|    Axe 2H     |   blunttwohand    |        2b        |    weapontwohand     |          2c           |
+---------------+-------------------+------------------+----------------------+-----------------------+
| Blunt 2H wide |   weapontwowide   |        2w        |    weapontwohand     |          2c           |
+---------------+-------------------+------------------+----------------------+-----------------------+
|     Spear     |   weapontwowide   |        2w        |    weapontwohand     |          2c           |
+---------------+-------------------+------------------+----------------------+-----------------------+
|      Bow      |    bowandarrow    |        bow       |                      |          1h           |
+---------------+-------------------+------------------+----------------------+-----------------------+
|    Crossbow   |     crossbow      |     crossbow     |                      |          1h           |
+---------------+-------------------+------------------+----------------------+-----------------------+
|     Thrown    |    throwweapon    |        1t        |                      |          1h           |
+---------------+-------------------+------------------+----------------------+-----------------------+

.. _`Graphic Herbalism`: https://www.nexusmods.com/morrowind/mods/46599
.. _`OpenMW Containers Animated`: https://www.nexusmods.com/morrowind/mods/46232
.. _`Glow in the Dahrk`: https://www.nexusmods.com/morrowind/mods/45886
.. _`Almalexia's Cast for Beasts`: https://www.nexusmods.com/morrowind/mods/45853
.. _`Weapon sheathing`: https://www.nexusmods.com/morrowind/mods/46069
