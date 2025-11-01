Game Settings
#############

.. omw-setting::
   :title: show owned
   :type: int
   :range: 0, 1, 2, 3
   :default: 0
   :location: :bdg-success:`Launcher > Settings > Interface`

   .. list-table::
      :header-rows: 1

      * - Mode
        - Meaning
      * - 0
        - No clues are provided which is the default Morrowind behaviour.
      * - 1
        - The background of the tool tip for the object is highlighted in the colour specified by the colour background owned setting in the GUI Settings Section.
          The crosshair is not visible if crosshair is false.
      * - 2
        - The crosshair is the colour of the colour crosshair owned setting in the GUI Settings section.
      * - 3
        - Both the tool tip background and the crosshair are coloured.

   Enable visual clues for items owned by NPCs when the crosshair is on the object.

.. omw-setting::
   :title: show projectile damage
   :type: boolean
   :range: true, false
   :default: false
   :location: :bdg-success:`Launcher > Settings > Interface`

   If this setting is true, the damage bonus of arrows and bolts will show on their tooltip.

.. omw-setting::
   :title: show melee info
   :type: boolean
   :range: true, false
   :default: false
   :location: :bdg-success:`Launcher > Settings > Interface`

   If this setting is true, the reach and speed of weapons will show on their tooltip.

.. omw-setting::
   :title: show enchant chance
   :type: boolean
   :range: true, false
   :default: false
   :location: :bdg-success:`Launcher > Settings > Interface`

   Whether or not the chance of success will be displayed in the enchanting menu.

.. omw-setting::
   :title: best attack
   :type: boolean
   :range: true, false
   :default: false
   :location: :bdg-info:`In Game > Options > Prefs`

   If this setting is true, the player character will always use the most powerful attack when striking with a weapon
   (chop, slash or thrust). If this setting is false,
   the type of attack is determined by the direction that the character is moving at the time the attack begins.

.. omw-setting::
   :title: can loot during death animation
   :type: boolean
   :range: true, false
   :default: true
   :location: :bdg-success:`Launcher > Settings > Gameplay`

   If this setting is true, the player is allowed to loot actors (e.g. summoned creatures) during death animation, 
   if they are not in combat. In this case we have to increment death counter and run disposed actor's script instantly.

   If this setting is false, player has to wait until end of death animation in all cases.
   Makes using of summoned creatures exploit (looting summoned Dremoras and Golden Saints for expensive weapons) a lot harder.
   Conflicts with mannequin mods, which use SkipAnim to prevent end of death animation.

.. omw-setting::
   :title: difficulty
   :type: int
   :range: -500 to 500
   :default: 0
   :location: :bdg-info:`In Game > Options > Prefs`

   This setting adjusts the difficulty of the game and is intended to be in the range -100 to 100 inclusive.
   Given the default game setting for fDifficultyMult of 5.0,
   a value of -100 results in the player taking 80% of the usual damage, doing 6 times the normal damage.
   A value of 100 results in the player taking 6 times as much damage, while inflicting only 80% of the usual damage.
   Values below -500 will result in the player receiving no damage,
   and values above 500 will result in the player inflicting no damage.

.. omw-setting::
   :title: actors processing range
   :type: int
   :range: 3584 to 7168
   :default: 7168
   :location: :bdg-info:`In Game > Options > Prefs`

   This setting specifies the actor state update distance from the player in game units.
   Actor state update includes AI, animations, and physics processing.
   Actors close to this distance softly fade in and out instead of appearing or disappearing abruptly.
   Keep in mind that actors running Travel AI packages are always active to avoid
   issues in mods with long-range AiTravel packages (for example, patrols, caravans and travellers).

.. omw-setting::
   :title: classic reflected absorb spells behavior
   :type: boolean
   :range: true, false
   :default: true
   :location: :bdg-success:`Launcher > Settings > Gameplay`

   If this setting is true, effects of Absorb spells which were reflected by the target are not mirrored,
   and the caster will absorb their own stat resulting in no effect on either the caster and the target.
   This makes the gameplay as a mage easier, but these spells become imbalanced.
   This is how Morrowind behaves.

.. omw-setting::
   :title: classic calm spells behavior
   :type: boolean
   :range: true, false
   :default: true
   :location: :bdg-success:`Launcher > Settings > Gameplay`

   If this setting is true, Calm spells will take their target out of combat every frame.
   This means that a Calm spell of any magnitude will always take actors out of combat for the entirety of its duration.
   This is how Morrowind behaves without the Morrowind Code Patch. If this setting is off,
   Calm spells will only take their target out of combat once. Allowing them to re-engage if the spell was not sufficiently strong.

