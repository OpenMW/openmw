0.47.0
------

    Bug #1952: Incorrect particle lighting
    Bug #3676: NiParticleColorModifier isn't applied properly
    Bug #5358: ForceGreeting always resets the dialogue window completely
    Bug #5363: Enchantment autocalc not always 0/1
    Bug #5364: Script fails/stops if trying to startscript an unknown script
    Bug #5367: Selecting a spell on an enchanted item per hotkey always plays the equip sound
    Bug #5369: Spawnpoint in the Grazelands doesn't produce oversized creatures
    Bug #5370: Opening an unlocked but trapped door uses the key
    Feature #5362: Show the soul gems' trapped soul in count dialog

0.46.0
------

    Bug #1515: Opening console masks dialogue, inventory menu
    Bug #1933: Actors can have few stocks of the same item
    Bug #2395: Duplicated plugins in the launcher when multiple data directories provide the same plugin
    Bug #2679: Unable to map mouse wheel under control settings
    Bug #2969: Scripted items can stack
    Bug #2976: Data lines in global openmw.cfg take priority over user openmw.cfg
    Bug #2987: Editor: some chance and AI data fields can overflow
    Bug #3006: 'else if' operator breaks script compilation
    Bug #3109: SetPos/Position handles actors differently
    Bug #3282: Unintended behaviour when assigning F3 and Windows keys
    Bug #3550: Companion from mod attacks the air after combat has ended
    Bug #3609: Items from evidence chest are not considered to be stolen if player is allowed to use the chest
    Bug #3623: Display scaling breaks mouse recognition
    Bug #3725: Using script function in a non-conditional expression breaks script compilation
    Bug #3733: Normal maps are inverted on mirrored UVs
    Bug #3765: DisableTeleporting makes Mark/Recall/Intervention effects undetectable
    Bug #3778: [Mod] Improved Thrown Weapon Projectiles - weapons have wrong transformation during throw animation
    Bug #3812: Wrong multiline tooltips width when word-wrapping is enabled
    Bug #3894: Hostile spell effects not detected/present on first frame of OnPCHitMe
    Bug #3977: Non-ASCII characters in object ID's are not supported
    Bug #4009: Launcher does not show data files on the first run after installing
    Bug #4077: Enchanted items are not recharged if they are not in the player's inventory
    Bug #4141: PCSkipEquip isn't set to 1 when reading books/scrolls
    Bug #4202: Open .omwaddon files without needing toopen openmw-cs first
    Bug #4240: Ash storm origin coordinates and hand shielding animation behavior are incorrect
    Bug #4262: Rain settings are hardcoded
    Bug #4270: Closing doors while they are obstructed desyncs closing sfx
    Bug #4276: Resizing character window differs from vanilla
    Bug #4284: ForceSneak behaviour is inconsistent if the target has AiWander package
    Bug #4329: Removed birthsign abilities are restored after reloading the save
    Bug #4341: Error message about missing GDB is too vague
    Bug #4383: Bow model obscures crosshair when arrow is drawn
    Bug #4384: Resist Normal Weapons only checks ammunition for ranged weapons
    Bug #4411: Reloading a saved game while falling prevents damage in some cases
    Bug #4449: Value returned by GetWindSpeed is incorrect
    Bug #4456: AiActivate should not be cancelled after target activation
    Bug #4493: If the setup doesn't find what it is expecting, it fails silently and displays the requester again instead of letting the user know what wasn't found.
    Bug #4523: "player->ModCurrentFatigue -0.001" in global script does not cause the running player to fall
    Bug #4540: Rain delay when exiting water
    Bug #4594: Actors without AI packages don't use Hello dialogue
    Bug #4598: Script parser does not support non-ASCII characters
    Bug #4600: Crash when no sound output is available or --no-sound is used.
    Bug #4601: Filtering referenceables by gender is broken
    Bug #4639: Black screen after completing first mages guild mission + training
    Bug #4650: Focus is lost after pressing ESC in confirmation dialog inside savegame dialog
    Bug #4680: Heap corruption on faulty esp
    Bug #4701: PrisonMarker record is not hardcoded like other markers
    Bug #4703: Editor: it's possible to preview levelled list records
    Bug #4705: Editor: unable to open exterior cell views from Instances table
    Bug #4714: Crash upon game load in the repair menu while the "Your repair failed!" message is active
    Bug #4715: "Cannot get class of an empty object" exception after pressing ESC in the dialogue mode
    Bug #4720: Inventory avatar has shield with two-handed weapon during [un]equipping animation
    Bug #4723: ResetActors command works incorrectly
    Bug #4736: LandTexture records overrides do not work
    Bug #4745: Editor: Interior cell lighting field values are not displayed as colors
    Bug #4746: Non-solid player can't run or sneak
    Bug #4747: Bones are not read from X.NIF file for NPC animation
    Bug #4748: Editor: Cloned, moved, added instances re-use RefNum indices
    Bug #4750: Sneaking doesn't work in first person view if the player is in attack ready state
    Bug #4756: Animation issues with VAOs
    Bug #4757: Content selector: files can be cleared when there aren't any files to clear
    Bug #4768: Fallback numerical value recovery chokes on invalid arguments
    Bug #4775: Slowfall effect resets player jumping flag
    Bug #4778: Interiors of Illusion puzzle in Sotha Sil Expanded mod is broken
    Bug #4783: Blizzard behavior is incorrect
    Bug #4787: Sneaking makes 1st person walking/bobbing animation super-slow
    Bug #4797: Player sneaking and running stances are not accounted for when in air
    Bug #4800: Standing collisions are not updated immediately when an object is teleported without a cell change
    Bug #4802: You can rest before taking falling damage from landing from a jump
    Bug #4803: Stray special characters before begin statement break script compilation
    Bug #4804: Particle system with the "Has Sizes = false" causes an exception
    Bug #4805: NPC movement speed calculations do not take race Weight into account
    Bug #4810: Raki creature broken in OpenMW
    Bug #4813: Creatures with known file but no "Sound Gen Creature" assigned use default sounds
    Bug #4815: "Finished" journal entry with lower index doesn't close journal, SetJournalIndex closes journal
    Bug #4820: Spell absorption is broken
    Bug #4823: Jail progress bar works incorrectly
    Bug #4826: Uninitialized memory in unit test
    Bug #4827: NiUVController is handled incorrectly
    Bug #4828: Potion looping effects VFX are not shown for NPCs
    Bug #4837: CTD when a mesh with NiLODNode root node with particles is loaded
    Bug #4841: Russian localization ignores implicit keywords
    Bug #4844: Data race in savegame loading / GlobalMap render
    Bug #4847: Idle animation reset oddities
    Bug #4851: No shadows since switch to OSG
    Bug #4860: Actors outside of processing range visible for one frame after spawning
    Bug #4867: Arbitrary text after local variable declarations breaks script compilation
    Bug #4876: AI ratings handling inconsistencies
    Bug #4877: Startup script executes only on a new game start
    Bug #4879: SayDone returns 0 on the frame Say is called
    Bug #4888: Global variable stray explicit reference calls break script compilation
    Bug #4896: Title screen music doesn't loop
    Bug #4902: Using scrollbars in settings causes resolution to change
    Bug #4904: Editor: Texture painting with duplicate of a base-version texture
    Bug #4911: Editor: QOpenGLContext::swapBuffers() warning with Qt5
    Bug #4916: Specular power (shininess) material parameter is ignored when shaders are used.
    Bug #4918: Abilities don't play looping VFX when they're initially applied
    Bug #4920: Combat AI uses incorrect invisibility check
    Bug #4922: Werewolves can not attack if the transformation happens during attack
    Bug #4927: Spell effect having both a skill and an attribute assigned is a fatal error
    Bug #4932: Invalid records matching when loading save with edited plugin
    Bug #4933: Field of View not equal with Morrowind
    Bug #4938: Strings from subrecords with actually empty headers can't be empty
    Bug #4942: Hand-to-Hand attack type is chosen randomly when "always use best attack" is turned off
    Bug #4945: Poor random magic magnitude distribution
    Bug #4947: Player character doesn't use lip animation
    Bug #4948: Footstep sounds while levitating on ground level
    Bug #4952: Torches held by NPCs flicker too quickly
    Bug #4961: Flying creature combat engagement takes z-axis into account
    Bug #4963: Enchant skill progress is incorrect
    Bug #4964: Multiple effect spell projectile sounds play louder than vanilla
    Bug #4965: Global light attenuation settings setup is lacking
    Bug #4969: "Miss" sound plays for any actor
    Bug #4971: OpenMW-CS: Make rotations display as degrees instead of radians
    Bug #4972: Player is able to use quickkeys while disableplayerfighting is active
    Bug #4979: AiTravel maximum range depends on "actors processing range" setting
    Bug #4980: Drowning mechanics is applied for actors indifferently from distance to player
    Bug #4984: "Friendly hits" feature should be used only for player's followers
    Bug #4989: Object dimension-dependent VFX scaling behavior is inconsistent
    Bug #4990: Dead bodies prevent you from hitting
    Bug #4991: Jumping occasionally takes too much fatigue
    Bug #4999: Drop instruction behaves differently from vanilla
    Bug #5001: Possible data race in the Animation::setAlpha()
    Bug #5004: Werewolves shield their eyes during storm
    Bug #5012: "Take all" on owned container generates a messagebox per item
    Bug #5018: Spell tooltips don't support purely negative magnitudes
    Bug #5025: Data race in the ICO::setMaximumNumOfObjectsToCompilePerFrame()
    Bug #5028: Offered price caps are not trading-specific
    Bug #5038: Enchanting success chance calculations are blatantly wrong
    Bug #5047: # in cell names sets color
    Bug #5050: Invalid spell effects are not handled gracefully
    Bug #5055: Mark, Recall, Intervention magic effect abilities have no effect when added and removed in the same frame
    Bug #5056: Calling Cast function on player doesn't equip the spell but casts it
    Bug #5059: Modded animation with combined attack keys always does max damage and can double damage
    Bug #5060: Magic effect visuals stop when death animation begins instead of when it ends
    Bug #5063: Shape named "Tri Shadow" in creature mesh is visible if it isn't hidden
    Bug #5067: Ranged attacks on unaware opponents ("critical hits") differ from the vanilla engine
    Bug #5069: Blocking creatures' attacks doesn't degrade shields
    Bug #5073: NPCs open doors in front of them even if they don't have to
    Bug #5074: Paralyzed actors greet the player
    Bug #5075: Enchanting cast style can be changed if there's no object
    Bug #5078: DisablePlayerLooking is broken
    Bug #5081: OpenMW-CS: Apparatus type "Alembic" is erroneously named "Albemic"
    Bug #5082: Scrolling with controller in GUI mode is broken
    Bug #5087: Some valid script names can't be used as string arguments
    Bug #5089: Swimming/Underwater creatures only swim around ground level
    Bug #5092: NPCs with enchanted weapons play sound when out of charges
    Bug #5093: Hand to hand sound plays on knocked out enemies
    Bug #5097: String arguments can't be parsed as number literals in scripts
    Bug #5099: Non-swimming enemies will enter water if player is water walking
    Bug #5103: Sneaking state behavior is still inconsistent
    Bug #5104: Black Dart's enchantment doesn't trigger at low Enchant levels
    Bug #5106: Still can jump even when encumbered
    Bug #5110: ModRegion with a redundant numerical argument breaks script execution
    Bug #5112: Insufficient magicka for current spell not reflected on HUD icon
    Bug #5113: Unknown alchemy question mark not centered
    Bug #5123: Script won't run on respawn
    Bug #5124: Arrow remains attached to actor if pulling animation was cancelled
    Bug #5126: Swimming creatures without RunForward animations are motionless during combat
    Bug #5134: Doors rotation by "Lock" console command is inconsistent
    Bug #5136: LegionUniform script: can not access local variables
    Bug #5137: Textures with Clamp Mode set to Clamp instead of Wrap are too dark outside the boundaries
    Bug #5138: Actors stuck in half closed door
    Bug #5149: Failing lock pick attempts isn't always a crime
    Bug #5155: Lock/unlock behavior differs from vanilla
    Bug #5158: Objects without a name don't fallback to their ID
    Bug #5159: NiMaterialColorController can only control the diffuse color
    Bug #5161: Creature companions can't be activated when they are knocked down
    Bug #5164: Faction owned items handling is incorrect
    Bug #5163: UserData is not copied during node cloning
    Bug #5166: Scripts still should be executed after player's death
    Bug #5167: Player can select and cast spells before magic menu is enabled
    Bug #5168: Force1stPerson and Force3rdPerson commands are not really force view change
    Bug #5169: Nested levelled items/creatures have significantly higher chance not to spawn
    Bug #5175: Random script function returns an integer value
    Bug #5177: Editor: Unexplored map tiles get corrupted after a file with terrain is saved
    Bug #5182: OnPCEquip doesn't trigger on skipped beast race attempts to equip something not equippable by beasts
    Bug #5186: Equipped item enchantments don't affect creatures
    Bug #5190: On-strike enchantments can be applied to and used with non-projectile ranged weapons
    Bug #5196: Dwarven ghosts do not use idle animations
    Bug #5206: A "class does not have NPC stats" error when player's follower kills an enemy with damage spell
    Bug #5209: Spellcasting ignores race height
    Bug #5210: AiActivate allows actors to open dialogue and inventory windows
    Bug #5211: Screen fades in if the first loaded save is in interior cell
    Bug #5212: AiTravel does not work for actors outside of AI processing range
    Bug #5213: SameFaction script function is broken
    Bug #5218: Crash when disabling ToggleBorders
    Bug #5220: GetLOS crashes when actor isn't loaded
    Bug #5222: Empty cell name subrecords are not saved
    Bug #5223: Bow replacement during attack animation removes attached arrow
    Bug #5226: Reputation should be capped
    Bug #5229: Crash if mesh controller node has no data node
    Bug #5239: OpenMW-CS does not support non-ASCII characters in path names
    Bug #5241: On-self absorb spells cannot be detected
    Bug #5242: ExplodeSpell behavior differs from Cast behavior
    Bug #5246: Water ripples persist after cell change
    Bug #5249: Wandering NPCs start walking too soon after they hello
    Bug #5250: Creatures display shield ground mesh instead of shield body part
    Bug #5255: "GetTarget, player" doesn't return 1 during NPC hello
    Bug #5261: Creatures can sometimes become stuck playing idles and never wander again
    Bug #5264: "Damage Fatigue" Magic Effect Can Bring Fatigue below 0
    Bug #5269: Editor: Cell lighting in resaved cleaned content files is corrupted
    Bug #5278: Console command Show doesn't fall back to global variable after local var not found
    Bug #5300: NPCs don't switch from torch to shield when starting combat
    Bug #5308: World map copying makes save loading much slower
    Bug #5313: Node properties of identical type are not applied in the correct order
    Bug #5326: Formatting issues in the settings.cfg
    Bug #5328: Skills aren't properly reset for dead actors
    Bug #5345: Dopey Necromancy does not work due to a missing quote
    Bug #5350: An attempt to launch magic bolt causes "AL error invalid value" error
    Bug #5352: Light source items' duration is decremented while they aren't visible
    Feature #1724: Handle AvoidNode
    Feature #2229: Improve pathfinding AI
    Feature #3025: Analogue gamepad movement controls
    Feature #3442: Default values for fallbacks from ini file
    Feature #3517: Multiple projectiles enchantment
    Feature #3610: Option to invert X axis
    Feature #3871: Editor: Terrain Selection
    Feature #3893: Implicit target for "set" function in console
    Feature #3980: In-game option to disable controller
    Feature #3999: Shift + Double Click should maximize/restore menu size
    Feature #4001: Toggle sneak controller shortcut
    Feature #4068: OpenMW-CS: Add a button to reset key bindings to defaults
    Feature #4129: Beta Comment to File
    Feature #4209: Editor: Faction rank sub-table
    Feature #4255: Handle broken RepairedOnMe script function
    Feature #4316: Implement RaiseRank/LowerRank functions properly
    Feature #4360: Improve default controller bindings
    Feature #4544: Actors movement deceleration
    Feature #4673: Weapon sheathing
    Feature #4675: Support for NiRollController
    Feature #4708: Radial fog support
    Feature #4730: Native animated containers support
    Feature #4784: Launcher: Duplicate Content Lists
    Feature #4812: Support NiSwitchNode
    Feature #4831: Item search in the player's inventory
    Feature #4836: Daytime node switch
    Feature #4840: Editor: Transient terrain change support
    Feature #4859: Make water reflections more configurable
    Feature #4882: Support for NiPalette node
    Feature #4887: Add openmw command option to set initial random seed
    Feature #4890: Make Distant Terrain configurable
    Feature #4944: Pause audio when OpenMW is minimized
    Feature #4958: Support eight blood types
    Feature #4962: Add casting animations for magic items
    Feature #4968: Scalable UI widget skins
    Feature #4994: Persistent pinnable windows hiding
    Feature #5000: Compressed BSA format support
    Feature #5005: Editor: Instance window via Scene window
    Feature #5010: Native graphics herbalism support
    Feature #5031: Make GetWeaponType function return different values for tools
    Feature #5033: Magic armor mitigation for creatures
    Feature #5034: Make enchanting window stay open after a failed attempt
    Feature #5036: Allow scripted faction leaving
    Feature #5046: Gamepad thumbstick cursor speed
    Feature #5051: Provide a separate textures for scrollbars
    Feature #5091: Human-readable light source duration
    Feature #5094: Unix like console hotkeys
    Feature #5098: Allow user controller bindings
    Feature #5114: Refresh launcher mod list
    Feature #5121: Handle NiTriStrips and NiTriStripsData
    Feature #5122: Use magic glow for enchanted arrows
    Feature #5131: Custom skeleton bones
    Feature #5132: Unique animations for different weapon types
    Feature #5146: Safe Dispose corpse
    Feature #5147: Show spell magicka cost in spell buying window
    Feature #5170: Editor: Land shape editing, land selection
    Feature #5172: Editor: Delete instances/references with keypress in scene window
    Feature #5193: Weapon sheathing
    Feature #5201: Editor: Show tool outline in scene view, when using editmodes
    Feature #5219: Impelement TestCells console command
    Feature #5224: Handle NiKeyframeController for NiTriShape
    Feature #5274: Editor: Keyboard shortcut to drop objects to ground/obstacle in scene view
    Feature #5304: Morrowind-style bump-mapping
    Feature #5311: Support for gyroscopic input (e.g. Android)
    Feature #5314: Ingredient filter in the alchemy window
    Task #4686: Upgrade media decoder to a more current FFmpeg API
    Task #4695: Optimize Distant Terrain memory consumption
    Task #4789: Optimize cell transitions
    Task #4721: Add NMake support to the Windows prebuild script

