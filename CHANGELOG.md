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