.. omw-setting::
   :title: use magic item animations
   :type: boolean
   :range: true, false
   :default: false
   :location: :bdg-success:`Launcher > Settings > Visuals > Animations`

   If this setting is true, the engine will use casting animations for magic items, including scrolls.
   Otherwise, there will be no casting animations, just as in original engine

.. omw-setting::
   :title: show effect duration
   :type: boolean
   :range: true, false
   :default: false
   :location: :bdg-success:`Launcher > Settings > Interface`

   Show the remaining duration of magic effects and lights if this setting is true.
   The remaining duration is displayed in the tooltip by hovering over the magical effect.

.. omw-setting::
   :title: enchanted weapons are magical
   :type: boolean
   :range: true, false
   :default: true
   :location: :bdg-success:`Launcher > Settings > Gameplay`

   Make enchanted weapons without Magical flag bypass normal weapons resistance (and weakness) certain creatures have.
   This is how Morrowind behaves.

.. omw-setting::
   :title: prevent merchant equipping
   :type: boolean
   :range: true, false
   :default: false
   :location: :bdg-success:`Launcher > Settings > Gameplay`

   Prevent merchants from equipping items that are sold to them.

.. omw-setting::
   :title: followers attack on sight
   :type: boolean
   :range: true, false
   :default: false
   :location: :bdg-success:`Launcher > Settings > Gameplay`

   Make player followers and escorters start combat with enemies who have started combat with them or the player.
   Otherwise they wait for the enemies or the player to do an attack first.
   Please note this setting has not been extensively tested and could have side effects with certain quests.

.. omw-setting::
   :title: shield sheathing
   :type: boolean
   :range: true, false
   :default: false
   :location: :bdg-success:`Launcher > Settings > Visuals > Animations`

   If this setting is true, OpenMW will utilize shield sheathing-compatible assets to display holstered shields.

   To make use of this, you need to have an xbase_anim_sh.nif file with weapon bones that will be injected into the skeleton.
   Also you can use additional _sh meshes for more precise shield placement.
   Warning: this feature may conflict with mods that use pseudo-shields to emulate item in actor's hand (e.g. books, baskets, pick axes).
   To avoid conflicts, you can use _sh mesh without "Bip01 Sheath" node for such "shields" meshes, or declare its bodypart as Clothing type, not as Armor.
   Also you can use an _sh node with empty "Bip01 Sheath" node.
   In this case the engine will use basic shield model, but will use transformations from the "Bip01 Sheath" node.

.. omw-setting::
   :title: weapon sheathing
   :type: boolean
   :range: true, false
   :default: false
   :location: :bdg-success:`Launcher > Settings > Visuals > Animations`

   If this setting is true, OpenMW will utilize weapon sheathing-compatible assets to display holstered weapons.

   To make use of this, you need to have an xbase_anim_sh.nif file with weapon bones that will be injected into the skeleton.
   Additional _sh suffix models are not essential for weapon sheathing to work but will act as quivers or scabbards for the weapons they correspond to.

.. omw-setting::
   :title: use additional anim sources
   :type: boolean
   :range: true, false
   :default: false
   :location: :bdg-success:`Launcher > Settings > Visuals > Animations`

   Allow the engine to load additional animation sources when enabled.
   For example, if the main animation mesh has name Meshes/x.nif, 
   the engine will load all KF-files from Animations/x folder and its child folders.
   This can be useful if you want to use several animation replacers without merging them.
   Attention: animations from AnimKit have their own format and are not supposed to be directly loaded in-game!

.. omw-setting::
   :title: barter disposition change is permanent
   :type: boolean
   :range: true, false
   :default: false
   :location: :bdg-success:`Launcher > Settings > Gameplay`

   If this setting is true, 
   disposition change of merchants caused by trading will be permanent and won't be discarded upon exiting dialogue with them.
   This imitates the option that Morrowind Code Patch offers.

.. omw-setting::
   :title: only appropriate ammunition bypasses resistance
   :type: boolean
   :range: true, false
   :default: false
   :location: :bdg-success:`Launcher > Settings > Gameplay`

   If this setting is true, you will have to use the appropriate ammunition to bypass normal weapon resistance (or weakness).
   An enchanted bow with chitin arrows will no longer be enough for the purpose, while a steel longbow with glass arrows will still work.
   This was previously the default engine behavior that diverged from Morrowind design.