0.45.0
------

    Bug #1875: Actors in inactive cells don't heal from resting
    Bug #1990: Sunrise/sunset not set correct
    Bug #2131: Lustidrike's spell misses the player every time
    Bug #2222: Fatigue's effect on selling price is backwards
    Bug #2256: Landing sound not playing when jumping immediately after landing
    Bug #2274: Thin platform clips through player character instead of lifting
    Bug #2326: After a bound item expires the last equipped item of that type is not automatically re-equipped
    Bug #2446: Restore Attribute/Skill should allow restoring drained attributes
    Bug #2455: Creatures attacks degrade armor
    Bug #2562: Forcing AI to activate a teleport door sometimes causes a crash
    Bug #2626: Resurrecting the player does not resume the game
    Bug #2772: Non-existing class or faction freezes the game
    Bug #2835: Player able to slowly move when overencumbered
    Bug #2852: No murder bounty when a player follower commits murder
    Bug #2862: [macOS] Can't quit launcher using Command-Q or OpenMW->Quit
    Bug #2872: Tab completion in console doesn't work with explicit reference
    Bug #2971: Compiler did not reject lines with naked expressions beginning with x.y
    Bug #3049: Drain and Fortify effects are not properly applied on health, magicka and fatigue
    Bug #3059: Unable to hit with marksman weapons when too close to an enemy
    Bug #3072: Fatal error on AddItem <item> that has a script containing Equip <item>
    Bug #3219: NPC and creature initial position tracing down limit is too small
    Bug #3249: Fixed revert function not updating views properly
    Bug #3288: TrueType fonts are handled incorrectly
    Bug #3374: Touch spells not hitting kwama foragers
    Bug #3486: [Mod] NPC Commands does not work
    Bug #3533: GetSpellEffects should detect effects with zero duration
    Bug #3591: Angled hit distance too low
    Bug #3629: DB assassin attack never triggers creature spawning
    Bug #3681: OpenMW-CS: Clicking Scripts in Preferences spawns many color pickers
    Bug #3762: AddSoulGem and RemoveSpell redundant count arguments break script execution
    Bug #3788: GetPCInJail and GetPCTraveling do not work as in vanilla
    Bug #3836: Script fails to compile when command argument contains "\n"
    Bug #3876: Landscape texture painting is misaligned
    Bug #3890: Magic light source attenuation is inaccurate
    Bug #3897: Have Goodbye give all choices the effects of Goodbye
    Bug #3911: [macOS] Typing in the "Content List name" dialog box produces double characters
    Bug #3920: RemoveSpellEffects doesn't remove constant effects
    Bug #3948: AiCombat moving target aiming uses incorrect speed for magic projectiles
    Bug #3950: FLATTEN_STATIC_TRANSFORMS optimization breaks animated collision shapes
    Bug #3993: Terrain texture blending map is not upscaled
    Bug #3997: Almalexia doesn't pace
    Bug #4036: Weird behaviour of AI packages if package target has non-unique ID
    Bug #4047: OpenMW not reporting its version number in MacOS; OpenMW-CS not doing it fully
    Bug #4110: Fixed undo / redo menu text losing the assigned shortcuts
    Bug #4125: OpenMW logo cropped on bugtracker
    Bug #4215: OpenMW shows book text after last EOL tag
    Bug #4217: Fixme implementation differs from Morrowind's
    Bug #4221: Characters get stuck in V-shaped terrain
    Bug #4230: AiTravel package issues break some Tribunal quests
    Bug #4231: Infected rats from the "Crimson Plague" quest rendered unconscious by change in Drain Fatigue functionality
    Bug #4251: Stationary NPCs do not return to their position after combat
    Bug #4260: Keyboard navigation makes persuasion exploitable
    Bug #4271: Scamp flickers when attacking
    Bug #4274: Pre-0.43 death animations are not forward-compatible with 0.43+
    Bug #4286: Scripted animations can be interrupted
    Bug #4291: Non-persistent actors that started the game as dead do not play death animations
    Bug #4292: CenterOnCell implementation differs from vanilla
    Bug #4293: Faction members are not aware of faction ownerships in barter
    Bug #4304: "Follow" not working as a second AI package
    Bug #4307: World cleanup should remove dead bodies only if death animation is finished
    Bug #4311: OpenMW does not handle RootCollisionNode correctly
    Bug #4327: Missing animations during spell/weapon stance switching
    Bug #4333: Keyboard navigation in containers is not intuitive
    Bug #4358: Running animation is interrupted when magic mode is toggled
    Bug #4368: Settings window ok button doesn't have key focus by default
    Bug #4378: On-self absorb spells restore stats
    Bug #4393: NPCs walk back to where they were after using ResetActors
    Bug #4416: Non-music files crash the game when they are tried to be played
    Bug #4419: MRK NiStringExtraData is handled incorrectly
    Bug #4426: RotateWorld behavior is incorrect
    Bug #4429: [Windows] Error on build INSTALL.vcxproj project (debug) with cmake 3.7.2
    Bug #4431: "Lock 0" console command is a no-op
    Bug #4432: Guards behaviour is incorrect if they do not have AI packages
    Bug #4433: Guard behaviour is incorrect with Alarm = 0
    Bug #4451: Script fails to compile when using "Begin, [ScriptName]" syntax
    Bug #4452: Default terrain texture bleeds through texture transitions
    Bug #4453: Quick keys behaviour is invalid for equipment
    Bug #4454: AI opens doors too slow
    Bug #4457: Item without CanCarry flag prevents shield autoequipping in dark areas
    Bug #4458: AiWander console command handles idle chances incorrectly
    Bug #4459: NotCell dialogue condition doesn't support partial matches
    Bug #4460: Script function "Equip" doesn't bypass beast restrictions
    Bug #4461: "Open" spell from non-player caster isn't a crime
    Bug #4463: %g format doesn't return more digits
    Bug #4464: OpenMW keeps AiState cached storages even after we cancel AI packages
    Bug #4467: Content selector: cyrillic characters are decoded incorrectly in plugin descriptions
    Bug #4469: Abot Silt Striders – Model turn 90 degrees on horizontal
    Bug #4470: Non-bipedal creatures with Weapon & Shield flag have inconsistent behaviour
    Bug #4474: No fallback when getVampireHead fails
    Bug #4475: Scripted animations should not cause movement
    Bug #4479: "Game" category on Advanced page is getting too long
    Bug #4480: Segfault in QuickKeysMenu when item no longer in inventory
    Bug #4489: Goodbye doesn't block dialogue hyperlinks
    Bug #4490: PositionCell on player gives "Error: tried to add local script twice"
    Bug #4494: Training cap based off Base Skill instead of Modified Skill
    Bug #4495: Crossbow animations blending is buggy
    Bug #4496: SpellTurnLeft and SpellTurnRight animation groups are unused
    Bug #4497: File names starting with x or X are not classified as animation
    Bug #4503: Cast and ExplodeSpell commands increase alteration skill
    Bug #4510: Division by zero in MWMechanics::CreatureStats::setAttribute
    Bug #4519: Knockdown does not discard movement in the 1st-person mode
    Bug #4527: Sun renders on water shader in some situations where it shouldn't
    Bug #4531: Movement does not reset idle animations
    Bug #4532: Underwater sfx isn't tied to 3rd person camera
    Bug #4539: Paper Doll is affected by GUI scaling
    Bug #4543: Picking cursed items through inventory (menumode) makes it disappear
    Bug #4545: Creatures flee from werewolves
    Bug #4551: Replace 0 sound range with default range separately
    Bug #4553: Forcegreeting on non-actor opens a dialogue window which cannot be closed
    Bug #4557: Topics with reserved names are handled differently from vanilla
    Bug #4558: Mesh optimizer: check for reserved node name is case-sensitive
    Bug #4560: OpenMW does not update pinned windows properly
    Bug #4563: Fast travel price logic checks destination cell instead of service actor cell
    Bug #4565: Underwater view distance should be limited
    Bug #4573: Player uses headtracking in the 1st-person mode
    Bug #4574: Player turning animations are twitchy
    Bug #4575: Weird result of attack animation blending with movement animations
    Bug #4576: Reset of idle animations when attack can not be started
    Bug #4591: Attack strength should be 0 if player did not hold the attack button
    Bug #4593: Editor: Instance dragging is broken
    Bug #4597: <> operator causes a compile error
    Bug #4604: Picking up gold from the ground only makes 1 grabbed
    Bug #4607: Scaling for animated collision shapes is applied twice
    Bug #4608: Falling damage is applied twice
    Bug #4611: Instant magic effects have 0 duration in custom spell cost calculations unlike vanilla
    Bug #4614: Crash due to division by zero when FlipController has no textures
    Bug #4615: Flicker effects for light sources are handled incorrectly
    Bug #4617: First person sneaking offset is not applied while the character is in air
    Bug #4618: Sneaking is possible while the character is flying
    Bug #4622: Recharging enchanted items with Soul Gems does not award experience if it fails
    Bug #4628: NPC record reputation, disposition and faction rank should have unsigned char type
    Bug #4633: Sneaking stance affects speed even if the actor is not able to crouch
    Bug #4641: GetPCJumping is handled incorrectly
    Bug #4644: %Name should be available for all actors, not just for NPCs
    Bug #4646: Weapon force-equipment messes up ongoing attack animations
    Bug #4648: Hud thinks that throwing weapons have condition
    Bug #4649: Levelup fully restores health
    Bug #4653: Length of non-ASCII strings is handled incorrectly in ESM reader
    Bug #4654: Editor: UpdateVisitor does not initialize skeletons for animated objects
    Bug #4656: Combat AI: back up behaviour is incorrect
    Bug #4668: Editor: Light source color is displayed as an integer
    Bug #4669: ToggleCollision should trace the player down after collision being enabled
    Bug #4671: knownEffect functions should use modified Alchemy skill
    Bug #4672: Pitch factor is handled incorrectly for crossbow animations
    Bug #4674: Journal can be opened when settings window is open
    Bug #4677: Crash in ESM reader when NPC record has DNAM record without DODT one
    Bug #4678: Crash in ESP parser when SCVR has no variable names
    Bug #4684: Spell Absorption is additive
    Bug #4685: Missing sound causes an exception inside Say command
    Bug #4689: Default creature soundgen entries are not used
    Bug #4691: Loading bar for cell should be moved up when text is still active at bottom of screen
    Feature #912: Editor: Add missing icons to UniversalId tables
    Feature #1221: Editor: Creature/NPC rendering
    Feature #1617: Editor: Enchantment effect record verifier
    Feature #1645: Casting effects from objects
    Feature #2606: Editor: Implemented (optional) case sensitive global search
    Feature #2787: Use the autogenerated collision box, if the creature mesh has no predefined one
    Feature #2845: Editor: add record view and preview default keybindings
    Feature #2847: Content selector: allow to copy the path to a file by using the context menu
    Feature #3083: Play animation when NPC is casting spell via script
    Feature #3103: Provide option for disposition to get increased by successful trade
    Feature #3276: Editor: Search - Show number of (remaining) search results and indicate a search without any results
    Feature #3641: Editor: Limit FPS in 3d preview window
    Feature #3703: Ranged sneak attack criticals
    Feature #4012: Editor: Write a log file if OpenCS crashes
    Feature #4222: 360° screenshots
    Feature #4256: Implement ToggleBorders (TB) console command
    Feature #4285: Support soundgen calls for activators
    Feature #4324: Add CFBundleIdentifier in Info.plist to allow for macOS function key shortcuts
    Feature #4345: Add equivalents for the command line commands to Launcher
    Feature #4404: Editor: All EnumDelegate fields should have their items sorted alphabetically
    Feature #4444: Per-group KF-animation files support
    Feature #4466: Editor: Add option to ignore "Base" records when running verifier
    Feature #4488: Make water shader rougher during rain
    Feature #4509: Show count of enchanted items in stack in the spells list
    Feature #4512: Editor: Use markers for lights and creatures levelled lists
    Feature #4548: Weapon priority: use the actual chance to hit the target instead of weapon skill
    Feature #4549: Weapon priority: use the actual damage in weapon rating calculations
    Feature #4550: Weapon priority: make ranged weapon bonus more sensible
    Feature #4579: Add option for applying Strength into hand to hand damage
    Feature #4581: Use proper logging system
    Feature #4624: Spell priority: don't cast hit chance-affecting spells if the enemy is not in respective stance at the moment
    Feature #4625: Weapon priority: use weighted mean for melee damage rating
    Feature #4626: Weapon priority: account for weapon speed
    Feature #4632: AI priority: utilize vanilla AI GMSTs for priority rating
    Feature #4636: Use sTo GMST in spellmaking menu
    Feature #4642: Batching potion creation
    Feature #4647: Cull actors outside of AI processing range
    Feature #4682: Use the collision box from basic creature mesh if the X one have no collisions
    Feature #4697: Use the real thrown weapon damage in tooltips and AI
    Task #2490: Don't open command prompt window on Release-mode builds automatically
    Task #4545: Enable is_pod string test
    Task #4605: Optimize skinning
    Task #4606: Support Rapture3D's OpenAL driver
    Task #4613: Incomplete type errors when compiling with g++ on OSX 10.9
    Task #4621: Optimize combat AI
    Task #4643: Revise editor record verifying functionality
    Task #4645: Use constants instead of widely used magic numbers
    Task #4652: Move call to enemiesNearby() from InputManager::rest() to World::canRest()

0.44.0
------

    Bug #1428: Daedra summoning scripts aren't executed when the item is taken through the inventory
    Bug #1987: Some glyphs are not supported
    Bug #2254: Magic related visual effects are not rendered when loading a saved game
    Bug #2485: Journal alphabetical index doesn't match "Morrowind content language" setting
    Bug #2703: OnPCHitMe is not handled correctly
    Bug #2829: Incorrect order for content list consisting of a game file and an esp without dependencies
    Bug #2841: "Total eclipse" happens if weather settings are not defined.
    Bug #2897: Editor: Rename "Original creature" field
    Bug #3278: Editor: Unchecking "Auto Calc" flag changes certain values
    Bug #3343: Editor: ID sorting is case-sensitive in certain tables
    Bug #3557: Resource priority confusion when using the local data path as installation root
    Bug #3587: Pathgrid and Flying Creatures wrong behaviour – abotWhereAreAllBirdsGoing
    Bug #3603: SetPos should not skip weather transitions
    Bug #3618: Myar Aranath total conversion can't be started due to capital-case extension of the master file
    Bug #3638: Fast forwarding can move NPC inside objects
    Bug #3664: Combat music does not start in dialogue
    Bug #3696: Newlines are accompanied by empty rectangle glyph in dialogs
    Bug #3708: Controllers broken on macOS
    Bug #3726: Items with suppressed activation can be picked up via the inventory menu
    Bug #3783: [Mod] Abot's Silt Striders 1.16 - silt strider "falls" to ground and glides on floor during travel
    Bug #3863: Can be forced to not resist arrest if you cast Calm Humanoid on aggroed death warrant guards
    Bug #3884: Incorrect enemy behavior when exhausted
    Bug #3926: Installation Wizard places Morrowind.esm after Tribunal/Bloodmoon if it has a later file creation date
    Bug #4061: Scripts error on special token included in name
    Bug #4111: Crash when mouse over soulgem with a now-missing soul
    Bug #4122: Swim animation should not be interrupted during underwater attack
    Bug #4134: Battle music behaves different than vanilla
    Bug #4135: Reflecting an absorb spell different from vanilla
    Bug #4136: Enchanted weapons without "ignore normal weapons" flag don't bypass creature "ignore normal weapons" effect
    Bug #4143: Antialiasing produces graphical artifacts when used with shader lighting
    Bug #4159: NPCs' base skeleton files should not be optimized
    Bug #4177: Jumping/landing animation interference/flickering
    Bug #4179: NPCs do not face target
    Bug #4180: Weapon switch sound playing even though no weapon is switched
    Bug #4184: Guards can initiate dialogue even though you are far above them
    Bug #4190: Enchanted clothes changes visibility with Chameleon on equip/unequip
    Bug #4191: "screenshot saved" message also appears in the screenshot image
    Bug #4192: Archers in OpenMW have shorter attack range than archers in Morrowind
    Bug #4210: Some dialogue topics are not highlighted on first encounter
    Bug #4211: FPS drops after minimizing the game during rainy weather
    Bug #4216: Thrown weapon projectile doesn't rotate
    Bug #4223: Displayed spell casting chance must be 0 if player doesn't have enough magicka to cast it
    Bug #4225: Double "Activate" key presses with Mouse and Gamepad.
    Bug #4226: The current player's class should be default value in the class select menu
    Bug #4229: Tribunal/Bloodmoon summoned creatures fight other summons
    Bug #4233: W and A keys override S and D Keys
    Bug #4235: Wireframe mode affects local map
    Bug #4239: Quick load from container screen causes crash
    Bug #4242: Crime greetings display in Journal
    Bug #4245: Merchant NPCs sell ingredients growing on potted plants they own
    Bug #4246: Take armor condition into account when calcuting armor rating
    Bug #4250: Jumping is not as fluid as it was pre-0.43.0
    Bug #4252: "Error in frame: FFmpeg exception: Failed to allocate input stream" message spam if OpenMW encounter non-music file in the Music folder
    Bug #4261: Magic effects from eaten ingredients always have 1 sec duration
    Bug #4263: Arrow position is incorrect in 3rd person view during attack for beast races
    Bug #4264: Player in god mode can be affected by some negative spell effects
    Bug #4269: Crash when hovering the faction section and the 'sAnd' GMST is missing (as in MW 1.0)
    Bug #4272: Root note transformations are discarded again
    Bug #4279: Sometimes cells are not marked as explored on the map
    Bug #4298: Problem with MessageBox and chargen menu interaction order
    Bug #4301: Optimizer breaks LOD nodes
    Bug #4308: PlaceAtMe doesn't inherit scale of calling object
    Bug #4309: Only harmful effects with resistance effect set are resistable
    Bug #4313: Non-humanoid creatures are capable of opening doors
    Bug #4314: Rainy weather slows down the game when changing from indoors/outdoors
    Bug #4319: Collisions for certain meshes are incorrectly ignored
    Bug #4320: Using mouse 1 to move forward causes selection dialogues to jump selections forward.
    Bug #4322: NPC disposition: negative faction reaction modifier doesn't take PC rank into account
    Bug #4328: Ownership by dead actors is not cleared from picked items
    Bug #4334: Torch and shield usage inconsistent with original game
    Bug #4336: Wizard: Incorrect Morrowind assets path autodetection
    Bug #4343: Error message for coc and starting cell shouldn't imply that it only works for interior cells
    Bug #4346: Count formatting does not work well with very high numbers
    Bug #4351: Using AddSoulgem fills all soul gems of the specified type
    Bug #4391: No visual indication is provided when an unavailable spell fails to be chosen via a quick key
    Bug #4392: Inventory filter breaks after loading a game
    Bug #4405: No default terrain in empty cells when distant terrain is enabled
    Bug #4410: [Mod] Arktwend: OpenMW does not use default marker definitions
    Bug #4412: openmw-iniimporter ignores data paths from config
    Bug #4413: Moving with 0 strength uses all of your fatigue
    Bug #4420: Camera flickering when I open up and close menus while sneaking
    Bug #4424: [macOS] Cursor is either empty or garbage when compiled against macOS 10.13 SDK
    Bug #4435: Item health is considered a signed integer
    Bug #4441: Adding items to currently disabled weapon-wielding creatures crashes the game
    Feature #1786: Round up encumbrance value in the encumbrance bar
    Feature #2694: Editor: rename "model" column to make its purpose clear
    Feature #3870: Editor: Terrain Texture Brush Button
    Feature #3872: Editor: Edit functions in terrain texture editing mode
    Feature #4054: Launcher: Create menu for settings.cfg options
    Feature #4064: Option for fast travel services to charge for the first companion
    Feature #4142: Implement fWereWolfHealth GMST
    Feature #4174: Multiple quicksaves
    Feature #4407: Support NiLookAtController
    Feature #4423: Rebalance soul gem values
    Task #4015: Use AppVeyor build artifact features to make continuous builds available
    Editor: New (and more complete) icon set

0.43.0
------

    Bug #815: Different settings cause inconsistent underwater visibility
    Bug #1452: autosave is not executed when waiting
    Bug #1555: Closing containers with spacebar doesn't work after touching an item
    Bug #1692: Can't close container when item is "held"
    Bug #2405: Maximum distance for guards attacking hostile creatures is incorrect
    Bug #2445: Spellcasting can be interrupted
    Bug #2489: Keeping map open not persisted between saves
    Bug #2594: 1st person view uses wrong body texture with Better bodies
    Bug #2628: enablestatreviewmenu command doen't read race, class and sign values from current game
    Bug #2639: Attacking flag isn't reset upon reloading
    Bug #2698: Snow and rain VFX move with the player
    Bug #2704: Some creature swim animations not being used
    Bug #2789: Potential risk of misunderstanding using the colored "owned" crosshair feature
    Bug #3045: Settings containing '#' cannot be loaded
    Bug #3097: Drop() doesn't work when an item is held (with the mouse)
    Bug #3110: GetDetected doesn't work without a reference
    Bug #3126: Framerate nosedives when adjusting dialogue window size
    Bug #3243: Ampersand in configuration files isn't escaped automatically
    Bug #3365: Wrong water reflection along banks
    Bug #3441: Golden saint always dispelling soul trap / spell priority issue
    Bug #3528: Disposing of corpses breaks quests
    Bug #3531: No FPS limit when playing bink videos even though "framerate limit" is set in settings.cfg
    Bug #3647: Multi-effect spells play audio louder than in Vanilla
    Bug #3656: NPCs forget where their place in the world is
    Bug #3665: Music transitions are too abrupt
    Bug #3679: Spell cast effect should disappear after using rest command
    Bug #3684: Merchants do not restock empty soul gems if they acquire filled ones.
    Bug #3694: Wrong magicka bonus applied on character creation
    Bug #3706: Guards don't try to arrest the player if attacked
    Bug #3709: Editor: Camera is not positioned correctly on mode switches related to orbital mode
    Bug #3720: Death counter not cleaned of non-existing IDs when loading a game
    Bug #3744: "Greater/lesser or equal" operators are not parsed when their signs are swapped
    Bug #3749: Yagrum Bagarn moves to different position on encountering
    Bug #3766: DisableLevitation does not remove visuals of preexisting effect
    Bug #3787: Script commands in result box for voiced dialogue are ignored
    Bug #3793: OpenMW tries to animate animated references even when they are disabled
    Bug #3794: Default sound buffer size is too small for mods
    Bug #3796: Mod 'Undress for me' doesn't work: NPCs re-equip everything
    Bug #3798: tgm command behaviour differs from vanilla
    Bug #3804: [Mod] Animated Morrowind: some animations do not loop correctly
    Bug #3805: Slight enchant miscalculation
    Bug #3826: Rendering problems with an image in a letter
    Bug #3833: [Mod] Windows Glow: windows textures are much darker than in original game
    Bug #3835: Bodyparts with multiple NiTriShapes are not handled correctly
    Bug #3839: InventoryStore::purgeEffect() removes only first effect with argument ID
    Bug #3843: Wrong jumping fatigue loss calculations
    Bug #3850: Boethiah's voice is distorted underwater
    Bug #3851: NPCs and player say things while underwater
    Bug #3864: Crash when exiting to Khartag point from Ilunibi
    Bug #3878: Swapping soul gems while enchanting allows constant effect enchantments using any soul gem
    Bug #3879: Dialogue option: Go to jail, persists beyond quickload
    Bug #3891: Journal displays empty entries
    Bug #3892: Empty space before dialogue entry display
    Bug #3898: (mod) PositionCell in dialogue results closes dialogue window
    Bug #3906: "Could not find Data Files location" dialog can appear multiple times
    Bug #3908: [Wizard] User gets stuck if they cancel out of installing from a CD
    Bug #3909: Morrowind Content Language dropdown is the only element on the right half of the Settings window
    Bug #3910: Launcher window can be resized so that it cuts off the scroll
    Bug #3915: NC text key on nifs doesn't work
    Bug #3919: Closing inventory while cursor hovers over spell (or other magic menu item) produces left click sound
    Bug #3922: Combat AI should avoid enemy hits when casts Self-ranged spells
    Bug #3934: [macOS] Copy/Paste from system clipboard uses Control key instead of Command key
    Bug #3935: Incorrect attack strength for AI actors
    Bug #3937: Combat AI: enchanted weapons have too high rating
    Bug #3942: UI sounds are distorted underwater
    Bug #3943: CPU/GPU usage should stop when the game is minimised
    Bug #3944: Attempting to sell stolen items back to their owner does not remove them from your inventory
    Bug #3955: Player's avatar rendering issues
    Bug #3956: EditEffectDialog: Cancel button does not update a Range button and an Area slider properly
    Bug #3957: Weird bodypart rendering if a node has reserved name
    Bug #3960: Clothes with high cost (> 32768) are not handled properly
    Bug #3963: When on edge of being burdened the condition doesn't lower as you run.
    Bug #3971: Editor: Incorrect colour field in cell table
    Bug #3974: Journal page turning doesn't produce sounds
    Bug #3978: Instant opening and closing happens when using a Controller with Menus/Containers
    Bug #3981: Lagging when spells are cast, especially noticeable on new landmasses such as Tamriel Rebuilt
    Bug #3982: Down sounds instead of Up ones are played when trading
    Bug #3987: NPCs attack after some taunting with no "Goodbye"
    Bug #3991: Journal can still be opened at main menu
    Bug #3995: Dispel cancels every temporary magic effect
    Bug #4002: Build broken on OpenBSD with clang
    Bug #4003: Reduce Render Area of Inventory Doll to Fit Within Border
    Bug #4004: Manis Virmaulese attacks without saying anything
    Bug #4010: AiWander: "return to the spawn position" feature does not work properly
    Bug #4016: Closing menus with spacebar will still send certain assigned actions through afterwards
    Bug #4017: GetPCRunning and GetPCSneaking should check that the PC is actually moving
    Bug #4024: Poor music track distribution
    Bug #4025: Custom spell with copy-pasted name always sorts to top of spell list
    Bug #4027: Editor: OpenMW-CS misreports its own name as "OpenCS", under Mac OS
    Bug #4033: Archers don't attack if the arrows have run out and there is no other weapon
    Bug #4037: Editor: New greetings do not work in-game.
    Bug #4049: Reloading a saved game while falling prevents damage
    Bug #4056: Draw animation should not be played when player equips a new weapon
    Bug #4074: Editor: Merging of LAND/LTEX records
    Bug #4076: Disposition bar is not updated when "goodbye" selected in dialogue
    Bug #4079: Alchemy skill increases do not take effect until next batch
    Bug #4093: GetResistFire, getResistFrost and getResistShock doesn't work as in vanilla
    Bug #4094: Level-up messages for levels past 20 are hardcoded not to be used
    Bug #4095: Error in framelistener when take all items from a dead corpse
    Bug #4096: Messagebox with the "%0.f" format should use 0 digit precision
    Bug #4104: Cycling through weapons does not skip broken ones
    Bug #4105: birthsign generation menu does not show full details
    Bug #4107: Editor: Left pane in Preferences window is too narrow
    Bug #4112: Inventory sort order is inconsistent
    Bug #4113: 'Resolution not supported in fullscreen' message is inconvenient
    Bug #4131: Pickpocketing behaviour is different from vanilla
    Bug #4155: NPCs don't equip a second ring in some cases
    Bug #4156: Snow doesn't create water ripples
    Bug #4165: NPCs autoequip new clothing with the same price
    Feature #452: Rain-induced water ripples
    Feature #824: Fading for doors and teleport commands
    Feature #933: Editor: LTEX record table
    Feature #936: Editor: LAND record table
    Feature #1374: AI: Resurface to breathe
    Feature #2320: ess-Importer: convert projectiles
    Feature #2509: Editor: highlighting occurrences of a word in a script
    Feature #2748: Editor: Should use one resource manager per document
    Feature #2834: Have openMW's UI remember what menu items were 'pinned' across boots.
    Feature #2923: Option to show the damage of the arrows through tooltip.
    Feature #3099: Disabling inventory while dragging an item forces you to drop it
    Feature #3274: Editor: Script Editor - Shortcuts and context menu options for commenting code out and uncommenting code respectively
    Feature #3275: Editor: User Settings- Add an option to reset settings to their default status (per category / all)
    Feature #3400: Add keyboard shortcuts for menus
    Feature #3492: Show success rate while enchanting
    Feature #3530: Editor: Reload data files
    Feature #3682: Editor: Default key binding reset
    Feature #3921: Combat AI: aggro priorities
    Feature #3941: Allow starting at an unnamed exterior cell with --start
    Feature #3952: Add Visual Studio 2017 support
    Feature #3953: Combat AI: use "WhenUsed" enchantments
    Feature #4082: Leave the stack of ingredients or potions grabbed after using an ingredient/potion
    Task #2258: Windows installer: launch OpenMW tickbox
    Task #4152: The Windows CI script is moving files around that CMake should be dealing with

0.42.0
------

    Bug #1956: Duplicate objects after loading the game, when a mod was edited
    Bug #2100: Falling leaves in Vurt's Leafy West Gash II not rendered correctly
    Bug #2116: Cant fit through some doorways pressed against staircases
    Bug #2289: Some modal dialogs are not centered on the screen when the window resizes
    Bug #2409: Softlock when pressing weapon/magic switch keys during chargen, afterwards switches weapons even though a text field is selected
    Bug #2483: Previous/Next Weapon hotkeys triggered while typing the name of game save
    Bug #2629: centeroncell, coc causes death / fall damage time to time when teleporting from high
    Bug #2645: Cycling weapons is possible while console/pause menu is open
    Bug #2678: Combat with water creatures do not end upon exiting water
    Bug #2759: Light Problems in Therana's Chamber in Tel Branora
    Bug #2771: unhandled sdl event of type 0x302
    Bug #2777: (constant/on cast) disintegrate armor/weapon on self is seemingly not working
    Bug #2838: Editor: '.' in a record name should be allowed
    Bug #2909: NPCs appear floating when standing on a slope
    Bug #3093: Controller movement cannot be used while mouse is moving
    Bug #3134: Crash possible when using console with open container
    Bug #3254: AI enemies hit between them.
    Bug #3344: Editor: Verification results sorting by Type is not alphabetical.
    Bug #3345: Editor: Cloned and added pathgrids are lost after reopen of saved omwgame file
    Bug #3355: [MGSO] Physics maxing out in south cornerclub Balmora
    Bug #3484: Editor: camera position is not set when changing cell via drag&drop
    Bug #3508: Slowfall kills Jump momentum
    Bug #3580: Crash: Error ElementBufferObject::remove BufferData<0> out of range
    Bug #3581: NPCs wander too much
    Bug #3601: Menu Titles not centered vertically
    Bug #3607: [Mac OS] Beginning of NPC speech cut off (same issue as closed bug #3453)
    Bug #3613: Can not map "next weapon" or "next spell" to controller
    Bug #3617: Enchanted arrows don't explode when hitting the ground
    Bug #3645: Unable to use steps in Vivec, Palace of Vivec
    Bug #3650: Tamriel Rebuilt 16.09.1 – Hist Cuirass GND nif is rendered inside a Pink Box
    Bug #3652: Item icon shadows get stuck in the alchemy GUI
    Bug #3653: Incorrect swish sounds
    Bug #3666: NPC collision should not be disabled until death animation has finished
    Bug #3669: Editor: Text field was missing from book object editing dialogue
    Bug #3670: Unhandled SDL event of type 0x304
    Bug #3671: Incorrect local variable value after picking up bittercup
    Bug #3686: Travelling followers doesn't increase travel fee
    Bug #3689: Problematic greetings from Antares Big Mod that override the appropriate ones.
    Bug #3690: Certain summoned creatures do not engage in combat with underwater creatures
    Bug #3691: Enemies do not initiate combat with player followers on sight
    Bug #3695: [Regression] Dispel does not always dispel spell effects in 0.41
    Bug #3699: Crash on MWWorld::ProjectileManager::moveMagicBolts
    Bug #3700: Climbing on rocks and mountains
    Bug #3704: Creatures don't auto-equip their shields on creation
    Bug #3705: AI combat engagement logic differs from vanilla
    Bug #3707: Animation playing does some very odd things if pc comes in contact with the animated mesh
    Bug #3712: [Mod] Freeze upon entering Adanumuran with mod Adanumuran Reclaimed
    Bug #3713: [Regression] Cancelling dialogue or using travel with creatures throws a (possibly game-breaking) exception
    Bug #3719: Dropped identification papers can't be picked up again
    Bug #3722: Command spell doesn't bring enemies out of combat
    Bug #3727: Using "Activate" mid-script-execution invalidates interpreter context
    Bug #3746: Editor: Book records show attribute IDs instead of skill IDs for teached skills entry.
    Bug #3755: Followers stop following after loading from savegame
    Bug #3772: ModStat lowers attribute to 100 if it was greater
    Bug #3781: Guns in Clean Hunter Rifles mod use crossbow sounds
    Bug #3797: NPC and creature names don't show up in combat when RMB windows are displayed
    Bug #3800: Wrong tooltip maximum width
    Bug #3801: Drowning widget is bugged
    Bug #3802: BarterOffer shouldn't limit pcMercantile
    Bug #3813: Some fatal error
    Bug #3816: Expression parser thinks the -> token is unexpected when a given explicit refID clashes with a journal ID
    Bug #3822: Custom added creatures are not animated
    Feature #451: Water sounds
    Feature #2691: Light particles sometimes not shown in inventory character preview
    Feature #3523: Light source on magic projectiles
    Feature #3644: Nif NiSphericalCollider Unknown Record Type
    Feature #3675: ess-Importer: convert mark location
    Feature #3693: ess-Importer: convert last known exterior cell
    Feature #3748: Editor: Replace "Scroll" check box in Book records with "Book Type" combo box.
    Feature #3751: Editor: Replace "Xyz Blood" check boxes in NPC and Creature records with "Blood Type" combo box
    Feature #3752: Editor: Replace emitter check boxes in Light records with "Emitter Type" combo box
    Feature #3756: Editor: Replace "Female" check box in NPC records with "Gender" combo box
    Feature #3757: Editor: Replace "Female" check box in BodyPart records with "Gender" combo box
    Task #3092: const version of ContainerStoreIterator
    Task #3795: /deps folder not in .gitignore

0.41.0
------

    Bug #1138: Casting water walking doesn't move the player out of the water
    Bug #1931: Rocks from blocked passage in Bamz-Amschend, Radacs Forge can reset and cant be removed again.
    Bug #2048: Almvisi and Divine Intervention display wrong spell effect
    Bug #2054: Show effect-indicator for "instant effect" spells and potions
    Bug #2150: Clockwork City door animation problem
    Bug #2288: Playback of weapon idle animation not correct
    Bug #2410: Stat-review window doesn't display starting spells, powers, or abilities
    Bug #2493: Repairing occasionally very slow
    Bug #2716: [OSG] Water surface is too transparent from some angles
    Bug #2859: [MAC OS X] Cannot exit fullscreen once enabled
    Bug #3091: Editor: will not save addon if global variable value type is null
    Bug #3277: Editor: Non-functional nested tables in subviews need to be hidden instead of being disabled
    Bug #3348: Disabled map markers show on minimap
    Bug #3350: Extending selection to instances with same object results in duplicates.
    Bug #3353: [Mod] Romance version 3.7 script failed
    Bug #3376: [Mod] Vampire Embrace script fails to execute
    Bug #3385: Banners don't animate in stormy weather as they do in the original game
    Bug #3393: Akulakhan re-enabled after main quest
    Bug #3427: Editor: OpenMW-CS instances won´t get deleted
    Bug #3451: Feril Salmyn corpse isn't where it is supposed to be
    Bug #3497: Zero-weight armor is displayed as "heavy" in inventory tooltip
    Bug #3499: Idle animations don't always loop
    Bug #3500: Spark showers at Sotha Sil do not appear until you look at the ceiling
    Bug #3515: Editor: Moved objects in interior cells are teleported to exterior cells.
    Bug #3520: Editor: OpenMW-CS cannot find project file when launching the game
    Bug #3521: Armed NPCs don't use correct melee attacks
    Bug #3535: Changing cell immediately after dying causes character to freeze.
    Bug #3542: Unable to rest if unalerted slaughterfish are in the cell with you
    Bug #3549: Blood effects occur even when a hit is resisted
    Bug #3551: NPC Todwendy in german version can't interact
    Bug #3552: Opening the journal when fonts are missing results in a crash
    Bug #3555: SetInvisible command should not apply graphic effect
    Bug #3561: Editor: changes from omwaddon are not loaded in [New Addon] mode
    Bug #3562: Non-hostile NPCs can be disarmed by stealing their weapons via sneaking
    Bug #3564: Editor: openmw-cs verification results
    Bug #3568: Items that should be invisible are shown in the inventory
    Bug #3574: Alchemy: Alembics and retorts are used in reverse
    Bug #3575: Diaglog choices don't work in mw 0.40
    Bug #3576: Minor differences in AI reaction to hostile spell effects
    Bug #3577: not local nolore dialog test
    Bug #3578: Animation Replacer hangs after one cicle/step
    Bug #3579: Bound Armor skillups and sounds
    Bug #3583: Targetted GetCurrentAiPackage returns 0
    Bug #3584: Persuasion bug
    Bug #3590: Vendor, Ilen Faveran, auto equips items from stock
    Bug #3594: Weather doesn't seem to update correctly in Mournhold
    Bug #3598: Saving doesn't save status of objects
    Bug #3600: Screen goes black when trying to travel to Sadrith Mora
    Bug #3608: Water ripples aren't created when walking on water
    Bug #3626: Argonian NPCs swim like khajiits
    Bug #3627: Cannot delete "Blessed touch" spell from spellbook
    Bug #3634: An enchanted throwing weapon consumes charges from the stack in your inventory. (0.40.0)
    Bug #3635: Levelled items in merchants are "re-rolled" (not bug 2952, see inside)
    Feature #1118: AI combat: flee
    Feature #1596: Editor: Render water
    Feature #2042: Adding a non-portable Light to the inventory should cause the player to glow
    Feature #3166: Editor: Instance editing mode - rotate sub mode
    Feature #3167: Editor: Instance editing mode - scale sub mode
    Feature #3420: ess-Importer: player control flags
    Feature #3489: You shouldn't be be able to re-cast a bound equipment spell
    Feature #3496: Zero-weight boots should play light boot footsteps
    Feature #3516: Water Walking should give a "can't cast" message and fail when you are too deep
    Feature #3519: Play audio and visual effects for all effects in a spell
    Feature #3527: Double spell explosion scaling
    Feature #3534: Play particle textures for spell effects
    Feature #3539: Make NPCs use opponent's weapon range to decide whether to dodge
    Feature #3540: Allow dodging for creatures with "biped" flag
    Feature #3545: Drop shadow for items in menu
    Feature #3558: Implement same spell range for "on touch" spells as original engine
    Feature #3560: Allow using telekinesis with touch spells on objects
    Task #3585: Some objects added by Morrowind Rebirth do not display properly their texture

0.40.0
------

    Bug #1320: AiWander - Creatures in cells without pathgrids do not wander
    Bug #1873: Death events are triggered at the beginning of the death animation
    Bug #1996: Resting interrupts magic effects
    Bug #2399: Vampires can rest in broad daylight and survive the experience
    Bug #2604: Incorrect magicka recalculation
    Bug #2721: Telekinesis extends interaction range where it shouldn't
    Bug #2981: When waiting, NPCs can go where they wouldn't go normally.
    Bug #3045: Esp files containing the letter '#' in the file name cannot be loaded on startup
    Bug #3071: Slowfall does not stop momentum when jumping
    Bug #3085: Plugins can not replace parent cell references with a cell reference of different type
    Bug #3145: Bug with AI Cliff Racer. He will not attack you, unless you put in front of him.
    Bug #3149: Editor: Weather tables were missing from regions
    Bug #3201: Netch shoots over your head
    Bug #3269: If you deselect a mod and try to load a save made inside a cell added by it, you end bellow the terrain in the grid 0/0
    Bug #3286: Editor: Script editor tab width
    Bug #3329: Teleportation spells cause crash to desktop after build update from 0.37 to 0.38.0
    Bug #3331: Editor: Start Scripts table: Adding a script doesn't refresh the list of Start Scripts and allows to add a single script multiple times
    Bug #3332: Editor: Scene view: Tool tips only occur when holding the left mouse button
    Bug #3340: ESS-Importer does not separate item stacks
    Bug #3342: Editor: Creation of pathgrids did not check if the pathgrid already existed
    Bug #3346: "Talked to PC" is always 0 for "Hello" dialogue
    Bug #3349: AITravel doesn't repeat
    Bug #3370: NPCs wandering to invalid locations after training
    Bug #3378: "StopCombat" command does not function in vanilla quest
    Bug #3384: Battle at Nchurdamz - Larienna Macrina does not stop combat after killing Hrelvesuu
    Bug #3388: Monster Respawn tied to Quicksave
    Bug #3390: Strange visual effect in Dagoth Ur's chamber
    Bug #3391: Inappropriate Blight weather behavior at end of main quest
    Bug #3394: Replaced dialogue inherits some of its old data
    Bug #3397: Actors that start the game dead always have the same death pose
    Bug #3401: Sirollus Saccus sells not glass arrows
    Bug #3402: Editor: Weapon data not being properly set
    Bug #3405: Mulvisic Othril will not use her chitin throwing stars
    Bug #3407: Tanisie Verethi will immediately detect the player
    Bug #3408: Improper behavior of ashmire particles
    Bug #3412: Ai Wander start time resets when saving/loading the game
    Bug #3416: 1st person and 3rd person camera isn't converted from .ess correctly
    Bug #3421: Idling long enough while paralyzed sometimes causes character to get stuck
    Bug #3423: Sleep interruption inside dungeons too agressive
    Bug #3424: Pickpocketing sometimes won't work
    Bug #3432: AiFollow / AiEscort durations handled incorrectly
    Bug #3434: Dead NPC's and Creatures still contribute to sneak skill increases
    Bug #3437: Weather-conditioned dialogue should not play in interiors
    Bug #3439: Effects cast by summon stick around after their death
    Bug #3440: Parallax maps looks weird
    Bug #3443: Class graphic for custom class should be Acrobat
    Bug #3446: OpenMW segfaults when using Atrayonis's "Anthology Solstheim: Tomb of the Snow Prince" mod
    Bug #3448: After dispelled, invisibility icon is still displayed
    Bug #3453: First couple of seconds of NPC speech is muted
    Bug #3455: Portable house mods lock player and npc movement up exiting house.
    Bug #3456: Equipping an item will undo dispel of constant effect invisibility
    Bug #3458: Constant effect restore health doesn't work during Wait
    Bug #3466: It is possible to stack multiple scroll effects of the same type
    Bug #3471: When two mods delete the same references, many references are not disabled by the engine.
    Bug #3473: 3rd person camera can be glitched
    Feature #1424: NPC "Face" function
    Feature #2974: Editor: Multiple Deletion of Subrecords
    Feature #3044: Editor: Render path grid v2
    Feature #3362: Editor: Configurable key bindings
    Feature #3375: Make sun / moon reflections weather dependent
    Feature #3386: Editor: Edit pathgrid

0.39.0
------

    Bug #1384: Dark Brotherhood Assassin (and other scripted NPCs?) spawns beneath/inside solid objects
    Bug #1544: "Drop" drops equipped item in a separate stack
    Bug #1587: Collision detection glitches
    Bug #1629: Container UI locks up in Vivec at Jeanne's
    Bug #1771: Dark Brotherhood Assassin oddity in Eight Plates
    Bug #1827: Unhandled NiTextureEffect in ex_dwrv_ruin30.nif
    Bug #2089: When saving while swimming in water in an interior cell, you will be spawned under water on loading
    Bug #2295: Internal texture not showing, nipixeldata
    Bug #2363: Corpses don't disappear
    Bug #2369: Respawns should be timed individually
    Bug #2393: Сharacter is stuck in the tree
    Bug #2444: [Mod] NPCs from Animated Morrowind appears not using proper animations
    Bug #2467: Creatures do not respawn
    Bug #2515: Ghosts in Ibar-Dad spawn stuck in walls
    Bug #2610: FixMe script still needs to be implemented
    Bug #2689: Riekling raider pig constantly screams while running
    Bug #2719: Vivec don't put their hands on the knees with this replacer (Psymoniser Vivec God Replacement NPC Edition v1.0
    Bug #2737: Camera shaking when side stepping around object
    Bug #2760: AI Combat Priority Problem - Use of restoration spell instead of attacking
    Bug #2806: Stack overflow in LocalScripts::getNext
    Bug #2807: Collision detection allows player to become stuck inside objects
    Bug #2814: Stairs to Marandus have improper collision
    Bug #2925: Ranes Ienith will not appear, breaking the Morag Tong and Thieves Guid questlines
    Bug #3024: Editor: Creator bar in startscript subview does not accept script ID drops
    Bug #3046: Sleep creature: Velk is spawned half-underground in the Thirr River Valley
    Bug #3080: Calling aifollow without operant in local script every frame causes mechanics to overheat + log
    Bug #3101: Regression: White guar does not move
    Bug #3108: Game Freeze after Killing Diseased Rat in Foreign Quarter Tomb
    Bug #3124: Bloodmoon Quest - Rite of the Wolf Giver (BM_WolfGiver) – Innocent victim won't turn werewolf
    Bug #3125: Improper dialogue window behavior when talking to creatures
    Bug #3130: Some wandering NPCs disappearing, cannot finish quests
    Bug #3132: Editor: GMST ID named sMake Enchantment is instead named sMake when making new game from scratch
    Bug #3133: OpenMW and the OpenCS are writting warnings about scripts that use the function GetDisabled.
    Bug #3135: Journal entry for The Pigrim's Path missing name
    Bug #3136: Dropped bow is displaced
    Bug #3140: Editor: OpenMW-CS fails to open newly converted and saved omwaddon file.
    Bug #3142: Duplicate Resist Magic message
    Bug #3143: Azura missing her head
    Bug #3146: Potion effect showing when ingredient effects are not known
    Bug #3155: When executing chop attack with a spear, hands turn partly invisible
    Bug #3161: Fast travel from Silt Strider or Boat Ride will break save files made afterwards
    Bug #3163: Editor: Objects dropped to scene do not always save
    Bug #3173: Game Crashes After Casting Recall Spell
    Bug #3174: Constant effect enchantments play spell animation on dead bodies
    Bug #3175: Spell effects do not wear down when caster dies
    Bug #3176: NPCs appearing randomly far away from towns
    Bug #3177: Submerged corpse floats ontop of water when it shouldn't (Widow Vabdas' Deed quest)
    Bug #3184: Bacola Closcius in Balmora, South Wall Cornerclub spams magic effects if attacked
    Bug #3207: Editor: New objects do not render
    Bug #3212: Arrow of Ranged Silence
    Bug #3213: Looking at Floor After Magical Transport
    Bug #3220: The number of remaining ingredients in the alchemy window doesn't go down when failing to brew a potion
    Bug #3222: Falling through the water in Vivec
    Bug #3223: Crash at the beginning with MOD (The Symphony)
    Bug #3228: Purple screen when leveling up.
    Bug #3233: Infinite disposition via MWDialogue::Filter::testDisposition() glitch
    Bug #3234: Armor mesh stuck on body in inventory menu
    Bug #3235: Unlike vanilla, OpenMW don't allow statics and activators cast effects on the player.
    Bug #3238: Not loading cells when using Poorly Placed Object Fix.esm
    Bug #3248: Editor: Using the "Next Script" and "Previous Script" buttons changes the record status to "Modified"
    Bug #3258: Woman biped skeleton
    Bug #3259: No alternating punches
    Bug #3262: Crash in class selection menu
    Bug #3279: Load menu: Deleting a savegame makes scroll bar jump to the top
    Bug #3326: Starting a new game, getting to class selection, then starting another new game temporarily assigns Acrobat class
    Bug #3327: Stuck in table after loading when character was sneaking when quicksave
    Feature #652: Editor: GMST verifier
    Feature #929: Editor: Info record verifier
    Feature #1279: Editor: Render cell border markers
    Feature #2482: Background cell loading and caching of loaded cells
    Feature #2484: Editor: point lighting
    Feature #2801: Support NIF bump map textures in osg
    Feature #2926: Editor: Optional line wrap in script editor wrap lines
    Feature #3000: Editor: Reimplement 3D scene camera system
    Feature #3035: Editor: Make scenes a drop target for referenceables
    Feature #3043: Editor: Render cell markers v2
    Feature #3164: Editor: Instance Selection Menu
    Feature #3165: Editor: Instance editing mode - move sub mode
    Feature #3244: Allow changing water Level of Interiors behaving like exteriors
    Feature #3250: Editor: Use "Enter" key instead of clicking "Create" button to confirm ID input in Creator Bar
    Support #3179: Fatal error on startup