.. omw-setting::
   :title: strength influences hand to hand
   :type: int
   :range: 0, 1, 2
   :default: 0
   :location: :bdg-success:`Launcher > Settings > Gameplay`

   This setting controls the behavior of factoring of Strength attribute into hand-to-hand damage, which is using the formula
   Morrowind Code Patch uses for its equivalent feature: damage is multiplied by its value divided by 40.

   .. list-table:: 
      :header-rows: 1

      * - Mode
        - Meaning
      * - 0
        - Strength attribute is ignored
      * - 1
        - Strength attribute is factored in damage from any actor
      * - 2
        - Strength attribute is factored in damage from any actor except werewolves

.. omw-setting::
   :title: normalise race speed
   :type: boolean
   :range: true, false
   :default: false
   :location: :bdg-success:`Launcher > Settings > Gameplay`

   By default race weight is factored into horizontal movement and magic projectile speed like in Morrowind.
   For example, an NPC which has 1.2 race weight is faster than an NPC with the exact same stats and weight 1.0 by a factor of 120%.
   If this setting is true, race weight is ignored in the calculations which allows for a movement behavior
   equivalent to the one introduced by the equivalent Morrowind Code Patch feature.
   This makes the movement speed behavior more fair between different races.

.. omw-setting::
   :title: projectiles enchant multiplier
   :type: float32
   :range: [0, 1]
   :default: 0.0
   

   The value of this setting determines how many projectiles (thrown weapons, arrows and bolts) you can enchant at once according to the following formula:

   .. math::

      \text{count} = \frac{\text{soul gem charge} \cdot \text{projectiles enchant multiplier}}{\text{enchantment strength}}

   A value of 0 means that you can only enchant one projectile.
   If you want to have Morrowind Code Patch-like count of projectiles being enchanted at once, set this value to 0.25 (i.e. 25% of the charge).

.. omw-setting::
   :title: uncapped damage fatigue
   :type: boolean
   :range: true, false
   :default: false
   :location: :bdg-success:`Launcher > Settings > Gameplay`

   There are four ways to decrease an actor's Fatigue stat in Morrowind gameplay mechanics:
   Drain, Absorb, Damage Fatigue magic effects and hand-to-hand combat.
   However, in Morrowind you can't knock down an actor with a Damage Fatigue spell or an Absorb Fatigue spell.
   Morrowind Code Patch adds an option to make it possible for Damage Fatigue spells. This is the equivalent of that option.

   Setting the value of this setting to true will remove the 0 lower cap from the value,
   allowing Damage Fatigue to reduce Fatigue to a value below zero.

.. omw-setting::
   :title: turn to movement direction
   :type: boolean
   :range: true, false
   :default: false
   :location: :bdg-success:`Launcher > Settings > Visuals > Animations`

   Affects side and diagonal movement. Enabling this setting makes movement more realistic.

   If disabled then the whole character's body is pointed to the direction of view. Diagonal movement has no special animation and causes sliding.

   If enabled then the character turns lower body to the direction of movement. Upper body is turned partially. Head is always pointed to the direction of view. In combat mode it works only for diagonal movement. In non-combat mode it changes straight right and straight left movement as well. Also turns the whole body up or down when swimming according to the movement direction.

.. omw-setting::
   :title: smooth movement
   :type: boolean
   :range: true, false
   :default: false
   :location: :bdg-success:`Launcher > Settings > Visuals > Animations`

   Makes NPCs and player movement more smooth.

   Recommended to use with "turn to movement direction" enabled.

.. omw-setting::
   :title: smooth movement player turning delay
   :type: float32
   :range: ≥ 0.01
   :default: 0.333

   Max delay of turning (in seconds) if player drastically changes direction on the run. Makes sense only if "smooth movement" is enabled.

.. omw-setting::
   :title: NPCs avoid collisions
   :type: boolean
   :range: true, false
   :default: false
   :location: :bdg-success:`Launcher > Settings > Gameplay`

   If enabled NPCs apply evasion maneuver to avoid collisions with others.

.. omw-setting::
   :title: NPCs give way
   :type: boolean
   :range: true, false
   :default: true

   Standing NPCs give way to moving ones. Works only if 'NPCs avoid collisions' is enabled.

.. omw-setting::
   :title: swim upward correction
   :type: boolean
   :range: true, false
   :default: false
   :location: :bdg-success:`Launcher > Settings > Gameplay`

   Makes player swim a bit upward from the line of sight. Applies only in third person mode. Intended to make simpler swimming without diving.

.. omw-setting::
   :title: swim upward coef
   :type: float32
   :range: -1.0 to 1.0
   :default: 0.2

   Regulates strength of the "swim upward correction" effect (if enabled).
   Makes player swim a bit upward (or downward in case of negative value) from the line of sight. Recommended range of values is from 0.0 to 0.25.