0.38.0
------

    Bug #1699: Guard will continuously run into mudcrab
    Bug #1934: Saw in Dome of Kasia doesnt harm the player
    Bug #1962: Rat floats when killed near the door
    Bug #1963: Kwama eggsacks pulse too fast
    Bug #2198: NPC voice sound source should be placed at their head
    Bug #2210: OpenMW installation wizard crashes...
    Bug #2211: Editor: handle DELE subrecord at the end of a record
    Bug #2413: ESM error Unknown subrecord in Grandmaster of Hlaalu
    Bug #2537: Bloodmoon quest Ristaag: Sattir not consistently dying, plot fails to advance; same with Grerid
    Bug #2697: "The Swimmer" moves away after leading you to underwater cave
    Bug #2724: Loading previous save duplicates containers and harvestables
    Bug #2769: Inventory doll - Cursor not respecting order of clothes
    Bug #2865: Scripts silently fail when moving NPCs between cells.
    Bug #2873: Starting a new game leads to CTD / Fatal Error
    Bug #2918: Editor: it's not possible to create an omwaddon containing a dot in the file name
    Bug #2933: Dialog box can't disable a npc if it is in another cell. (Rescue Madura Seran).
    Bug #2942: atronach sign behavior (spell absorption) changes when trying to receive a blessing at "shrine of tribunal"
    Bug #2952: Enchantment Merchant Items reshuffled EVERY time 'barter' is clicked
    Bug #2961: ESM Error: Unknown subrecord if Deus Ex Machina mod is loaded
    Bug #2972: Resurrecting the player via console does not work when health was 0
    Bug #2986: Projectile weapons work underwater
    Bug #2988: "Expected subrecord" bugs showing up.
    Bug #2991: Can't use keywords in strings for MessageBox
    Bug #2993: Tribunal:The Shrine of the Dead – Urvel Dulni can't stop to follow the player.
    Bug #3008: NIFFile Error while loading meshes with a NiLODNode
    Bug #3010: Engine: items should sink to the ground when dropped under water
    Bug #3011: NIFFile Error while loading meshes with a NiPointLight
    Bug #3016: Engine: something wrong with scripting - crash / fatal error
    Bug #3020: Editor: verify does not check if given "item ID" (as content) for a "container" exists
    Bug #3026: [MOD: Julan Ashlander Companion] Dialogue not triggering correctly
    Bug #3028: Tooltips for Health, Magicka and Fatigue show in Options menu even when bars aren't visible
    Bug #3034: Item count check dialogue option doesn't work (Guards accept gold even if you don't have enough)
    Bug #3036: Owned tooltip color affects spell tooltips incorrrectly
    Bug #3037: Fatal error loading old ES_Landscape.esp in Store<ESM::LandTexture>::search
    Bug #3038: Player sounds come from underneath
    Bug #3040: Execution of script failed: There is a message box already
    Bug #3047: [MOD: Julan Ashlander Companion] Scripts KS_Bedscript or KS_JulanNight not working as intended
    Bug #3048: Fatal Error
    Bug #3051: High field of view results in first person rendering glitches
    Bug #3053: Crash on new game at character class selection
    Bug #3058: Physiched sleeves aren't rendered correctly.
    Bug #3060: NPCs use wrong landing sound
    Bug #3062: Mod support regression: Andromeda's fast travel.
    Bug #3063: Missing Journal Textures without Tribunal and Bloodmoon installed
    Bug #3077: repeated aifollow causes the distance to stack
    Bug #3078: Creature Dialogues not showing when certain Function/Conditions are required.
    Bug #3082: Crash when entering Holamayan Monastery with mesh replacer installed
    Bug #3086: Party at Boro's House – Creature with Class don't talk under OpenMW
    Bug #3089: Dreamers spawn too soon
    Bug #3100: Certain controls erroneously work as a werewolf
    Bug #3102: Multiple unique soultrap spell sources clone souls.
    Bug #3105: Summoned creatures and objects disappear at midnight
    Bug #3112: gamecontrollerdb file creation with wrong extension
    Bug #3116: Dialogue Function "Same Race" is avoided
    Bug #3117: Dialogue Bug: Choice conditions are tested when not in a choice
    Bug #3118: Body Parts are not rendered when used in a pose.
    Bug #3122: NPC direction is reversed during sneak awareness check
    Feature #776: Sound effects from one direction don't necessarily affect both speakers in stereo
    Feature #858: Different fov settings for hands and the game world
    Feature #1176: Handle movement of objects between cells
    Feature #2507: Editor: choosing colors for syntax highlighting
    Feature #2867: Editor: hide script error list when there are no errors
    Feature #2885: Accept a file format other than nif
    Feature #2982: player->SetDelete 1 results in: PC can't move, menu can be opened
    Feature #2996: Editor: make it possible to preset the height of the script check area in a script view
    Feature #3014: Editor: Tooltips in 3D scene
    Feature #3064: Werewolf field of view
    Feature #3074: Quicksave indicator
    Task #287: const version of Ptr
    Task #2542: Editor: redo user settings system

0.37.0
------

    Bug #385: Light emitting objects have a too short distance of activation
    Bug #455: Animation doesn't resize creature's bounding box
    Bug #602: Only collision model is updated when modifying objects trough console
    Bug #639: Sky horizon at nighttime
    Bug #672: incorrect trajectory of the moons
    Bug #814: incorrect NPC width
    Bug #827: Inaccurate raycasting for dead actors
    Bug #996: Can see underwater clearly when at right height/angle
    Bug #1317: Erene Llenim in Seyda Neen does not walk around
    Bug #1330: Cliff racers fail to hit the player
    Bug #1366: Combat AI can't aim down (in order to hit small creatures)
    Bug #1511: View distance while under water is much too short
    Bug #1563: Terrain positioned incorrectly and appears to vibrate in far-out cells
    Bug #1612: First person models clip through walls
    Bug #1647: Crash switching from full screen to windows mode - D3D9
    Bug #1650: No textures with directx on windows
    Bug #1730: Scripts names starting with digit(s) fail to compile
    Bug #1738: Socucius Ergalla's greetings are doubled during the tutorial
    Bug #1784: First person weapons always in the same position
    Bug #1813: Underwater flora lighting up entire area.
    Bug #1871: Handle controller extrapolation flags
    Bug #1921: Footstep frequency and velocity do not immediately update when speed attribute changes
    Bug #2001: OpenMW crashes on start with OpenGL 1.4 drivers
    Bug #2014: Antialiasing setting does nothing on Linux
    Bug #2037: Some enemies attack the air when spotting the player
    Bug #2052: NIF rotation matrices including scales are not supported
    Bug #2062: Crank in Old Mournhold: Forgotten Sewer turns about the wrong axis
    Bug #2111: Raindrops in front of fire look wrong
    Bug #2140: [OpenGL] Water effects, flames and parts of creatures solid black when observed through brazier flame
    Bug #2147: Trueflame and Hopesfire flame effects not properly aligned with blade
    Bug #2148: Verminous fabricants have little coloured box beneath their feet
    Bug #2149: Sparks in Clockwork City should bounce off the floor
    Bug #2151: Clockwork City dicer trap doesn't activate when you're too close
    Bug #2186: Mini map contains scrambled pixels that cause the mini map to flicker
    Bug #2187: NIF file with more than 255 NiBillboardNodes does not load
    Bug #2191: Editor: Crash when trying to view cell in render view in OpenCS
    Bug #2270: Objects flicker transparently
    Bug #2280: Latest 32bit windows build of openmw runns out of vram
    Bug #2281: NPCs don't scream when they die
    Bug #2286: Jumping animation restarts when equipping mid-air
    Bug #2287: Weapon idle animation stops when turning
    Bug #2355: Light spell doesn't work in 1st person view
    Bug #2362: Lantern glas opaque to flame effect from certain viewing angles
    Bug #2364: Light spells are not as bright as in Morrowind
    Bug #2383: Remove the alpha testing override list
    Bug #2436: Crash on entering cell "Tower of Tel Fyr, Hall of Fyr"
    Bug #2457: Player followers should not report crimes
    Bug #2458: crash in some fighting situations
    Bug #2464: Hiding an emitter node should make that emitter stop firing particles
    Bug #2466: Can't load a save created with OpenMW-0.35.0-win64
    Bug #2468: music from title screen continues after loading savegame
    Bug #2494: Map not consistent between saves
    Bug #2504: Dialog scroll should always start at the top
    Bug #2506: Editor: Undo/Redo shortcuts do not work in script editor
    Bug #2513: Mannequins in mods appear as dead bodies
    Bug #2524: Editor: TopicInfo "custom" condition section is missing
    Bug #2540: Editor: search and verification result table can not be sorted by clicking on the column names
    Bug #2543: Editor: there is a problem with spell effects
    Bug #2544: Editor fails to save NPC information correctly.
    Bug #2545: Editor: delete record in Objects (referenceables) table messes up data
    Bug #2546: Editor: race base attributes and skill boni are not displayed, thus not editable
    Bug #2547: Editor: some NPC data is not displayed, thus not editable
    Bug #2551: Editor: missing data in cell definition
    Bug #2553: Editor: value filter does not work for float values
    Bug #2555: Editor: undo leaves the record status as Modified
    Bug #2559: Make Detect Enchantment marks appear on top of the player arrow
    Bug #2563: position consoling npc doesn't work without cell reload
    Bug #2564: Editor: Closing a subview from code does not clean up properly and will lead to crash on opening the next subview
    Bug #2568: Editor: Setting default window size is ignored
    Bug #2569: Editor: saving from an esp to omwaddon file results in data loss for TopicInfo
    Bug #2575: Editor: Deleted record (with Added (ModifiedOnly) status) remains in the Dialog SubView
    Bug #2576: Editor: Editor doesn't scroll to a newly opened subview, when ScrollBar Only mode is active
    Bug #2578: Editor: changing Level or Reputation of an NPC crashes the editor
    Bug #2579: Editor: filters not updated when adding or cloning records
    Bug #2580: Editor: omwaddon makes OpenMW crash
    Bug #2581: Editor: focus problems in edit subviews single- and multiline input fields
    Bug #2582: Editor: object verifier should check for non-existing scripts being referenced
    Bug #2583: Editor: applying filter to TopicInfo on mods that have added dialouge makes the Editor crash
    Bug #2586: Editor: some dialogue only editable items do not refresh after undo
    Bug #2588: Editor: Cancel button exits program
    Bug #2589: Editor: Regions table - mapcolor does not change correctly
    Bug #2591: Placeatme - spurious 5th parameter raises error
    Bug #2593: COC command prints multiple times when GUI is hidden
    Bug #2598: Editor: scene view of instances has to be zoomed out to displaying something - center camera instance please
    Bug #2607: water behind an invisible NPC becomes invisible as well
    Bug #2611: Editor: Sort problem in Objects table when few nested rows are added
    Bug #2621: crash when a creature has no model
    Bug #2624: Editor: missing columns in tables
    Bug #2627: Character sheet doesn't properly update when backing out of CharGen
    Bug #2642: Editor: endif without if - is not reported as error when "verify" was executed
    Bug #2644: Editor: rebuild the list of available content files when opening the open/new dialogues
    Bug #2656: OpenMW & OpenMW-CS: setting "Flies" flag for ghosts has no effect
    Bug #2659: OpenMW & OpenMW-CS: savegame load fail due to script attached to NPCs
    Bug #2668: Editor: reputation value in the input field is not stored
    Bug #2696: Horkers use land idle animations under water
    Bug #2705: Editor: Sort by Record Type (Objects table) is incorrect
    Bug #2711: Map notes on an exterior cell that shows up with a map marker on the world map do not show up in the tooltip for that cell's marker on the world map
    Bug #2714: Editor: Can't reorder rows with the same topic in different letter case
    Bug #2720: Head tracking for creatures not implemented
    Bug #2722: Alchemy should only include effects shared by at least 2 ingredients
    Bug #2723: "ori" console command is not working
    Bug #2726: Ashlanders in front of Ghostgate start wandering around
    Bug #2727: ESM writer does not handle encoding when saving the TES3 header
    Bug #2728: Editor: Incorrect position of an added row in Info tables
    Bug #2731: Editor: Deleting a record triggers a Qt warning
    Bug #2733: Editor: Undo doesn't restore the Modified status of a record when a nested data is changed
    Bug #2734: Editor: The Search doesn't work
    Bug #2738: Additive moon blending
    Bug #2746: NIF node names should be case insensitive
    Bug #2752: Fog depth/density not handled correctly
    Bug #2753: Editor: line edit in dialogue subview tables shows after a single click
    Bug #2755: Combat AI changes target too frequently
    Bug #2761: Can't attack during block animations
    Bug #2764: Player doesn't raise arm in 3rd person for weathertype 9
    Bug #2768: Current screen resolution not selected in options when starting OpenMW
    Bug #2773: Editor: Deleted scripts are editable
    Bug #2776: ordinators still think I'm wearing their helm even though Khajiit and argonians can't
    Bug #2779: Slider bars continue to move if you don't release mouse button
    Bug #2781: sleep interruption is a little off (is this an added feature?)
    Bug #2782: erroneously able to ready weapon/magic (+sheathe weapon/magic) while paralyzed
    Bug #2785: Editor: Incorrect GMSTs for newly created omwgame files
    Bug #2786: Kwama Queen head is inverted under OpenMW
    Bug #2788: additem and removeitem incorrect gold behavior
    Bug #2790: --start doesn't trace down
    Bug #2791: Editor: Listed attributes and skill should not be based on number of NPC objects.
    Bug #2792: glitched merchantile/infinite free items
    Bug #2794: Need to ignore quotes in names of script function
    Bug #2797: Editor: Crash when removing the first row in a nested table
    Bug #2800: Show an error message when S3TC support is missing
    Bug #2811: Targetted Open spell effect persists.
    Bug #2819: Editor: bodypart's race filter not displayed correctly
    Bug #2820: Editor: table sorting is inverted
    Bug #2821: Editor: undo/redo command labels are incorrect
    Bug #2826: locking beds that have been locked via magic psuedo-freezes the game
    Bug #2830: Script compiler does not accept IDs as instruction/functions arguments if the ID is also a keyword
    Bug #2832: Cell names are not localized on the world map
    Bug #2833: [cosmetic] Players swimming at water's surface are slightly too low.
    Bug #2840: Save/load menu is not entirely localized
    Bug #2853: [exploit/bug] disintegrate weapon incorrectly applying to lockpicks, probes. creates unbreakable lockpicks
    Bug #2855: Mouse wheel in journal is not disabled by "Options" panel.
    Bug #2856: Heart of Lorkhan doesn't visually respond to attacks
    Bug #2863: Inventory highlights wrong category after load
    Bug #2864: Illuminated Order 1.0c Bug – The teleport amulet is not placed in the PC inventory.
    Bug #2866: Editor: use checkbox instead of combobox for boolean values
    Bug #2875: special cases of fSleepRandMod not behaving properly.
    Bug #2878: Editor: Verify reports "creature has non-positive level" but there is no level setting
    Bug #2879: Editor: entered value of field "Buys *" is not saved for a creature
    Bug #2880: OpenMW & OpenMW-CS: having a scale value of 0.000 makes the game laggy
    Bug #2882: Freeze when entering cell "Guild of Fighters (Ald'ruhn)" after dropping some items inside
    Bug #2883: game not playable if mod providing a spell is removed but the list of known spells still contains it
    Bug #2884: NPC chats about wrong player race
    Bug #2886: Adding custom races breaks existing numbering of PcRace
    Bug #2888: Editor: value entered in "AI Wander Idle" is not kept
    Bug #2889: Editor: creatures made with the CS (not cloned) are always dead
    Bug #2890: Editor: can't make NPC say a specific "Hello" voice-dialouge
    Bug #2893: Editor: making a creature use textual dialogue doesn't work.
    Bug #2901: Editor: gold for trading can not be set for creatures
    Bug #2907: looking from uderwater part of the PC that is below the surface looks like it would be above the water
    Bug #2914: Magicka not recalculated on character generation
    Bug #2915: When paralyzed, you can still enter and exit sneak
    Bug #2917: chameleon does not work for creatures
    Bug #2927: Editor: in the automatic script checker local variable caches are not invalidated/updated on modifications of other scripts
    Bug #2930: Editor: AIWander Idle can not be set for a creature
    Bug #2932: Editor: you can add rows to "Creature Attack" but you can not enter values
    Bug #2938: Editor: Can't add a start script.
    Bug #2944: Spell chance for power to show as 0 on hud when used
    Bug #2953: Editor: rightclick in an empty place in the menu bar shows an unnamed checkbox
    Bug #2956: Editor: freezes while editing Filter
    Bug #2959: space character in field enchantment (of an amulet) prevents rendering of surroundings
    Bug #2962: OpenMW: Assertion `it != invStore.end()' failed
    Bug #2964: Recursive script execution can corrupt script runtime data
    Bug #2973: Editor: placing a chest in the game world and activating it heavily blurrs the character portrait
    Bug #2978: Editor: Cannot edit alchemy ingredient properties
    Bug #2980: Editor: Attribute and Skill can be selected for spells that do not require these parameters, leading to non-functional spells
    Bug #2990: Compiling a script with warning mode 2 and enabled error downgrading leads to infinite recursion
    Bug #2992: [Mod: Great House Dagoth] Killing Dagoth Gares freezes the game
    Bug #3007: PlaceItem takes radians instead of degrees + angle reliability
    Feature #706: Editor: Script Editor enhancements
    Feature #872: Editor: Colour values in tables
    Feature #880: Editor: ID auto-complete
    Feature #928: Editor: Partial sorting in info tables
    Feature #942: Editor: Dialogue for editing/viewing content file meta information
    Feature #1057: NiStencilProperty
    Feature #1278: Editor: Mouse picking in worldspace widget
    Feature #1280: Editor: Cell border arrows
    Feature #1401: Editor: Cloning enhancements
    Feature #1463: Editor: Fine grained configuration of extended revert/delete commands
    Feature #1591: Editor: Make fields in creation bar drop targets where applicable
    Feature #1998: Editor: Magic effect record verifier
    Feature #1999: Editor Sound Gen record verifier
    Feature #2000: Editor: Pathgrid record verifier
    Feature #2528: Game Time Tracker
    Feature #2534: Editor: global search does not auomatically focus the search input field
    Feature #2535: OpenMW: allow comments in openmw.cfg
    Feature #2541: Editor: provide a go to the very bottom button for TopicInfo and JournalInfo
    Feature #2549: Editor: add a horizontal slider to scroll between opened tables
    Feature #2558: Editor: provide a shortcut for closing the subview that has the focus
    Feature #2565: Editor: add context menu for dialogue sub view fields with an item matching "Edit 'x'" from the table subview context menu
    Feature #2585: Editor: Ignore mouse wheel input for numeric values unless the respective widget has the focus
    Feature #2620: Editor: make the verify-view refreshable
    Feature #2622: Editor: Make double click behaviour in result tables configurable (see ID tables)
    Feature #2717: Editor: Add severity column to report tables
    Feature #2729: Editor: Various dialogue button bar improvements
    Feature #2739: Profiling overlay
    Feature #2740: Resource manager optimizations
    Feature #2741: Make NIF files into proper resources
    Feature #2742: Use the skinning data in NIF files as-is
    Feature #2743: Small feature culling
    Feature #2744: Configurable near clip distance
    Feature #2745: GUI scaling option
    Feature #2747: Support anonymous textures
    Feature #2749: Loading screen optimizations
    Feature #2751: Character preview optimization
    Feature #2804: Editor: Merge Tool
    Feature #2818: Editor: allow copying a record ID to the clipboard
    Feature #2946: Editor: add script line number in results of search
    Feature #2963: Editor: Mouse button bindings in 3D scene
    Feature #2983: Sun Glare fader
    Feature #2999: Scaling of journal and books
    Task #2665: Support building with Qt5
    Task #2725: Editor: Remove Display_YesNo
    Task #2730: Replace hardcoded column numbers in SimpleDialogueSubView/DialogueSubView
    Task #2750: Bullet shape instancing optimization
    Task #2793: Replace grid size setting with half grid size setting
    Task #3003: Support FFMPEG 2.9 (Debian request)

0.36.1
------

    Bug #2590: Start scripts not added correctly

0.36.0
------

    Bug #923: Editor: Operations-Multithreading is broken
    Bug #1317: Erene Llenim in Seyda Neen does not walk around
    Bug #1405: Water rendering glitch near Seyda Neen lighthouse
    Bug #1621: "Error Detecting Morrowind Installation" in the default directory
    Bug #2216: Creating a clone of the player stops you moving.
    Bug #2387: Casting bound weapon spell doesn't switch to "ready weapon" mode
    Bug #2407: Default to (0, 0) when "unknown cell" is encountered.
    Bug #2411: enchanted item charges don't update/refresh if spell list window is pinned open
    Bug #2428: Editor: cloning / creating new container class results in invalid omwaddon file - openmw-0.35
    Bug #2429: Editor - cloning omits some values or sets different values than the original has
    Bug #2430: NPC with negative fatigue don't fall (LGNPC Vivec, Foreign Quarter v2.21)
    Bug #2432: Error on startup with Uvirith's Legacy enabled
    Bug #2435: Editor: changed entries in the objects window are not shown as such
    Bug #2437: Editor: changing an entry of a container/NPC/clothing/ingredient/globals will not be saved in the omwaddon file
    Bug #2447: Editor doesn't save terrain information
    Bug #2451: Editor not listing files with accented characters
    Bug #2453: Chargen: sex, race and hair sliders not initialized properly
    Bug #2459: Minor terrain clipping through statics due to difference in triangle alignment
    Bug #2461: Invisible sound mark has collision in Sandus Ancestral Tomb
    Bug #2465: tainted gold stack
    Bug #2475: cumulative stacks of 100 point fortify skill speechcraft boosts do not apply correctly
    Bug #2498: Editor: crash when issuing undo command after the table subview is closed
    Bug #2500: Editor: object table - can't undo delete record
    Bug #2518: OpenMW detect spell returns false positives
    Bug #2521: NPCs don't react to stealing when inventory menu is open.
    Bug #2525: Can't click on red dialogue choice [rise of house telvanni][60fffec]
    Bug #2530: GetSpellEffects not working as in vanilla
    Bug #2557: Crash on first launch after choosing "Run installation wizard"
    Feature #139: Editor: Global Search & Replace
    Feature #1219: Editor: Add dialogue mode only columns
    Feature #2024: Hotkey for hand to hand (i.e. unequip any weapon)
    Feature #2119: "Always Sneak" key bind
    Feature #2262: Editor: Handle moved instances
    Feature #2425: Editor: Add start script table
    Feature #2426: Editor: start script record verifier
    Feature #2480: Launcher: Multiselect entries in the Data Files list
    Feature #2505: Editor: optionally show a line number column in the script editor
    Feature #2512: Editor: Offer use of monospace fonts in the script editor as an option
    Feature #2514: Editor: focus on ID input field on clone/add
    Feature #2519: it is not possible to change icons that appear on the map after casting the Detect <animal | enchantment | key> spells
    Task #2460: OS X: Use Application Support directory as user data path
    Task #2516: Editor: Change References / Referenceables terminology

0.35.1
------

    Bug #781: incorrect trajectory of the sun
    Bug #1079: Wrong starting position in "Character Stuff Wonderland"
    Bug #1443: Repetitive taking of a stolen object is repetitively considered as a crime
    Bug #1533: Divine Intervention goes to the wrong place.
    Bug #1714: No visual indicator for time passed during training
    Bug #1916: Telekinesis does not allow safe opening of traps
    Bug #2227: Editor: addon file name inconsistency
    Bug #2271: Player can melee enemies from water with impunity
    Bug #2275: Objects with bigger scale move further using Move script
    Bug #2285: Aryon's Dominator enchantment does not work properly
    Bug #2290: No punishment for stealing gold from owned containers
    Bug #2328: Launcher does not respond to Ctrl+C
    Bug #2334: Drag-and-drop on a content file in the launcher creates duplicate items
    Bug #2338: Arrows reclaimed from corpses do not stack sometimes
    Bug #2344: Launcher - Settings importer running correctly?
    Bug #2346: Launcher - Importing plugins into content list screws up the load order
    Bug #2348: Mod: H.E.L.L.U.V.A. Handy Holdables does not appear in the content list
    Bug #2353: Detect Animal detects dead creatures
    Bug #2354: Cmake does not respect LIB_SUFFIX
    Bug #2356: Active magic set inactive when switching magic items
    Bug #2361: ERROR: ESM Error: Previous record contains unread bytes
    Bug #2382: Switching spells with "next spell" or "previous spell" while holding shift promps delete spell dialog
    Bug #2388: Regression: Can't toggle map on/off
    Bug #2392: MOD Shrines - Restore Health and Cancel Options adds 100 health points
    Bug #2394: List of Data Files tab in openmw-laucher needs to show all content files.
    Bug #2402: Editor: skills saved incorrectly
    Bug #2408: Equipping a constant effect Restore Health/Magicka/Fatigue item will permanently boost the stat it's restoring
    Bug #2415: It is now possible to fall off the prison ship into the water when starting a new game
    Bug #2419: MOD MCA crash to desktop
    Bug #2420: Game crashes when character enters a certain area
    Bug #2421: infinite loop when using cycle weapon without having a weapon
    Feature #2221: Cannot dress dead NPCs
    Feature #2349: Check CMake sets correct MSVC compiler settings for release build.
    Feature #2397: Set default values for global mandatory records.
    Feature #2412: Basic joystick support

0.35.0
------

    Bug #244: Clipping/static in relation to the ghostgate/fence sound.
    Bug #531: Missing transparent menu items
    Bug #811: Content Lists in openmw.cfg are overwritten
    Bug #925: OpenCS doesn't launch because it thinks its already started
    Bug #969: Water shader strange behaviour on AMD card
    Bug #1049: Partially highlighted word in dialogue may cause incorrect line break
    Bug #1069: omwlauncher.exe crashes due to file lock
    Bug #1192: It is possible to jump on top of hostile creatures in combat
    Bug #1342: Loud ambient sounds
    Bug #1431: Creatures can climb the player
    Bug #1605: Guard in CharGen doesn't turn around to face you when reaching stairs
    Bug #1624: Moon edges don't transition properly
    Bug #1634: Items dropped by PC have collision
    Bug #1637: Weird NPC behaviour in Vivec, Hlaalu Ancestral Vaults?
    Bug #1638: Cannot climb staircases
    Bug #1648: Enchanted equipment badly handled at game reload
    Bug #1663: Crash when casting spell at enemy near you
    Bug #1683: Scale doesn't apply to animated collision nodes
    Bug #1702: Active enchanted item forgotten
    Bug #1730: Scripts names starting with digit(s) fail to compile
    Bug #1743: Moons are transparent
    Bug #1745: Shadows crash: Assertion `mEffects.empty()' failed.
    Bug #1785: Can't equip two-handed weapon and shield
    Bug #1809: Player falls too easily
    Bug #1825: Sword of Perithia can´t run in OpenMW
    Bug #1899: The launcher resets any alterations you´ve made in the mod list order,
    Bug #1964: Idle voices/dialogs not triggered correctly
    Bug #1980: Please, change default click behavior in OpenMW Launchers Data Files list
    Bug #1984: Vampire corpses standing up when looting the first item
    Bug #1985: Calm spell does nothing
    Bug #1986: Spell name lights up on mouseover but spell cost does not
    Bug #1989: Tooltip still shown when menu toggled off
    Bug #2010: Raindrops Displayed While Underwater
    Bug #2023: Walking into plants causes massive framedrop
    Bug #2031: [MOD: Shrines - Restore Health and Cancel Options]: Restore health option doesn't work
    Bug #2039: Lake Fjalding pillar of fire not rendered
    Bug #2040: AI_follow should stop further from the target
    Bug #2076: Slaughterfish AI
    Bug #2077: Direction of long jump can be changed much more than it is possible in vanilla
    Bug #2078: error during rendering: Object '' not found (const)
    Bug #2105: Lockpicking causes screen sync glitch
    Bug #2113: [MOD: Julan Ashlander Companion] Julan does not act correctly within the Ghostfence.
    Bug #2123: Window glow mod: Collision issues
    Bug #2133: Missing collision for bridges in Balmora when using Morrowind Rebirth 2.81
    Bug #2135: Casting a summon spell while the summon is active does not reset the summon.
    Bug #2144: Changing equipment will unequip drawn arrows/bolts
    Bug #2169: Yellow on faces when using opengl renderer and mods from overhaul on windows
    Bug #2175: Pathgrid mods do not overwrite the existing pathgrid
    Bug #2176: Morrowind -Russian localization end add-on ChaosHeart. Error in framelistener;object ;frenzying toush; not found <const>
    Bug #2181: Mod Morrowind crafting merchants die.
    Bug #2182: mods changing skill progression double the bonus for class specialization
    Bug #2183: Editor: Skills "use value" only allows integer between 0 and 99
    Bug #2184: Animated Morrowind Expanded produces an error on Open MW Launch
    Bug #2185: Conditional Operator formats
    Bug #2193: Quest: Gateway Ghost
    Bug #2194: Cannot summon multiples of the same creature
    Bug #2195: Pathgrid in the (0,0) exterior cell not loaded
    Bug #2200: Outdoor NPCs can stray away and keep walking into a wall
    Bug #2201: Creatures do not receive fall damage
    Bug #2202: The enchantment the item can hold is calculated incorrectly
    Bug #2203: Having the mod Living Cities of Vvardenfall running causes the game world to fail to load after leaving the prison ship
    Bug #2204: Abot's Water Life - Book rendered incorrectly
    Bug #2205: sound_waterfall script no longer compiles
    Bug #2206: Dialogue script fails to compile (extra .)
    Bug #2207: Script using – instead of - character does not compile
    Bug #2208: Failing dialogue scripts in french Morrowind.esm
    Bug #2214: LGNPC Vivec Redoran 1.62 and The King Rat (Size and inventory Issues)
    Bug #2215: Beast races can use enchanted boots
    Bug #2218: Incorrect names body parts in 3D models for open helmet with skinning
    Bug #2219: Orcs in Ghorak Manor in Caldera don't attack if you pick their pockets.
    Bug #2220: Chargen race preview head incorrect orientation
    Bug #2223: Reseting rock falling animation
    Bug #2224: Fortify Attribute effects do not stack when Spellmaking.
    Bug #2226: OpenCS pseudo-crash
    Bug #2230: segfaulting when entering Ald'ruhn with a specific mod: "fermeture la nuit" (closed by night)
    Bug #2233: Area effect spells on touch do not have the area effect
    Bug #2234: Dwarven Crossbow clips through the ground when dropped
    Bug #2235: class SettingsBase<> reverses the order of entries with multiple keys.
    Bug #2236: Weird two handed longsword + torch interaction
    Bug #2237: Shooting arrows while sneaking do not agro
    Bug #2238: Bipedal creatures not using weapons are not handled properly
    Bug #2245: Incorrect topic highlighting in HT_SpyBaladas quest
    Bug #2252: Tab completion incomplete for places using COC from the console.
    Bug #2255: Camera reverts to first person on load
    Bug #2259: enhancement: the save/load progress bar is not very progressive
    Bug #2263: TogglePOV can not be bound to Alt key
    Bug #2267: dialogue disabling via mod
    Bug #2268: Highlighting Files with load order problems in Data Files tab of Launcher
    Bug #2276: [Mod]ShotN issues with Karthwasten
    Bug #2283: Count argument for PlaceAt functions not working
    Bug #2284: Local map notes should be visible on door marker leading to the cell with the note
    Bug #2293: There is a graphical glitch at the end of the spell's animation in 3rd Person (looking over the shoulder) view
    Bug #2294: When using Skyrim UI Overhaul, the tops of pinnable menus are invisible
    Bug #2302: Random leveled items repeat way too often in a single dungeon
    Bug #2306: Enchanted arrows should not be retrievable from corpses
    Bug #2308: No sound effect when drawing the next throwing knife
    Bug #2309: Guards chase see the player character even if they're invisible
    Bug #2319: Inverted controls and other issues after becoming a vampire
    Bug #2324: Spells cast when crossing cell border are imprinted on the local map
    Bug #2330: Actors with Drain Health effect retain health after dying
    Bug #2331: tgm (god mode) won't allow the player to cast spells if the player doesn't have enough mana
    Bug #2332: Error in framelistener: Need a skeleton to attach the arrow to
    Feature #114: ess-Importer
    Feature #504: Editor: Delete selected rows from result windows
    Feature #1024: Addition of remaining equipping hotkeys
    Feature #1067: Handle NIF interpolation type 4 (XYZ_ROTATION_KEY)
    Feature #1125: AI fast-forward
    Feature #1228: Drowning while knocked out
    Feature #1325: Editor: Opening window and User Settings window cleanup
    Feature #1537: Ability to change the grid size from 3x3 to 5x5 (or more with good pc)
    Feature #1546: Leveled list script functions
    Feature #1659: Test dialogue scripts in --script-all
    Feature #1720: NPC lookAt controller
    Feature #2178: Load initial particle system state from NIF files
    Feature #2197: Editor: When clicking on a script error in the report window set cursor in script editor to the respective line/column
    Feature #2261: Warn when loading save games with mod mismatch
    Feature #2313: ess-Importer: convert global map exploration overlay
    Feature #2318: Add commandline option to load a save game
    Task #810: Rename "profile" to "content list"
    Task #2196: Label local/global openmw.cfg files via comments

0.34.0
------

    Bug #904: omwlauncher doesn't allow installing Tribunal and Bloodmoon if only MW is installed
    Bug #986: Launcher: renaming profile names is broken
    Bug #1061: "Browse to CD..." launcher crash
    Bug #1135: Launcher crashes if user does not have write permission
    Bug #1231: Current installer in launcher does not correctly import russian Morrowind.ini settings from setup.inx
    Bug #1288: Fix the Alignment of the Resolution Combobox
    Bug #1343: BIK videos occasionally out of sync with audio
    Bug #1684: Morrowind Grass Mod graphical glitches
    Bug #1734: NPC in fight with invisible/sneaking player
    Bug #1982: Long class names are cut off in the UI
    Bug #2012: Editor: OpenCS script compiler sometimes fails to find IDs
    Bug #2015: Running while levitating does not affect speed but still drains fatigue
    Bug #2018: OpenMW don´t reset modified cells to vanilla when a plugin is deselected and don´t apply changes to cells already visited.
    Bug #2045: ToggleMenus command should close dialogue windows
    Bug #2046: Crash: light_de_streetlight_01_223
    Bug #2047: Buglamp tooltip minor correction
    Bug #2050: Roobrush floating texture bits
    Bug #2053: Slaves react negatively to PC picking up slave's bracers
    Bug #2055: Dremora corpses use the wrong model
    Bug #2056: Mansilamat Vabdas's corpse is floating in the water
    Bug #2057: "Quest: Larius Varro Tells A Little Story": Bounty not completely removed after finishing quest
    Bug #2059: Silenced enemies try to cast spells anyway
    Bug #2060: Editor: Special case implementation for top level window with single sub-window should be optional
    Bug #2061: Editor: SubView closing that is not directly triggered by the user isn't handled properly
    Bug #2063: Tribunal: Quest 'The Warlords' doesn't work
    Bug #2064: Sneak attack on hostiles causes bounty
    Bug #2065: Editor: Qt signal-slot error when closing a dialogue subview
    Bug #2070: Loading ESP in OpenMW works but fails in OpenCS
    Bug #2071: CTD in 0.33
    Bug #2073: Storm atronach animation stops now and then
    Bug #2075: Molag Amur Region, Map shows water on solid ground
    Bug #2080: game won't work with fair magicka regen
    Bug #2082: NPCs appear frozen or switched off after leaving and quickly reentering a cell
    Bug #2088: OpenMW is unable to play OGG files.
    Bug #2093: Darth Gares talks to you in Ilunibi even when he's not there, screwing up the Main Quests
    Bug #2095: Coordinate and rotation editing in the Reference table does not work.
    Bug #2096: Some overflow fun and bartering exploit
    Bug #2098: [D3D] Game crash on maximize
    Bug #2099: Activate, player seems not to work
    Bug #2104: Only labels are sensitive in buttons
    Bug #2107: "Slowfall" effect is too weak
    Bug #2114: OpenCS doesn't load an ESP file full of errors even though Vanilla MW Construction Set can
    Bug #2117: Crash when encountering bandits on opposite side of river from the egg mine south of Balmora
    Bug #2124: [Mod: Baldurians Transparent Glass Amor] Armor above head
    Bug #2125: Unnamed NiNodes in weapons problem in First Person
    Bug #2126: Dirty dialog script in tribunal.esm causing bug in Tribunal MQ
    Bug #2128: Crash when picking character's face
    Bug #2129: Disable the third-person zoom feature by default
    Bug #2130: Ash storm particles shown too long during transition to clear sky
    Bug #2137: Editor: exception caused by following the Creature column of a SoundGen record
    Bug #2139: Mouse movement should be ignored during intro video
    Bug #2143: Editor: Saving is broken
    Bug #2145: OpenMW - crash while exiting x64 debug build
    Bug #2152: You can attack Almalexia during her final monologue
    Bug #2154: Visual effects behave weirdly after loading/taking a screenshot
    Bug #2155: Vivec has too little magicka
    Bug #2156: Azura's spirit fades away too fast
    Bug #2158: [Mod]Julan Ashlander Companion 2.0: Negative magicka
    Bug #2161: Editor: combat/magic/stealth values of creature not displayed correctly
    Bug #2163: OpenMW can't detect death if the NPC die by the post damage effect of a magic weapon.
    Bug #2168: Westly's Master Head Pack X – Some hairs aren't rendered correctly.
    Bug #2170: Mods using conversations to update PC inconsistant
    Bug #2180: Editor: Verifier doesn't handle Windows-specific path issues when dealing with resources
    Bug #2212: Crash or unexpected behavior while closing OpenCS cell render window on OS X
    Feature #238: Add UI to run INI-importer from the launcher
    Feature #854: Editor: Add user setting to show status bar
    Feature #987: Launcher: first launch instructions for CD need to be more explicit
    Feature #1232: There is no way to set the "encoding" option using launcher UI.
    Feature #1281: Editor: Render cell markers
    Feature #1918: Editor: Functionality for Double-Clicking in Tables
    Feature #1966: Editor: User Settings dialogue grouping/labelling/tooltips
    Feature #2097: Editor: Edit position of references in 3D scene
    Feature #2121: Editor: Add edit mode button to scene toolbar
    Task #1965: Editor: Improve layout of user settings dialogue

0.33.1
------

    Bug #2108: OpenCS fails to build

0.33.0
------

    Bug #371: If console assigned to ` (probably to any symbolic key), "`" symbol will be added to console every time it closed
    Bug #1148: Some books'/scrolls' contents are displayed incorrectly
    Bug #1290: Editor: status bar is not updated when record filter is changed
    Bug #1292: Editor: Documents are not removed on closing the last view
    Bug #1301: Editor: File->Exit only checks the document it was issued from.
    Bug #1353: Bluetooth on with no speaker connected results in significantly longer initial load times
    Bug #1436: NPCs react from too far distance
    Bug #1472: PC is placed on top of following NPC when changing cell
    Bug #1487: Tall PC can get stuck in staircases
    Bug #1565: Editor: Subviews are deleted on shutdown instead when they are closed
    Bug #1623: Door marker on Ghorak Manor's balcony makes PC stuck
    Bug #1633: Loaddoor to Sadrith Mora, Telvanni Council House spawns PC in the air
    Bug #1655: Use Appropriate Application Icons on Windows
    Bug #1679: Tribunal expansion, Meryn Othralas the backstage manager in the theatre group in Mournhold in the great bazaar district is floating a good feet above the ground.
    Bug #1705: Rain is broken in third person
    Bug #1706: Thunder and lighting still occurs while the game is paused during the rain
    Bug #1708: No long jumping
    Bug #1710: Editor: ReferenceableID drag to references record filter field creates incorrect filter
    Bug #1712: Rest on Water
    Bug #1715: "Cancel" button is not always on the same side of menu
    Bug #1725: Editor: content file can be opened multiple times from the same dialogue
    Bug #1730: [MOD: Less Generic Nerevarine] Compile failure attempting to enter the Corprusarium.
    Bug #1733: Unhandled ffmpeg sample formats
    Bug #1735: Editor: "Edit Record" context menu button not opening subview for journal infos
    Bug #1750: Editor: record edits result in duplicate entries
    Bug #1789: Editor: Some characters cannot be used in addon name
    Bug #1803: Resizing the map does not keep the pre-resize center at the post-resize center
    Bug #1821: Recovering Cloudcleaver quest: attacking Sosia is considered a crime when you side with Hlormar
    Bug #1838: Editor: Preferences window appears off screen
    Bug #1839: Editor: Record filter title should be moved two pixels to the right
    Bug #1849: Subrecord error in MAO_Containers
    Bug #1854: Knocked-out actors don't fully act knocked out
    Bug #1855: "Soul trapped" sound doesn't play
    Bug #1857: Missing sound effect for enchanted items with empty charge
    Bug #1859: Missing console command: ResetActors (RA)
    Bug #1861: Vendor category "MagicItems" is unhandled
    Bug #1862: Launcher doesn't start if a file listed in launcher.cfg has correct name but wrong capitalization
    Bug #1864: Editor: Region field for cell record in dialogue subview not working
    Bug #1869: Editor: Change label "Musics" to "Music"
    Bug #1870: Goblins killed while knocked down remain in knockdown-pose
    Bug #1874: CellChanged events should not trigger when crossing exterior cell border
    Bug #1877: Spriggans killed instantly if hit while regening
    Bug #1878: Magic Menu text not un-highlighting correctly when going from spell to item as active magic
    Bug #1881: Stuck in ceiling when entering castle karstaags tower
    Bug #1884: Unlit torches still produce a burning sound
    Bug #1885: Can type text in price field in barter window
    Bug #1887: Equipped items do not emit sounds
    Bug #1889: draugr lord aesliip will attack you and remain non-hostile
    Bug #1892: Guard asks player to pay bounty of 0 gold
    Bug #1895: getdistance should only return max float if ref and target are in different worldspaces
    Bug #1896: Crash Report
    Bug #1897: Conjured Equipment cant be re-equipped if removed
    Bug #1898: Only Gidar Verothan follows you during establish the mine quest
    Bug #1900: Black screen when you open the door and breath underwater
    Bug #1904: Crash on casting recall spell
    Bug #1906: Bound item checks should use the GMSTs
    Bug #1907: Bugged door. Mournhold, The Winged Guar
    Bug #1908: Crime reported for attacking Drathas Nerus's henchmen while they attack Dilborn
    Bug #1909: Weird Quest Flow Infidelities quest
    Bug #1910: Follower fighting with gone npc
    Bug #1911: Npcs will drown themselves
    Bug #1912: World map arrow stays static when inside a building
    Bug #1920: Ulyne Henim disappears when game is loaded inside Vas
    Bug #1922: alchemy-> potion of paralyze
    Bug #1923: "levitation magic cannot be used here" shows outside of tribunal
    Bug #1927: AI prefer melee over magic.
    Bug #1929: Tamriel Rebuilt: Named cells that lie within the overlap with Morrowind.esm are not shown
    Bug #1932: BTB - Spells 14.1 magic effects don´t overwrite the Vanilla ones but are added
    Bug #1935: Stacks of items are worth more when sold individually
    Bug #1940: Launcher does not list addon files if base game file is renamed to a different case
    Bug #1946: Mod "Tel Nechim - moved" breaks savegames
    Bug #1947: Buying/Selling price doesn't properly affect the growth of mercantile skill
    Bug #1950: followers from east empire company quest will fight each other if combat happens with anything
    Bug #1958: Journal can be scrolled indefinitely with a mouse wheel
    Bug #1959: Follower not leaving party on quest end
    Bug #1960: Key bindings not always saved correctly
    Bug #1961: Spell merchants selling racial bonus spells
    Bug #1967: segmentation fault on load saves
    Bug #1968: Jump sounds are not controlled by footsteps slider, sound weird compared to footsteps
    Bug #1970: PC suffers silently when taking damage from lava
    Bug #1971: Dwarven Sceptre collision area is not removed after killing one
    Bug #1974: Dalin/Daris Norvayne follows player indefinitely
    Bug #1975: East Empire Company faction rank breaks during Raven Rock questline
    Bug #1979: 0 strength = permanently over encumbered
    Bug #1993: Shrine blessing in Maar Gan doesn't work
    Bug #2008: Enchanted items do not recharge
    Bug #2011: Editor: OpenCS script compiler doesn't handle member variable access properly
    Bug #2016: Dagoth Ur already dead in Facility Cavern
    Bug #2017: Fighters Guild Quest: The Code Book - dialogue loop when UMP is loaded.
    Bug #2019: Animation of 'Correct UV Mudcrabs' broken
    Bug #2022: Alchemy window - Removing ingredient doesn't remove the number of ingredients
    Bug #2025: Missing mouse-over text for non affordable items
    Bug #2028: [MOD: Tamriel Rebuilt] Crashing when trying to enter interior cell "Ruinous Keep, Great Hall"
    Bug #2029: Ienith Brothers Thiev's Guild quest journal entry not adding
    Feature #471: Editor: Special case implementation for top-level window with single sub-window
    Feature #472: Editor: Sub-Window re-use settings
    Feature #704: Font colors import from fallback settings
    Feature #879: Editor: Open sub-views in a new top-level window
    Feature #932: Editor: magic effect table
    Feature #937: Editor: Path Grid table
    Feature #938: Editor: Sound Gen table
    Feature #1117: Death and LevelUp music
    Feature #1226: Editor: Request UniversalId editing from table columns
    Feature #1545: Targeting console on player
    Feature #1597: Editor: Render terrain
    Feature #1695: Editor: add column for CellRef's global variable
    Feature #1696: Editor: use ESM::Cell's RefNum counter
    Feature #1697: Redden player's vision when hit
    Feature #1856: Spellcasting for non-biped creatures
    Feature #1879: Editor: Run OpenMW with the currently edited content list
    Task #1851: Move AI temporary state out of AI packages
    Task #1865: Replace char type in records