.. omw-setting::
   :title: trainers training skills based on base skill
   :type: boolean
   :range: true, false
   :default: false
   :location: :bdg-success:`Launcher > Settings > Gameplay`

   The trainers in Morrowind choose their proposed training skills based on their 3 best attributes.

   If disabled then the 3 best skills of trainers and the training limits take into account fortified/drained trainer skill.

   If enabled then the 3 best skills of trainers and the training limits are based on the trainer base skills.

.. omw-setting::
   :title: use modified skill levels for training
   :type: boolean
   :range: true, false
   :default: false
   :location: :bdg-success:`Launcher > Settings > Gameplay`

   When enabled, a trainer will base prices on modified player skill levels, instead of base skill levels.

   This allows the player to adjust training prices and availability using Drain/Fortify skill magic.

   This re-implements the Training Exploit from vanilla Morrowind.

.. omw-setting::
   :title: always allow stealing from knocked out actors
   :type: boolean
   :range: true, false
   :default: false
   :location: :bdg-success:`Launcher > Settings > Gameplay`

   By Bethesda's design, in the latest released version of Morrowind pickpocketing is impossible during combat,
   even if the fighting NPC is knocked out.

   This setting allows the player to steal items from fighting NPCs that were knocked out if enabled.

.. omw-setting::
   :title: graphic herbalism
   :type: boolean
   :range: true, false
   :default: true
   :location: :bdg-success:`Launcher > Settings > Gameplay`

   Some mods add harvestable container models. When this setting is enabled, activating a container using a harvestable model will visually harvest from it instead of opening the menu.

   When this setting is turned off or when activating a regular container, the menu will open as usual.

.. omw-setting::
   :title: allow actors to follow over water surface
   :type: boolean
   :range: true, false
   :default: true
   :location: :bdg-success:`Launcher > Settings > Gameplay`

   If enabled actors will always find path over the water surface when following other actors. This makes OpenMW behaviour closer to the vanilla engine.

   If disabled actors without the ability to swim will not follow other actors to the water.

   .. note::
       Has effect only when Navigator is enabled.

.. omw-setting::
   :title: default actor pathfind half extents
   :type: float32|float32|float32
   :range: > 0
   :default: 29.27999496459961 28.479997634887695 66.5

   Actor half extents used for exterior cells to generate navmesh.
   Changing the value will invalidate navmesh disk cache.

.. omw-setting::
   :title: day night switches
   :type: boolean
   :range: true, false
   :default: true
   :location: :bdg-success:`Launcher > Settings > Gameplay`

   Some mods add models which change visuals based on time of day. When this setting is enabled, supporting models will automatically make use of Day/night state.

.. omw-setting::
   :title: actor collision shape type
   :type: int
   :range: 0, 1, 2
   :default: 0 (Axis-aligned bounding box)
   :location: :bdg-success:`Launcher > Settings > Gameplay`

   Collision is used for both physics simulation and navigation mesh generation for pathfinding.
   Cylinder gives the best consistency bewtween available navigation paths and ability to move by them.
   Changing this value affects navigation mesh generation therefore navigation mesh disk cache generated for one value
   will not be useful with another.

   .. list-table::
      :header-rows: 1

      * - Mode
        - Meaning
      * - 0
        - Axis-aligned bounding box
      * - 1
        - Rotating box
      * - 2
        - Cylinder

.. omw-setting::
   :title: player movement ignores animation
   :type: boolean
   :range: true, false
   :default: false
   :location: :bdg-success:`Launcher > Settings > Visuals > Animations`

   In third person, the camera will sway along with the movement animations of the player. 
   Enabling this option disables this swaying by having the player character move independently of its animation.

.. omw-setting::
   :title: smooth animation transitions
   :type: boolean
   :range: true, false
   :default: false
   :location: :bdg-success:`Launcher > Settings > Visuals > Animations`

   Enabling this option uses smooth transitions between animations making them a lot less jarring. Also allows to load modded animation blending.

.. omw-setting::
   :title: rebalance soul gem values
   :type: boolean
   :range: true, false
   :default: false
   :location: :bdg-success:`Launcher > Settings > Gameplay`

   Enabling this option drastically reduces the value of filled soul gems.
   The value will depend on soul magnitude but not the size of the used
   soul gem.

   The new value formula is based on the Morrowind Code Patch project:

   .. math::

   	\text{new value} = 0.0001 \cdot (\text{soul magnitude})^3 + 2 \cdot (\text{soul magnitude})