0.32.0
------

    Bug #1132: Unable to jump when facing a wall
    Bug #1341: Summoned Creatures do not immediately disappear when killed.
    Bug #1430: CharGen Revamped script does not compile
    Bug #1451: NPCs shouldn't equip weapons prior to fighting
    Bug #1461: Stopped start scripts do not restart on load
    Bug #1473: Dead NPC standing and in 2 pieces
    Bug #1482: Abilities are depleted when interrupted during casting
    Bug #1503: Behaviour of NPCs facing the player
    Bug #1506: Missing character, French edition: three-points
    Bug #1528: Inventory very slow after 2 hours
    Bug #1540: Extra arguments should be ignored for script functions
    Bug #1541: Helseth's Champion: Tribunal
    Bug #1570: Journal cannot be opened while in inventory screen
    Bug #1573: PC joins factions at random
    Bug #1576: NPCs aren't switching their weapons when out of ammo
    Bug #1579: Guards detect creatures in far distance, instead on sight
    Bug #1588: The Siege of the Skaal Village: bloodmoon
    Bug #1593: The script compiler isn't recognising some names that contain a -
    Bug #1606: Books: Question marks instead of quotation marks
    Bug #1608: Dead bodies prevent door from opening/closing.
    Bug #1609: Imperial guards in Sadrith Mora are not using their spears
    Bug #1610: The bounty number is not displayed properly with high numbers
    Bug #1620: Implement correct formula for auto-calculated NPC spells
    Bug #1630: Boats standing vertically in Vivec
    Bug #1635: Arrest dialogue is executed second time after I select "Go to jail"
    Bug #1637: Weird NPC behaviour in Vivec, Hlaalu Ancestral Vaults?
    Bug #1641: Persuasion dialog remains after loading, possibly resulting in crash
    Bug #1644: "Goodbye" and similar options on dialogues prevents escape working properly.
    Bug #1646: PC skill stats are not updated immediately when changing equipment
    Bug #1652: Non-aggressive creature
    Bug #1653: Quickloading while the container window is open crashes the game
    Bug #1654: Priority of checks in organic containers
    Bug #1656: Inventory items merge issue when repairing
    Bug #1657: Attacked state of NPCs is not saved properly
    Bug #1660: Rank dialogue condition ignored
    Bug #1668: Game starts on day 2 instead of day 1
    Bug #1669: Critical Strikes while fighting a target who is currently fighting me
    Bug #1672: OpenCS doesn't save the projects
    Bug #1673: Fatigue decreasing by only one point when running
    Bug #1675: Minimap and localmap graphic glitches
    Bug #1676: Pressing the OK button on the travel menu cancels the travel and exits the menu
    Bug #1677: Sleeping in a rented bed is considered a crime
    Bug #1685: NPCs turn towards player even if invisible/sneaking
    Bug #1686: UI bug: cursor is clicking "world/local" map button while inventory window is closed?
    Bug #1690: Double clicking on a inventory window header doesn't close it.
    Bug #1693: Spell Absorption does not absorb shrine blessings
    Bug #1694: journal displays learned topics as quests
    Bug #1700: Sideways scroll of text boxes
    Bug #1701: Player enchanting requires player hold money, always 100% sucessful.
    Bug #1704: self-made Fortify Intelligence/Drain willpower potions are broken
    Bug #1707: Pausing the game through the esc menu will silence rain, pausing it by opening the inventory will not.
    Bug #1709: Remesa Othril is hostile to Hlaalu members
    Bug #1713: Crash on load after death
    Bug #1719: Blind effect has slight border at the edge of the screen where it is ineffective.
    Bug #1722: Crash after creating enchanted item, reloading saved game
    Bug #1723: Content refs that are stacked share the same index after unstacking
    Bug #1726: Can't finish Aengoth the Jeweler's quest : Retrieve the Scrap Metal
    Bug #1727: Targets almost always resist soultrap scrolls
    Bug #1728: Casting a soultrap spell on invalid target yields no message
    Bug #1729: Chop attack doesn't work if walking diagonally
    Bug #1732: Error handling for missing script function arguments produces weird message
    Bug #1736: Alt-tabbing removes detail from overworld map.
    Bug #1737: Going through doors with (high magnitude?) leviation will put the player high up, possibly even out of bounds.
    Bug #1739: Setting a variable on an NPC from another NPC's dialogue result sets the wrong variable
    Bug #1741: The wait dialogue doesn't black the screen out properly during waiting.
    Bug #1742: ERROR: Object 'sDifficulty' not found (const)
    Bug #1744: Night sky in Skies V.IV (& possibly v3) by SWG rendered incorrectly
    Bug #1746: Bow/marksman weapon condition does not degrade with use
    Bug #1749: Constant Battle Music
    Bug #1752: Alt-Tabbing in the character menus makes the paper doll disappear temporarily
    Bug #1753: Cost of training is not added to merchant's inventory
    Bug #1755: Disposition changes do not persist if the conversation menu is closed by purchasing training.
    Bug #1756: Caught Blight after being cured of Corprus
    Bug #1758: Crash Upon Loading New Cell
    Bug #1760: Player's Magicka is not recalculated upon drained or boosted intelligence
    Bug #1761: Equiped torches lost on reload
    Bug #1762: Your spell did not get a target. Soul trap. Gorenea Andrano
    Bug #1763: Custom Spell Magicka Cost
    Bug #1765: Azuras Star breaks on recharging item
    Bug #1767: GetPCRank did not handle ignored explicit references
    Bug #1772: Dark Brotherhood Assassins never use their Carved Ebony Dart, sticking to their melee weapon.
    Bug #1774: String table overflow also occurs when loading TheGloryRoad.esm
    Bug #1776: dagoth uthol runs in slow motion
    Bug #1778: Incorrect values in spellmaking window
    Bug #1779: Icon of Master Propylon Index is not visible
    Bug #1783: Invisible NPC after looting corpse
    Bug #1787: Health Calculation
    Bug #1788: Skeletons, ghosts etc block doors when we try to open
    Bug #1791: [MOD: LGNPC Foreign Quarter] NPC in completely the wrong place.
    Bug #1792: Potions should show more effects
    Bug #1793: Encumbrance while bartering
    Bug #1794: Fortify attribute not affecting fatigue
    Bug #1795: Too much magicka
    Bug #1796: "Off by default" torch burning
    Bug #1797: Fish too slow
    Bug #1798: Rest until healed shouldn't show with full health and magicka
    Bug #1802: Mark location moved
    Bug #1804: stutter with recent builds
    Bug #1810: attack gothens dremora doesnt agro the others.
    Bug #1811: Regression: Crash Upon Loading New Cell
    Bug #1812: Mod: "QuickChar" weird button placement
    Bug #1815: Keys show value and weight, Vanilla Morrowind's keys dont.
    Bug #1817: Persuasion results do not show using unpatched MW ESM
    Bug #1818: Quest B3_ZainabBride moves to stage 47 upon loading save while Falura Llervu is following
    Bug #1823: AI response to theft incorrect - only guards react, in vanilla everyone does.
    Bug #1829: On-Target Spells Rendered Behind Water Surface Effects
    Bug #1830: Galsa Gindu's house is on fire
    Bug #1832: Fatal Error: OGRE Exception(2:InvalidParametersException)
    Bug #1836: Attacked Guards open "fine/jail/resist"-dialogue after killing you
    Bug #1840: Infinite recursion in ActionTeleport
    Bug #1843: Escorted people change into player's cell after completion of escort stage
    Bug #1845: Typing 'j' into 'Name' fields opens the journal
    Bug #1846: Text pasted into the console still appears twice (Windows)
    Bug #1847: "setfatigue 0" doesn't render NPC unconscious
    Bug #1848: I can talk to unconscious actors
    Bug #1866: Crash when player gets killed by a creature summoned by him
    Bug #1868: Memory leaking when openmw window is minimized
    Feature #47: Magic Effects
    Feature #642: Control NPC mouth movement using current Say sound
    Feature #939: Editor: Resources tables
    Feature #961: AI Combat for magic (spells, potions and enchanted items)
    Feature #1111: Collision script instructions (used e.g. by Lava)
    Feature #1120: Command creature/humanoid magic effects
    Feature #1121: Elemental shield magic effects
    Feature #1122: Light magic effect
    Feature #1139: AI: Friendly hits
    Feature #1141: AI: combat party
    Feature #1326: Editor: Add tooltips to all graphical buttons
    Feature #1489: Magic effect Get/Mod/Set functions
    Feature #1505: Difficulty slider
    Feature #1538: Targeted scripts
    Feature #1571: Allow creating custom markers on the local map
    Feature #1615: Determine local variables from compiled scripts instead of the values in the script record
    Feature #1616: Editor: Body part record verifier
    Feature #1651: Editor: Improved keyboard navigation for scene toolbar
    Feature #1666: Script blacklisting
    Feature #1711: Including the Git revision number from the command line "--version" switch.
    Feature #1721: NPC eye blinking
    Feature #1740: Scene toolbar buttons for selecting which type of elements are rendered
    Feature #1790: Mouse wheel scrolling for the journal
    Feature #1850: NiBSPArrayController
    Task #768: On windows, settings folder should be "OpenMW", not "openmw"
    Task #908: Share keyframe data
    Task #1716: Remove defunct option for building without FFmpeg

0.31.0
------

    Bug #245: Cloud direction and weather systems differ from Morrowind
    Bug #275: Local Map does not always show objects that span multiple cells
    Bug #538: Update CenterOnCell (COC) function behavior
    Bug #618: Local and World Map Textures are sometimes Black
    Bug #640: Water behaviour at night
    Bug #668: OpenMW doesn't support non-latin paths on Windows
    Bug #746: OpenMW doesn't check if the background music was already played
    Bug #747: Door is stuck if cell is left before animation finishes
    Bug #772: Disabled statics are visible on map
    Bug #829: OpenMW uses up all available vram, when playing for extended time
    Bug #869: Dead bodies don't collide with anything
    Bug #894: Various character creation issues
    Bug #897/#1369: opencs Segmentation Fault after "new" or "load"
    Bug #899: Various jumping issues
    Bug #952: Reflection effects are one frame delayed
    Bug #993: Able to interact with world during Wait/Rest dialog
    Bug #995: Dropped items can be placed inside the wall
    Bug #1008: Corpses always face up upon reentering the cell
    Bug #1035: Random colour patterns appearing in automap
    Bug #1037: Footstep volume issues
    Bug #1047: Creation of wrong links in dialogue window
    Bug #1129: Summoned creature time life duration seems infinite
    Bug #1134: Crimes can be committed against hostile NPCs
    Bug #1136: Creature run speed formula is incorrect
    Bug #1150: Weakness to Fire doesn't apply to Fire Damage in the same spell
    Bug #1155: NPCs killing each other
    Bug #1166: Bittercup script still does not work
    Bug #1178: .bsa file names are case sensitive.
    Bug #1179: Crash after trying to load game after being killed
    Bug #1180: Changing footstep sound location
    Bug #1196: Jumping not disabled when showing messageboxes
    Bug #1202: "strange" keys are not shown in binding menu, and are not saved either, but works
    Bug #1216: Broken dialog topics in russian Morrowind
    Bug #1217: Container content changes based on the current position of the mouse
    Bug #1234: Loading/saving issues with dynamic records
    Bug #1277: Text pasted into the console appears twice
    Bug #1284: Crash on New Game
    Bug #1303: It's possible to skip the chargen
    Bug #1304: Slaughterfish should not detect the player unless the player is in the water
    Bug #1311: Editor: deleting Record Filter line does not reset the filter
    Bug #1324: ERROR: ESM Error: String table overflow when loading Animated Morrowind.esp
    Bug #1328: Editor: Bogus Filter created when dragging multiple records to filter bar of non-applicable table
    Bug #1331: Walking/running sound persist after killing NPC`s that are walking/running.
    Bug #1334: Previously equipped items not shown as unequipped after attempting to sell them.
    Bug #1335: Actors ignore vertical axis when deciding to attack
    Bug #1338: Unknown toggle option for shadows
    Bug #1339: "Ashlands Region" is visible when beginning new game during "Loading Area" process
    Bug #1340: Guards prompt Player with punishment options after resisting arrest with another guard.
    Bug #1348: Regression: Bug #1098 has returned with a vengeance
    Bug #1349: [TR] TR_Data mesh tr_ex_imp_gatejamb01 cannot be activated
    Bug #1352: Disabling an ESX file does not disable dependent ESX files
    Bug #1355: CppCat Checks OpenMW
    Bug #1356: Incorrect voice type filtering for sleep interrupts
    Bug #1357: Restarting the game clears saves
    Bug #1360: Seyda Neen silk rider dialog problem
    Bug #1361: Some lights don't work
    Bug #1364: It is difficult to bind "Mouse 1" to an action in the options menu
    Bug #1370: Animation compilation mod does not work properly
    Bug #1371: SL_Pick01.nif from third party fails to load in openmw, but works in Vanilla
    Bug #1373: When stealing in front of Sellus Gravius cannot exit the dialog
    Bug #1378: Installs to /usr/local are not working
    Bug #1380: Loading a save file fail if one of the content files is disabled
    Bug #1382: "getHExact() size mismatch" crash on loading official plugin "Siege at Firemoth.esp"
    Bug #1386: Arkngthand door will not open
    Bug #1388: Segfault when modifying View Distance in Menu options
    Bug #1389: Crash when loading a save after dying
    Bug #1390: Apostrophe characters not displayed [French version]
    Bug #1391: Custom made icon background texture for magical weapons and stuff isn't scaled properly on GUI.
    Bug #1393: Coin icon during the level up dialogue are off of the background
    Bug #1394: Alt+F4 doesn't work on Win version
    Bug #1395: Changing rings switches only the last one put on
    Bug #1396: Pauldron parts aren't showing when the robe is equipped
    Bug #1402: Dialogue of some shrines have wrong button orientation
    Bug #1403: Items are floating in the air when they're dropped onto dead bodies.
    Bug #1404: Forearms are not rendered on Argonian females
    Bug #1407: Alchemy allows making potions from two of the same item
    Bug #1408: "Max sale" button gives you all the items AND all the trader's gold
    Bug #1409: Rest "Until Healed" broken for characters with stunted magicka.
    Bug #1412: Empty travel window opens while playing through start game
    Bug #1413: Save game ignores missing writing permission
    Bug #1414: The Underground 2 ESM Error
    Bug #1416: Not all splash screens in the Splash directory are used
    Bug #1417: Loading saved game does not terminate
    Bug #1419: Skyrim: Home of the Nords error
    Bug #1422: ClearInfoActor
    Bug #1423: ForceGreeting closes existing dialogue windows
    Bug #1425: Cannot load save game
    Bug #1426: Read skill books aren't stored in savegame
    Bug #1427: Useless items can be set under hotkeys
    Bug #1429: Text variables in journal
    Bug #1432: When attacking friendly NPC, the crime is reported and bounty is raised after each swing
    Bug #1435: Stealing priceless items is without punishment
    Bug #1437: Door marker at Jobasha's Rare Books is spawning PC in the air
    Bug #1440: Topic selection menu should be wider
    Bug #1441: Dropping items on the rug makes them inaccessible
    Bug #1442: When dropping and taking some looted items, bystanders consider that as a crime
    Bug #1444: Arrows and bolts are not dropped where the cursor points
    Bug #1445: Security trainers offering acrobatics instead
    Bug #1447: Character dash not displayed, French edition
    Bug #1448: When the player is killed by the guard while having a bounty on his head, the guard dialogue opens over and over instead of loading dialogue
    Bug #1454: Script error in SkipTutorial
    Bug #1456: Bad lighting when using certain Morrowind.ini generated by MGE
    Bug #1457: Heart of Lorkan comes after you when attacking it
    Bug #1458: Modified Keybindings are not remembered
    Bug #1459: Dura Gra-Bol doesn't respond to PC attack
    Bug #1462: Interior cells not loaded with Morrowind Patch active
    Bug #1469: Item tooltip should show the base value, not real value
    Bug #1477: Death count is not stored in savegame
    Bug #1478: AiActivate does not trigger activate scripts
    Bug #1481: Weapon not rendered when partially submerged in water
    Bug #1483: Enemies are attacking even while dying
    Bug #1486: ESM Error: Don't know what to do with INFO
    Bug #1490: Arrows shot at PC can end up in inventory
    Bug #1492: Monsters respawn on top of one another
    Bug #1493: Dialogue box opens with follower NPC even if NPC is dead
    Bug #1494: Paralysed cliffracers remain airbourne
    Bug #1495: Dialogue box opens with follower NPC even the game is paused
    Bug #1496: GUI messages are not cleared when loading another saved game
    Bug #1499: Underwater sound sometimes plays when transitioning from interior.
    Bug #1500: Targetted spells and water.
    Bug #1502: Console error message on info refusal
    Bug #1507: Bloodmoon MQ The Ritual of Beasts: Can't remove the arrow
    Bug #1508: Bloodmoon: Fort Frostmoth, cant talk with Carnius Magius
    Bug #1516: PositionCell doesn't move actors to current cell
    Bug #1518: ForceGreeting broken for explicit references
    Bug #1522: Crash after attempting to play non-music file
    Bug #1523: World map empty after loading interior save
    Bug #1524: Arrows in waiting/resting dialog act like minimum and maximum buttons
    Bug #1525: Werewolf: Killed NPC's don't fill werewolfs hunger for blood
    Bug #1527: Werewolf: Detect life detects wrong type of actor
    Bug #1529: OpenMW crash during "the shrine of the dead" mission (tribunal)
    Bug #1530: Selected text in the console has the same color as the background
    Bug #1539: Barilzar's Mazed Band: Tribunal
    Bug #1542: Looping taunts from NPC`s after death: Tribunal
    Bug #1543: OpenCS crash when using drag&drop in script editor
    Bug #1547: Bamz-Amschend: Centurion Archers combat problem
    Bug #1548: The Missing Hand: Tribunal
    Bug #1549: The Mad God: Tribunal, Dome of Serlyn
    Bug #1557: A bounty is calculated from actual item cost
    Bug #1562: Invisible terrain on top of Red Mountain
    Bug #1564: Cave of the hidden music: Bloodmoon
    Bug #1567: Editor: Deleting of referenceables does not work
    Bug #1568: Picking up a stack of items and holding the enter key and moving your mouse around paints a bunch of garbage on screen.
    Bug #1574: Solstheim: Drauger cant inflict damage on player
    Bug #1578: Solstheim: Bonewolf running animation not working
    Bug #1585: Particle effects on PC are stopped when paralyzed
    Bug #1589: Tribunal: Crimson Plague quest does not update when Gedna Relvel is killed
    Bug #1590: Failed to save game: compile error
    Bug #1598: Segfault when making Drain/Fortify Skill spells
    Bug #1599: Unable to switch to fullscreen
    Bug #1613: Morrowind Rebirth duplicate objects / vanilla objects not removed
    Bug #1618: Death notice fails to show up
    Bug #1628: Alt+Tab Segfault
    Feature #32: Periodic Cleanup/Refill
    Feature #41: Precipitation and weather particles
    Feature #568: Editor: Configuration setup
    Feature #649: Editor: Threaded loading
    Feature #930: Editor: Cell record saving
    Feature #934: Editor: Body part table
    Feature #935: Editor: Enchantment effect table
    Feature #1162: Dialogue merging
    Feature #1174: Saved Game: add missing creature state
    Feature #1177: Saved Game: fog of war state
    Feature #1312: Editor: Combat/Magic/Stealth values for creatures are not displayed
    Feature #1314: Make NPCs and creatures fight each other
    Feature #1315: Crime: Murder
    Feature #1321: Sneak skill enhancements
    Feature #1323: Handle restocking items
    Feature #1332: Saved Game: levelled creatures
    Feature #1347: modFactionReaction script instruction
    Feature #1362: Animated main menu support
    Feature #1433: Store walk/run toggle
    Feature #1449: Use names instead of numbers for saved game files and folders
    Feature #1453: Adding Delete button to the load menu
    Feature #1460: Enable Journal screen while in dialogue
    Feature #1480: Play Battle music when in combat
    Feature #1501: Followers unable to fast travel with you
    Feature #1520: Disposition and distance-based aggression/ShouldAttack
    Feature #1595: Editor: Object rendering in cells
    Task #940: Move license to locations where applicable
    Task #1333: Remove cmake git tag reading
    Task #1566: Editor: Object rendering refactoring

0.30.0
------

    Bug #416: Extreme shaking can occur during cell transitions while moving
    Bug #1003: Province Cyrodiil: Ogre Exception in Stirk
    Bug #1071: Crash when given a non-existent content file
    Bug #1080: OpenMW allows resting/using a bed while in combat
    Bug #1097: Wrong punishment for stealing in Census and Excise Office at the start of a new game
    Bug #1098: Unlocked evidence chests should get locked after new evidence is put into them
    Bug #1099: NPCs that you attacked still fight you after you went to jail/paid your fine
    Bug #1100: Taking items from a corpse is considered stealing
    Bug #1126: Some creatures can't get close enough to attack
    Bug #1144: Killed creatures seem to die again each time player transitions indoors/outdoors
    Bug #1181: loading a saved game does not reset the player control status
    Bug #1185: Collision issues in Addamasartus
    Bug #1187: Athyn Sarethi mission, rescuing varvur sarethi from the doesnt end the mission
    Bug #1189: Crash when entering interior cell "Gnisis, Arvs-Drelen"
    Bug #1191: Picking up papers without inventory in new game
    Bug #1195: NPCs do not equip torches in certain interiors
    Bug #1197: mouse wheel makes things scroll too fast
    Bug #1200: door blocked by monsters
    Bug #1201: item's magical charges are only refreshed when they are used
    Bug #1203: Scribs do not defend themselves
    Bug #1204: creatures life is not empty when they are dead
    Bug #1205: armor experience does not progress when hits are taken
    Bug #1206: blood particules always red. Undeads and mechanicals should have a different one.
    Bug #1209: Tarhiel never falls
    Bug #1210: journal adding script is ran again after having saved/loaded
    Bug #1224: Names of custom classes are not properly handled in save games
    Bug #1227: Editor: Fixed case handling for broken localised versions of Morrowind.esm
    Bug #1235: Indoors walk stutter
    Bug #1236: Aborting intro movie brings up the menu
    Bug #1239: NPCs get stuck when walking past each other
    Bug #1240: BTB - Settings 14.1 and Health Bar.
    Bug #1241: BTB - Character and Khajiit Prejudice
    Bug #1248: GUI Weapon icon is changed to hand-to-hand after save load
    Bug #1254: Guild ranks do not show in dialogue
    Bug #1255: When opening a container and selecting "Take All", the screen flashes blue
    Bug #1260: Level Up menu doesn't show image when using a custom class
    Bug #1265: Quit Menu Has Misaligned Buttons
    Bug #1270: Active weapon icon is not updated when weapon is repaired
    Bug #1271: NPC Stuck in hovering "Jumping" animation
    Bug #1272: Crash when attempting to load Big City esm file.
    Bug #1276: Editor: Dropping a region into the filter of a cell subview fails
    Bug #1286: Dialogue topic list clips with window frame
    Bug #1291: Saved game: store faction membership
    Bug #1293: Pluginless Khajiit Head Pack by ashiraniir makes OpenMW close.
    Bug #1294: Pasting in console adds text to end, not at cursor
    Bug #1295: Conversation loop when asking about "specific place" in Vivec
    Bug #1296: Caius doesn't leave at start of quest "Mehra Milo and the Lost Prophecies"
    Bug #1297: Saved game: map markers
    Bug #1302: ring_keley script causes vector::_M_range_check exception
    Bug #1309: Bug on "You violated the law" dialog
    Bug #1319: Creatures sometimes rendered incorrectly
    Feature #50: Ranged Combat
    Feature #58: Sneaking Skill
    Feature #73: Crime and Punishment
    Feature #135: Editor: OGRE integration
    Feature #541: Editor: Dialogue Sub-Views
    Feature #853: Editor: Rework User Settings
    Feature #944: Editor: lighting modes
    Feature #945: Editor: Camera navigation mode
    Feature #953: Trader gold
    Feature #1140: AI: summoned creatures
    Feature #1142: AI follow: Run stance
    Feature #1154: Not all NPCs get aggressive when one is attacked
    Feature #1169: Terrain threading
    Feature #1172: Loading screen and progress bars during saved/loading game
    Feature #1173: Saved Game: include weather state
    Feature #1207: Class creation form does not remember
    Feature #1220: Editor: Preview Subview
    Feature #1223: Saved Game: Local Variables
    Feature #1229: Quicksave, quickload, autosave
    Feature #1230: Deleting saves
    Feature #1233: Bribe gold is placed into NPCs inventory
    Feature #1252: Saved Game: quick key bindings
    Feature #1273: Editor: Region Map context menu
    Feature #1274: Editor: Region Map drag & drop
    Feature #1275: Editor: Scene subview drop
    Feature #1282: Non-faction member crime recognition.
    Feature #1289: NPCs return to default position
    Task #941: Remove unused cmake files

0.29.0
------

    Bug #556: Video soundtrack not played when music volume is set to zero
    Bug #829: OpenMW uses up all available vram, when playing for extended time
    Bug #848: Wrong amount of footsteps playing in 1st person
    Bug #888: Ascended Sleepers have movement issues
    Bug #892: Explicit references are allowed on all script functions
    Bug #999: Graphic Herbalism (mod): sometimes doesn't activate properly
    Bug #1009: Lake Fjalding AI related slowdown.
    Bug #1041: Music playback issues on OS X >= 10.9
    Bug #1043: No message box when advancing skill "Speechcraft" while in dialog window
    Bug #1060: Some message boxes are cut off at the bottom
    Bug #1062: Bittercup script does not work ('end' variable)
    Bug #1074: Inventory paperdoll obscures armour rating
    Bug #1077: Message after killing an essential NPC disappears too fast
    Bug #1078: "Clutterbane" shows empty charge bar
    Bug #1083: UndoWerewolf fails
    Bug #1088: Better Clothes Bloodmoon Plus 1.5 by Spirited Treasure pants are not rendered
    Bug #1090: Start scripts fail when going to a non-predefined cell
    Bug #1091: Crash: Assertion `!q.isNaN() && "Invalid orientation supplied as parameter"' failed.
    Bug #1093: Weapons of aggressive NPCs are invisible after you exit and re-enter interior
    Bug #1105: Magicka is depleted when using uncastable spells
    Bug #1106: Creatures should be able to run
    Bug #1107: TR cliffs have way too huge collision boxes in OpenMW
    Bug #1109: Cleaning True Light and Darkness with Tes3cmd makes Addamasartus , Zenarbael and Yasamsi flooded.
    Bug #1114: Bad output for desktop-file-validate on openmw.desktop (and opencs.desktop)
    Bug #1115: Memory leak when spying on Fargoth
    Bug #1137: Script execution fails (drenSlaveOwners script)
    Bug #1143: Mehra Milo quest (vivec informants) is broken
    Bug #1145: Issues with moving gold between inventory and containers
    Bug #1146: Issues with picking up stacks of gold
    Bug #1147: Dwemer Crossbows are held incorrectly
    Bug #1158: Armor rating should always stay below inventory mannequin
    Bug #1159: Quick keys can be set during character generation
    Bug #1160: Crash on equip lockpick when
    Bug #1167: Editor: Referenceables are not correctly loaded when dealing with more than one content file
    Bug #1184: Game Save: overwriting an existing save does not actually overwrites the file
    Feature #30: Loading/Saving (still missing a few parts)
    Feature #101: AI Package: Activate
    Feature #103: AI Package: Follow, FollowCell
    Feature #138: Editor: Drag & Drop
    Feature #428: Player death
    Feature #505: Editor: Record Cloning
    Feature #701: Levelled creatures
    Feature #708: Improved Local Variable handling
    Feature #709: Editor: Script verifier
    Feature #764: Missing journal backend features
    Feature #777: Creature weapons/shields
    Feature #789: Editor: Referenceable record verifier
    Feature #924: Load/Save GUI (still missing loading screen and progress bars)
    Feature #946: Knockdown
    Feature #947: Decrease fatigue when running, swimming and attacking
    Feature #956: Melee Combat: Blocking
    Feature #957: Area magic
    Feature #960: Combat/AI combat for creatures
    Feature #962: Combat-Related AI instructions
    Feature #1075: Damage/Restore skill/attribute magic effects
    Feature #1076: Soultrap magic effect
    Feature #1081: Disease contraction
    Feature #1086: Blood particles
    Feature #1092: Interrupt resting
    Feature #1101: Inventory equip scripts
    Feature #1116: Version/Build number in Launcher window
    Feature #1119: Resistance/weakness to normal weapons magic effect
    Feature #1123: Slow Fall magic effect
    Feature #1130: Auto-calculate spells
    Feature #1164: Editor: Case-insensitive sorting in tables

0.28.0
------

    Bug #399: Inventory changes are not visible immediately
    Bug #417: Apply weather instantly when teleporting
    Bug #566: Global Map position marker not updated for interior cells
    Bug #712: Looting corpse delay
    Bug #716: Problem with the "Vurt's Ascadian Isles Mod" mod
    Bug #805: Two TR meshes appear black (v0.24RC)
    Bug #841: Third-person activation distance taken from camera rather than head
    Bug #845: NPCs hold torches during the day
    Bug #855: Vvardenfell Visages Volume I some hairs don´t appear since 0,24
    Bug #856: Maormer race by Mac Kom - The heads are way up
    Bug #864: Walk locks during loading in 3rd person
    Bug #871: active weapon/magic item icon is not immediately made blank if item is removed during dialog
    Bug #882: Hircine's Ring doesn't always work
    Bug #909: [Tamriel Rebuilt] crashes in Akamora
    Bug #922: Launcher writing merged openmw.cfg files
    Bug #943: Random magnitude should be calculated per effect
    Bug #948: Negative fatigue level should be allowed
    Bug #949: Particles in world space
    Bug #950: Hard crash on x64 Linux running --new-game (on startup)
    Bug #951: setMagicka and setFatigue have no effect
    Bug #954: Problem with equipping inventory items when using a keyboard shortcut
    Bug #955: Issues with equipping torches
    Bug #966: Shield is visible when casting spell
    Bug #967: Game crashes when equipping silver candlestick
    Bug #970: Segmentation fault when starting at Bal Isra
    Bug #977: Pressing down key in console doesn't go forward in history
    Bug #979: Tooltip disappears when changing inventory
    Bug #980: Barter: item category is remembered, but not shown
    Bug #981: Mod: replacing model has wrong position/orientation
    Bug #982: Launcher: Addon unchecking is not saved
    Bug #983: Fix controllers to affect objects attached to the base node
    Bug #985: Player can talk to NPCs who are in combat
    Bug #989: OpenMW crashes when trying to include mod with capital .ESP
    Bug #991: Merchants equip items with harmful constant effect enchantments
    Bug #994: Don't cap skills/attributes when set via console
    Bug #998: Setting the max health should also set the current health
    Bug #1005: Torches are visible when casting spells and during hand to hand combat.
    Bug #1006: Many NPCs have 0 skill
    Bug #1007: Console fills up with text
    Bug #1013: Player randomly loses health or dies
    Bug #1014: Persuasion window is not centered in maximized window
    Bug #1015: Player status window scroll state resets on status change
    Bug #1016: Notification window not big enough for all skill level ups
    Bug #1020: Saved window positions are not rescaled appropriately on resolution change
    Bug #1022: Messages stuck permanently on screen when they pile up
    Bug #1023: Journals doesn't open
    Bug #1026: Game loses track of torch usage.
    Bug #1028: Crash on pickup of jug in Unexplored Shipwreck, Upper level
    Bug #1029: Quick keys menu: Select compatible replacement when tool used up
    Bug #1042: TES3 header data wrong encoding
    Bug #1045: OS X: deployed OpenCS won't launch
    Bug #1046: All damaged weaponry is worth 1 gold
    Bug #1048: Links in "locked" dialogue are still clickable
    Bug #1052: Using color codes when naming your character actually changes the name's color
    Bug #1054: Spell effects not visible in front of water
    Bug #1055: Power-Spell animation starts even though you already casted it that day
    Bug #1059: Cure disease potion removes all effects from player, even your race bonus and race ability
    Bug #1063: Crash upon checking out game start ship area in Seyda Neen
    Bug #1064: openmw binaries link to unnecessary libraries
    Bug #1065: Landing from a high place in water still causes fall damage
    Bug #1072: Drawing weapon increases torch brightness
    Bug #1073: Merchants sell stacks of gold
    Feature #43: Visuals for Magic Effects
    Feature #51: Ranged Magic
    Feature #52: Touch Range Magic
    Feature #53: Self Range Magic
    Feature #54: Spell Casting
    Feature #70: Vampirism
    Feature #100: Combat AI
    Feature #171: Implement NIF record NiFlipController
    Feature #410: Window to restore enchanted item charge
    Feature #647: Enchanted item glow
    Feature #723: Invisibility/Chameleon magic effects
    Feature #737: Resist Magicka magic effect
    Feature #758: GetLOS
    Feature #926: Editor: Info-Record tables
    Feature #958: Material controllers
    Feature #959: Terrain bump, specular, & parallax mapping
    Feature #990: Request: unlock mouse when in any menu
    Feature #1018: Do not allow view mode switching while performing an action
    Feature #1027: Vertex morph animation (NiGeomMorpherController)
    Feature #1031: Handle NiBillboardNode
    Feature #1051: Implement NIF texture slot DarkTexture
    Task #873: Unify OGRE initialisation

0.27.0
------

    Bug #597: Assertion `dialogue->mId == id' failed in esmstore.cpp
    Bug #794: incorrect display of decimal numbers
    Bug #840: First-person sneaking camera height
    Bug #887: Ambient sounds playing while paused
    Bug #902: Problems with Polish character encoding
    Bug #907: Entering third person using the mousewheel is possible even if it's impossible using the key
    Bug #910: Some CDs not working correctly with Unshield installer
    Bug #917: Quick character creation plugin does not work
    Bug #918: Fatigue does not refill
    Bug #919: The PC falls dead in Beshara - OpenMW nightly Win64 (708CDE2)
    Feature #57: Acrobatics Skill
    Feature #462: Editor: Start Dialogue
    Feature #546: Modify ESX selector to handle new content file scheme
    Feature #588: Editor: Adjust name/path of edited content files
    Feature #644: Editor: Save
    Feature #710: Editor: Configure script compiler context
    Feature #790: God Mode
    Feature #881: Editor: Allow only one instance of OpenCS
    Feature #889: Editor: Record filtering
    Feature #895: Extinguish torches
    Feature #898: Breath meter enhancements
    Feature #901: Editor: Default record filter
    Feature #913: Merge --master and --plugin switches

0.26.0
------

    Bug #274: Inconsistencies in the terrain
    Bug #557: Already-dead NPCs do not equip clothing/items.
    Bug #592: Window resizing
    Bug #612: [Tamriel Rebuilt] Missing terrain (South of Tel Oren)
    Bug #664: Heart of lorkhan acts like a dead body (container)
    Bug #767: Wonky ramp physics & water
    Bug #780: Swimming out of water
    Bug #792: Wrong ground alignment on actors when no clipping
    Bug #796: Opening and closing door sound issue
    Bug #797: No clipping hinders opening and closing of doors
    Bug #799: sliders in enchanting window
    Bug #838: Pressing key during startup procedure freezes the game
    Bug #839: Combat/magic stances during character creation
    Bug #843: [Tribunal] Dark Brotherhood assassin appears without equipment
    Bug #844: Resting "until healed" option given even with full stats
    Bug #846: Equipped torches are invisible.
    Bug #847: Incorrect formula for autocalculated NPC initial health
    Bug #850: Shealt weapon sound plays when leaving magic-ready stance
    Bug #852: Some boots do not produce footstep sounds
    Bug #860: FPS bar misalignment
    Bug #861: Unable to print screen
    Bug #863: No sneaking and jumping at the same time
    Bug #866: Empty variables in [Movies] section of Morrowind.ini gets imported into OpenMW.cfg as blank fallback option and crashes game on start.
    Bug #867: Dancing girls in "Suran, Desele's House of Earthly Delights" don't dance.
    Bug #868: Idle animations are repeated
    Bug #874: Underwater swimming close to the ground is jerky
    Bug #875: Animation problem while swimming on the surface and looking up
    Bug #876: Always a starting upper case letter in the inventory
    Bug #878: Active spell effects don't update the layout properly when ended
    Bug #891: Cell 24,-12 (Tamriel Rebuilt) crashes on load
    Bug #896: New game sound issue
    Feature #49: Melee Combat
    Feature #71: Lycanthropy
    Feature #393: Initialise MWMechanics::AiSequence from ESM::AIPackageList
    Feature #622: Multiple positions for inventory window
    Feature #627: Drowning
    Feature #786: Allow the 'Activate' key to close the countdialog window
    Feature #798: Morrowind installation via Launcher (Linux/Max OS only)
    Feature #851: First/Third person transitions with mouse wheel
    Task #689: change PhysicActor::enableCollisions
    Task #707: Reorganise Compiler

0.25.0
------

    Bug #411: Launcher crash on OS X < 10.8
    Bug #604: Terrible performance drop in the Census and Excise Office.
    Bug #676: Start Scripts fail to load
    Bug #677: OpenMW does not accept script names with -
    Bug #766: Extra space in front of topic links
    Bug #793: AIWander Isn't Being Passed The Repeat Parameter
    Bug #795: Sound playing with drawn weapon and crossing cell-border
    Bug #800: can't select weapon for enchantment
    Bug #801: Player can move while over-encumbered
    Bug #802: Dead Keys not working
    Bug #808: mouse capture
    Bug #809: ini Importer does not work without an existing cfg file
    Bug #812: Launcher will run OpenMW with no ESM or ESP selected
    Bug #813: OpenMW defaults to Morrowind.ESM with no ESM or ESP selected
    Bug #817: Dead NPCs and Creatures still have collision boxes
    Bug #820: Incorrect sorting of answers (Dialogue)
    Bug #826: mwinimport dumps core when given an unknown parameter
    Bug #833: getting stuck in door
    Bug #835: Journals/books not showing up properly.
    Feature #38: SoundGen
    Feature #105: AI Package: Wander
    Feature #230: 64-bit compatibility for OS X
    Feature #263: Hardware mouse cursors
    Feature #449: Allow mouse outside of window while paused
    Feature #736: First person animations
    Feature #750: Using mouse wheel in third person mode
    Feature #822: Autorepeat for slider buttons

0.24.0
------

    Bug #284: Book's text misalignment
    Bug #445: Camera able to get slightly below floor / terrain
    Bug #582: Seam issue in Red Mountain
    Bug #632: Journal Next Button shows white square
    Bug #653: IndexedStore ignores index
    Bug #694: Parser does not recognize float values starting with .
    Bug #699: Resource handling broken with Ogre 1.9 trunk
    Bug #718: components/esm/loadcell is using the mwworld subsystem
    Bug #729: Levelled item list tries to add nonexistent item
    Bug #730: Arrow buttons in the settings menu do not work.
    Bug #732: Erroneous behavior when binding keys
    Bug #733: Unclickable dialogue topic
    Bug #734: Book empty line problem
    Bug #738: OnDeath only works with implicit references
    Bug #740: Script compiler fails on scripts with special names
    Bug #742: Wait while no clipping
    Bug #743: Problem with changeweather console command
    Bug #744: No wait dialogue after starting a new game
    Bug #748: Player is not able to unselect objects with the console
    Bug #751: AddItem should only spawn a message box when called from dialogue
    Bug #752: The enter button has several functions in trade and looting that is not impelemted.
    Bug #753: Fargoth's Ring Quest Strange Behavior
    Bug #755: Launcher writes duplicate lines into settings.cfg
    Bug #759: Second quest in mages guild does not work
    Bug #763: Enchantment cast cost is wrong
    Bug #770: The "Take" and "Close" buttons in the scroll GUI are stretched incorrectly
    Bug #773: AIWander Isn't Being Passed The Correct idle Values
    Bug #778: The journal can be opened at the start of a new game
    Bug #779: Divayth Fyr starts as dead
    Bug #787: "Batch count" on detailed FPS counter gets cut-off
    Bug #788: chargen scroll layout does not match vanilla
    Feature #60: Atlethics Skill
    Feature #65: Security Skill
    Feature #74: Interaction with non-load-doors
    Feature #98: Render Weapon and Shield
    Feature #102: AI Package: Escort, EscortCell
    Feature #182: Advanced Journal GUI
    Feature #288: Trading enhancements
    Feature #405: Integrate "new game" into the menu
    Feature #537: Highlight dialogue topic links
    Feature #658: Rotate, RotateWorld script instructions and local rotations
    Feature #690: Animation Layering
    Feature #722: Night Eye/Blind magic effects
    Feature #735: Move, MoveWorld script instructions.
    Feature #760: Non-removable corpses

0.23.0
------

    Bug #522: Player collides with placeable items
    Bug #553: Open/Close sounds played when accessing main menu w/ Journal Open
    Bug #561: Tooltip word wrapping delay
    Bug #578: Bribing works incorrectly
    Bug #601: PositionCell fails on negative coordinates
    Bug #606: Some NPCs hairs not rendered with Better Heads addon
    Bug #609: Bad rendering of bone boots
    Bug #613: Messagebox causing assert to fail
    Bug #631: Segfault on shutdown
    Bug #634: Exception when talking to Calvus Horatius in Mournhold, royal palace courtyard
    Bug #635: Scale NPCs depending on race
    Bug #643: Dialogue Race select function is inverted
    Bug #646: Twohanded weapons don't work properly
    Bug #654: Crash when dropping objects without a collision shape
    Bug #655/656: Objects that were disabled or deleted (but not both) were added to the scene when re-entering a cell
    Bug #660: "g" in "change" cut off in Race Menu
    Bug #661: Arrille sells me the key to his upstairs room
    Bug #662: Day counter starts at 2 instead of 1
    Bug #663: Cannot select "come unprepared" topic in dialog with Dagoth Ur
    Bug #665: Pickpocket -> "Grab all" grabs all NPC inventory, even not listed in container window.
    Bug #666: Looking up/down problem
    Bug #667: Active effects border visible during loading
    Bug #669: incorrect player position at new game start
    Bug #670: race selection menu: sex, face and hair left button not totally clickable
    Bug #671: new game: player is naked
    Bug #674: buying or selling items doesn't change amount of gold
    Bug #675: fatigue is not set to its maximum when starting a new game
    Bug #678: Wrong rotation order causes RefData's rotation to be stored incorrectly
    Bug #680: different gold coins in Tel Mara
    Bug #682: Race menu ignores playable flag for some hairs and faces
    Bug #685: Script compiler does not accept ":" after a function name
    Bug #688: dispose corpse makes cross-hair to disappear
    Bug #691: Auto equipping ignores equipment conditions
    Bug #692: OpenMW doesnt load "loose file" texture packs that places resources directly in data folder
    Bug #696: Draugr incorrect head offset
    Bug #697: Sail transparency issue
    Bug #700: "On the rocks" mod does not load its UV coordinates correctly.
    Bug #702: Some race mods don't work
    Bug #711: Crash during character creation
    Bug #715: Growing Tauryon
    Bug #725: Auto calculate stats
    Bug #728: Failure to open container and talk dialogue
    Bug #731: Crash with Mush-Mere's "background" topic
    Feature #55/657: Item Repairing
    Feature #62/87: Enchanting
    Feature #99: Pathfinding
    Feature #104: AI Package: Travel
    Feature #129: Levelled items
    Feature #204: Texture animations
    Feature #239: Fallback-Settings
    Feature #535: Console object selection improvements
    Feature #629: Add levelup description in levelup layout dialog
    Feature #630: Optional format subrecord in (tes3) header
    Feature #641: Armor rating
    Feature #645: OnDeath script function
    Feature #683: Companion item UI
    Feature #698: Basic Particles
    Task #648: Split up components/esm/loadlocks
    Task #695: mwgui cleanup

0.22.0
------

    Bug #311: Potential infinite recursion in script compiler
    Bug #355: Keyboard repeat rate (in Xorg) are left disabled after game exit.
    Bug #382: Weird effect in 3rd person on water
    Bug #387: Always use detailed shape for physics raycasts
    Bug #420: Potion/ingredient effects do not stack
    Bug #429: Parts of dwemer door not picked up correctly for activation/tooltips
    Bug #434/Bug #605: Object movement between cells not properly implemented
    Bug #502: Duplicate player collision model at origin
    Bug #509: Dialogue topic list shifts inappropriately
    Bug #513: Sliding stairs
    Bug #515: Launcher does not support non-latin strings
    Bug #525: Race selection preview camera wrong position
    Bug #526: Attributes / skills should not go below zero
    Bug #529: Class and Birthsign menus options should be preselected
    Bug #530: Lock window button graphic missing
    Bug #532: Missing map menu graphics
    Bug #545: ESX selector does not list ESM files properly
    Bug #547: Global variables of type short are read incorrectly
    Bug #550: Invisible meshes collision and tooltip
    Bug #551: Performance drop when loading multiple ESM files
    Bug #552: Don't list CG in options if it is not available
    Bug #555: Character creation windows "OK" button broken
    Bug #558: Segmentation fault when Alt-tabbing with console opened
    Bug #559: Dialog window should not be available before character creation is finished
    Bug #560: Tooltip borders should be stretched
    Bug #562: Sound should not be played when an object cannot be picked up
    Bug #565: Water animation speed + timescale
    Bug #572: Better Bodies' textures don't work
    Bug #573: OpenMW doesn't load if TR_Mainland.esm is enabled (Tamriel Rebuilt mod)
    Bug #574: Moving left/right should not cancel auto-run
    Bug #575: Crash entering the Chamber of Song
    Bug #576: Missing includes
    Bug #577: Left Gloves Addon causes ESMReader exception
    Bug #579: Unable to open container "Kvama Egg Sack"
    Bug #581: Mimicking vanilla Morrowind water
    Bug #583: Gender not recognized
    Bug #586: Wrong char gen behaviour
    Bug #587: "End" script statements with spaces don't work
    Bug #589: Closing message boxes by pressing the activation key
    Bug #590: Ugly Dagoth Ur rendering
    Bug #591: Race selection issues
    Bug #593: Persuasion response should be random
    Bug #595: Footless guard
    Bug #599: Waterfalls are invisible from a certain distance
    Bug #600: Waterfalls rendered incorrectly, cut off by water
    Bug #607: New beast bodies mod crashes
    Bug #608: Crash in cell "Mournhold, Royal Palace"
    Bug #611: OpenMW doesn't find some of textures used in Tamriel Rebuilt
    Bug #613: Messagebox causing assert to fail
    Bug #615: Meshes invisible from above water
    Bug #617: Potion effects should be hidden until discovered
    Bug #619: certain moss hanging from tree has rendering bug
    Bug #621: Batching bloodmoon's trees
    Bug #623: NiMaterialProperty alpha unhandled
    Bug #628: Launcher in latest master crashes the game
    Bug #633: Crash on startup: Better Heads
    Bug #636: Incorrect Char Gen Menu Behavior
    Feature #29: Allow ESPs and multiple ESMs
    Feature #94: Finish class selection-dialogue
    Feature #149: Texture Alphas
    Feature #237: Run Morrowind-ini importer from launcher
    Feature #286: Update Active Spell Icons
    Feature #334: Swimming animation
    Feature #335: Walking animation
    Feature #360: Proper collision shapes for NPCs and creatures
    Feature #367: Lights that behave more like original morrowind implementation
    Feature #477: Special local scripting variables
    Feature #528: Message boxes should close when enter is pressed under certain conditions.
    Feature #543: Add bsa files to the settings imported by the ini importer
    Feature #594: coordinate space and utility functions
    Feature #625: Zoom in vanity mode
    Task #464: Refactor launcher ESX selector into a re-usable component
    Task #624: Unified implementation of type-variable sub-records

0.21.0
------

    Bug #253: Dialogs don't work for Russian version of Morrowind
    Bug #267: Activating creatures without dialogue can still activate the dialogue GUI
    Bug #354: True flickering lights
    Bug #386: The main menu's first entry is wrong (in french)
    Bug #479: Adding the spell "Ash Woe Blight" to the player causes strange attribute oscillations
    Bug #495: Activation Range
    Bug #497: Failed Disposition check doesn't stop a dialogue entry from being returned
    Bug #498: Failing a disposition check shouldn't eliminate topics from the the list of those available
    Bug #500: Disposition for most NPCs is 0/100
    Bug #501: Getdisposition command wrongly returns base disposition
    Bug #506: Journal UI doesn't update anymore
    Bug #507: EnableRestMenu is not a valid command - change it to EnableRest
    Bug #508: Crash in Ald Daedroth Shrine
    Bug #517: Wrong price calculation when untrading an item
    Bug #521: MWGui::InventoryWindow creates a duplicate player actor at the origin
    Bug #524: Beast races are able to wear shoes
    Bug #527: Background music fails to play
    Bug #533: The arch at Gnisis entrance is not displayed
    Bug #534: Terrain gets its correct shape only some time after the cell is loaded
    Bug #536: The same entry can be added multiple times to the journal
    Bug #539: Race selection is broken
    Bug #544: Terrain normal map corrupt when the map is rendered
    Feature #39: Video Playback
    Feature #151: ^-escape sequences in text output
    Feature #392: Add AI related script functions
    Feature #456: Determine required ini fallback values and adjust the ini importer accordingly
    Feature #460: Experimental DirArchives improvements
    Feature #540: Execute scripts of objects in containers/inventories in active cells
    Task #401: Review GMST fixing
    Task #453: Unify case smashing/folding
    Task #512: Rewrite utf8 component

0.20.0
------

    Bug #366: Changing the player's race during character creation does not change the look of the player character
    Bug #430: Teleporting and using loading doors linking within the same cell reloads the cell
    Bug #437: Stop animations when paused
    Bug #438: Time displays as "0 a.m." when it should be "12 a.m."
    Bug #439: Text in "name" field of potion/spell creation window is persistent
    Bug #440: Starting date at a new game is off by one day
    Bug #442: Console window doesn't close properly sometimes
    Bug #448: Do not break container window formatting when item names are very long
    Bug #458: Topics sometimes not automatically added to known topic list
    Bug #476: Auto-Moving allows player movement after using DisablePlayerControls
    Bug #478: After sleeping in a bed the rest dialogue window opens automtically again
    Bug #492: On creating potions the ingredients are removed twice
    Feature #63: Mercantile skill
    Feature #82: Persuasion Dialogue
    Feature #219: Missing dialogue filters/functions
    Feature #369: Add a FailedAction
    Feature #377: Select head/hair on character creation
    Feature #391: Dummy AI package classes
    Feature #435: Global Map, 2nd Layer
    Feature #450: Persuasion
    Feature #457: Add more script instructions
    Feature #474: update the global variable pcrace when the player's race is changed
    Task #158: Move dynamically generated classes from Player class to World Class
    Task #159: ESMStore rework and cleanup
    Task #163: More Component Namespace Cleanup
    Task #402: Move player data from MWWorld::Player to the player's NPC record
    Task #446: Fix no namespace in BulletShapeLoader

0.19.0
------

    Bug #374: Character shakes in 3rd person mode near the origin
    Bug #404: Gamma correct rendering
    Bug #407: Shoes of St. Rilm do not work
    Bug #408: Rugs has collision even if they are not supposed to
    Bug #412: Birthsign menu sorted incorrectly
    Bug #413: Resolutions presented multiple times in launcher
    Bug #414: launcher.cfg file stored in wrong directory
    Bug #415: Wrong esm order in openmw.cfg
    Bug #418: Sound listener position updates incorrectly
    Bug #423: wrong usage of "Version" entry in openmw.desktop
    Bug #426: Do not use hardcoded splash images
    Bug #431: Don't use markers for raycast
    Bug #432: Crash after picking up items from an NPC
    Feature #21/#95: Sleeping/resting
    Feature #61: Alchemy Skill
    Feature #68: Death
    Feature #69/#86: Spell Creation
    Feature #72/#84: Travel
    Feature #76: Global Map, 1st Layer
    Feature #120: Trainer Window
    Feature #152: Skill Increase from Skill Books
    Feature #160: Record Saving
    Task #400: Review GMST access

0.18.0
------

    Bug #310: Button of the "preferences menu" are too small
    Bug #361: Hand-to-hand skill is always 100
    Bug #365: NPC and creature animation is jerky; Characters float around when they are not supposed to
    Bug #372: playSound3D uses original coordinates instead of current coordinates.
    Bug #373: Static OGRE build faulty
    Bug #375: Alt-tab toggle view
    Bug #376: Screenshots are disable
    Bug #378: Exception when drinking self-made potions
    Bug #380: Cloth visibility problem
    Bug #384: Weird character on doors tooltip.
    Bug #398: Some objects do not collide in MW, but do so in OpenMW
    Feature #22: Implement level-up
    Feature #36: Hide Marker
    Feature #88: Hotkey Window
    Feature #91: Level-Up Dialogue
    Feature #118: Keyboard and Mouse-Button bindings
    Feature #119: Spell Buying Window
    Feature #133: Handle resources across multiple data directories
    Feature #134: Generate a suitable default-value for --data-local
    Feature #292: Object Movement/Creation Script Instructions
    Feature #340: AIPackage data structures
    Feature #356: Ingredients use
    Feature #358: Input system rewrite
    Feature #370: Target handling in actions
    Feature #379: Door markers on the local map
    Feature #389: AI framework
    Feature #395: Using keys to open doors / containers
    Feature #396: Loading screens
    Feature #397: Inventory avatar image and race selection head preview
    Task #339: Move sounds into Action

0.17.0
------

    Bug #225: Valgrind reports about 40MB of leaked memory
    Bug #241: Some physics meshes still don't match
    Bug #248: Some textures are too dark
    Bug #300: Dependency on proprietary CG toolkit
    Bug #302: Some objects don't collide although they should
    Bug #308: Freeze in Balmora, Meldor: Armorer
    Bug #313: openmw without a ~/.config/openmw folder segfault.
    Bug #317: adding non-existing spell via console locks game
    Bug #318: Wrong character normals
    Bug #341: Building with Ogre Debug libraries does not use debug version of plugins
    Bug #347: Crash when running openmw with --start="XYZ"
    Bug #353: FindMyGUI.cmake breaks path on Windows
    Bug #359: WindowManager throws exception at destruction
    Bug #364: Laggy input on OS X due to bug in Ogre's event pump implementation
    Feature #33: Allow objects to cross cell-borders
    Feature #59: Dropping Items (replaced stopgap implementation with a proper one)
    Feature #93: Main Menu
    Feature #96/329/330/331/332/333: Player Control
    Feature #180: Object rotation and scaling.
    Feature #272: Incorrect NIF material sharing
    Feature #314: Potion usage
    Feature #324: Skill Gain
    Feature #342: Drain/fortify dynamic stats/attributes magic effects
    Feature #350: Allow console only script instructions
    Feature #352: Run scripts in console on startup
    Task #107: Refactor mw*-subsystems
    Task #325: Make CreatureStats into a class
    Task #345: Use Ogre's animation system
    Task #351: Rewrite Action class to support automatic sound playing

0.16.0
------

    Bug #250: OpenMW launcher erratic behaviour
    Bug #270: Crash because of underwater effect on OS X
    Bug #277: Auto-equipping in some cells not working
    Bug #294: Container GUI ignores disabled inventory menu
    Bug #297: Stats review dialog shows all skills and attribute values as 0
    Bug #298: MechanicsManager::buildPlayer does not remove previous bonuses
    Bug #299: Crash in World::disable
    Bug #306: Non-existent ~/.config/openmw "crash" the launcher.
    Bug #307: False "Data Files" location make the launcher "crash"
    Feature #81: Spell Window
    Feature #85: Alchemy Window
    Feature #181: Support for x.y script syntax
    Feature #242: Weapon and Spell icons
    Feature #254: Ingame settings window
    Feature #293: Allow "stacking" game modes
    Feature #295: Class creation dialog tooltips
    Feature #296: Clicking on the HUD elements should show/hide the respective window
    Feature #301: Direction after using a Teleport Door
    Feature #303: Allow object selection in the console
    Feature #305: Allow the use of = as a synonym for ==
    Feature #312: Compensation for slow object access in poorly written Morrowind.esm scripts
    Task #176: Restructure enabling/disabling of MW-references
    Task #283: Integrate ogre.cfg file in settings file
    Task #290: Auto-Close MW-reference related GUI windows

0.15.0
------

    Bug #5: Physics reimplementation (fixes various issues)
    Bug #258: Resizing arrow's background is not transparent
    Bug #268: Widening the stats window in X direction causes layout problems
    Bug #269: Topic pane in dialgoue window is too small for some longer topics
    Bug #271: Dialog choices are sorted incorrectly
    Bug #281: The single quote character is not rendered on dialog windows
    Bug #285: Terrain not handled properly in cells that are not predefined
    Bug #289: Dialogue filter isn't doing case smashing/folding for item IDs
    Feature #15: Collision with Terrain
    Feature #17: Inventory-, Container- and Trade-Windows
    Feature #44: Floating Labels above Focussed Objects
    Feature #80: Tooltips
    Feature #83: Barter Dialogue
    Feature #90: Book and Scroll Windows
    Feature #156: Item Stacking in Containers
    Feature #213: Pulsating lights
    Feature #218: Feather & Burden
    Feature #256: Implement magic effect bookkeeping
    Feature #259: Add missing information to Stats window
    Feature #260: Correct case for dialogue topics
    Feature #280: GUI texture atlasing
    Feature #291: Ability to use GMST strings from GUI layout files
    Task #255: Make MWWorld::Environment into a singleton

0.14.0
------

    Bug #1: Meshes rendered with wrong orientation
    Bug #6/Task #220: Picking up small objects doesn't always work
    Bug #127: tcg doesn't work
    Bug #178: Compablity problems with Ogre 1.8.0 RC 1
    Bug #211: Wireframe mode (toggleWireframe command) should not apply to Console & other UI
    Bug #227: Terrain crashes when moving away from predefined cells
    Bug #229: On OS X Launcher cannot launch game if path to binary contains spaces
    Bug #235: TGA texture loading problem
    Bug #246: wireframe mode does not work in water
    Feature #8/#232: Water Rendering
    Feature #13: Terrain Rendering
    Feature #37: Render Path Grid
    Feature #66: Factions
    Feature #77: Local Map
    Feature #78: Compass/Mini-Map
    Feature #97: Render Clothing/Armour
    Feature #121: Window Pinning
    Feature #205: Auto equip
    Feature #217: Contiainer should track changes to its content
    Feature #221: NPC Dialogue Window Enhancements
    Feature #233: Game settings manager
    Feature #240: Spell List and selected spell (no GUI yet)
    Feature #243: Draw State
    Task #113: Morrowind.ini Importer
    Task #215: Refactor the sound code
    Task #216: Update MyGUI

0.13.0
------

    Bug #145: Fixed sound problems after cell change
    Bug #179: Pressing space in console triggers activation
    Bug #186: CMake doesn't use the debug versions of Ogre libraries on Linux
    Bug #189: ASCII 16 character added to console on it's activation on Mac OS X
    Bug #190: Case Folding fails with music files
    Bug #192: Keypresses write Text into Console no matter which gui element is active
    Bug #196: Collision shapes out of place
    Bug #202: ESMTool doesn't not work with localised ESM files anymore
    Bug #203: Torch lights only visible on short distance
    Bug #207: Ogre.log not written
    Bug #209: Sounds do not play
    Bug #210: Ogre crash at Dren plantation
    Bug #214: Unsupported file format version
    Bug #222: Launcher is writing openmw.cfg file to wrong location
    Feature #9: NPC Dialogue Window
    Feature #16/42: New sky/weather implementation
    Feature #40: Fading
    Feature #48: NPC Dialogue System
    Feature #117: Equipping Items (backend only, no GUI yet, no rendering of equipped items yet)
    Feature #161: Load REC_PGRD records
    Feature #195: Wireframe-mode
    Feature #198/199: Various sound effects
    Feature #206: Allow picking data path from launcher if non is set
    Task #108: Refactor window manager class
    Task #172: Sound Manager Cleanup
    Task #173: Create OpenEngine systems in the appropriate manager classes
    Task #184: Adjust MSVC and gcc warning levels
    Task #185: RefData rewrite
    Task #201: Workaround for transparency issues
    Task #208: silenced esm_reader.hpp warning

0.12.0
------

    Bug #154: FPS Drop
    Bug #169: Local scripts continue running if associated object is deleted
    Bug #174: OpenMW fails to start if the config directory doesn't exist
    Bug #187: Missing lighting
    Bug #188: Lights without a mesh are not rendered
    Bug #191: Taking screenshot causes crash when running installed
    Feature #28: Sort out the cell load problem
    Feature #31: Allow the player to move away from pre-defined cells
    Feature #35: Use alternate storage location for modified object position
    Feature #45: NPC animations
    Feature #46: Creature Animation
    Feature #89: Basic Journal Window
    Feature #110: Automatically pick up the path of existing MW-installations
    Feature #183: More FPS display settings
    Task #19: Refactor engine class
    Task #109/Feature #162: Automate Packaging
    Task #112: Catch exceptions thrown in input handling functions
    Task #128/#168: Cleanup Configuration File Handling
    Task #131: NPC Activation doesn't work properly
    Task #144: MWRender cleanup
    Task #155: cmake cleanup

0.11.1
------

    Bug #2: Resources loading doesn't work outside of bsa files
    Bug #3: GUI does not render non-English characters
    Bug #7: openmw.cfg location doesn't match
    Bug #124: The TCL alias for ToggleCollision is missing.
    Bug #125: Some command line options can't be used from a .cfg file
    Bug #126: Toggle-type script instructions are less verbose compared with original MW
    Bug #130: NPC-Record Loading fails for some NPCs
    Bug #167: Launcher sets invalid parameters in ogre config
    Feature #10: Journal
    Feature #12: Rendering Optimisations
    Feature #23: Change Launcher GUI to a tabbed interface
    Feature #24: Integrate the OGRE settings window into the launcher
    Feature #25: Determine openmw.cfg location (Launcher)
    Feature #26: Launcher Profiles
    Feature #79: MessageBox
    Feature #116: Tab-Completion in Console
    Feature #132: --data-local and multiple --data
    Feature #143: Non-Rendering Performance-Optimisations
    Feature #150: Accessing objects in cells via ID does only work for objects with all lower case IDs
    Feature #157: Version Handling
    Task #14: Replace tabs with 4 spaces
    Task #18: Move components from global namespace into their own namespace
    Task #123: refactor header files in components/esm

0.10.0
------

* NPC dialogue window (not functional yet)
* Collisions with objects
* Refactor the PlayerPos class
* Adjust file locations
* CMake files and test linking for Bullet
* Replace Ogre raycasting test for activation with something more precise
* Adjust player movement according to collision results
* FPS display
* Various Portability Improvements
* Mac OS X support is back!

0.9.0
-----

* Exterior cells loading, unloading and management
* Character Creation GUI
* Character creation
* Make cell names case insensitive when doing internal lookups
* Music player
* NPCs rendering

0.8.0
-----

* GUI
* Complete and working script engine
* In game console
* Sky rendering
* Sound and music
* Tons of smaller stuff

0.7.0
-----

* This release is a complete rewrite in C++.
* All D code has been culled, and all modules have been rewritten.
* The game is now back up to the level of rendering interior cells and moving around, but physics, sound, GUI, and scripting still remain to be ported from the old codebase.

0.6.0
-----

* Coded a GUI system using MyGUI
* Skinned MyGUI to look like Morrowind (work in progress)
* Integrated the Monster script engine
* Rewrote some functions into script code
* Very early MyGUI < > Monster binding
* Fixed Windows sound problems (replaced old openal32.dll)

0.5.0
-----

* Collision detection with Bullet
* Experimental walk & fall character physics
* New key bindings:
  * t toggle physics mode (walking, flying, ghost),
  * n night eye, brightens the scene
* Fixed incompatability with DMD 1.032 and newer compilers
* * (thanks to tomqyp)
* Various minor changes and updates

0.4.0
-----

* Switched from Audiere to OpenAL
* * (BIG thanks to Chris Robinson)
* Added complete Makefile (again) as a alternative build tool
* More realistic lighting (thanks again to Chris Robinson)
* Various localization fixes tested with Russian and French versions
* Temporary workaround for the Unicode issue: invalid UTF displayed as '?'
* Added ns option to disable sound, for debugging
* Various bug fixes
* Cosmetic changes to placate gdc Wall

0.3.0
-----

* Built and tested on Windows XP
* Partial support for FreeBSD (exceptions do not work)
* You no longer have to download Monster separately
* Made an alternative for building without DSSS (but DSSS still works)
* Renamed main program from 'morro' to 'openmw'
* Made the config system more robust
* Added oc switch for showing Ogre config window on startup
* Removed some config files, these are auto generated when missing.
* Separated plugins.cfg into linux and windows versions.
* Updated Makefile and sources for increased portability
* confirmed to work against OIS 1.0.0 (Ubuntu repository package)

0.2.0
-----

* Compiles with gdc
* Switched to DSSS for building D code
* Includes the program esmtool

0.1.0
-----

first release
