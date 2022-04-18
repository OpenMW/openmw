###################
Object Record Types
###################

A game world contains many items, such as chests, weapons and monsters. All of
these items are merely instances of templates we call Objects. The OpenMW-CS
Objects table contains information about each of these template objects, such
as its value and weight in the case of items, or an aggression level in the
case of NPCs.

The following is a list of all Record Types and what you can tell OpenMW-CS
about each of them.

Activator
   Activators can have a script attached to them. As long as the cell this
   object is in is active the script will be run once per frame.

Apparatus
   This is a tool to make potions. Again there’s an icon for your inventory as
   well as a weight and a coin value. It also has a *Quality* value attached to
   it: the higher the number, the better the effect on your potions will be.
   The *Apparatus Type* describes if the item is a *Calcinator*, *Retort*,
   *Alembic* or *Mortar & Pestle*.

Armor
   This type of item adds *Enchantment Points* to the mix. Every piece of
   clothing or armor has a "pool" of potential *Magicka* that gets unlocked
   when the player enchants it. Strong enchantments consume more magicka from
   this pool: the stronger the enchantment, the more *Enchantment Points* each
   cast will take up. *Health* means the amount of hit points this piece of
   armor has. If it sustains enough damage, the armor will be destroyed.
   Finally, *Armor Value* tells the game how much points to add to the player
   character’s *Armor Rating*.

Book
   This includes scrolls and notes. For the game to make the distinction
   between books and scrolls, an extra property, *Scroll*, has been added.
   Under the *Skill* column a scroll or book can have an in-game skill listed.
   Reading this item will raise the player’s level in that specific skill.

Clothing
   These items work just like armors, but confer no protective properties.
   Rather than *Armor Type*, these items have a *Clothing Type*.

Container
   This is all the stuff that stores items, from chests to sacks to plants. Its
   *Capacity* shows how much stuff you can put in the container. You can
   compare it to the maximum allowed load a player character can carry. A
   container, however, will just refuse to take the item in question when it
   gets "over-encumbered". Organic Containers are containers such as plants.
   Containers that respawn are not safe to store stuff in. After a certain
   amount of time, they will reset to their default contents, meaning that
   everything in them is gone forever.

Creature
   These can be monsters, animals and the like.

Creature Levelled List
   A list of creatures that can periodically spawn at a particular location.
   The strength, type, and number of creatures depends on the player's level.
   The list accepts entries of individual Creatures or also of other Creature
   Levelled Lists. Each entry has a corresponding level value that determines
   the lowest player level the creature can be spawned at.

   When ``Calculate all levels <= player`` is enabled, all the creatures with
   a level value lower or equal to the player can be spawned. Otherwise,
   the creature levelled list will only resolve a creature that matches
   the player's level

   ``Chance None`` is the percentage of possibility that no creatures will spawn.

Door
   Objects in the environment that can be opened or closed when activated. Can
   also be locked and trapped.

Ingredient
   Objects required to create potions in an apparatus. Each ingredient can hold
   up to four alchemical effects.

Item Levelled List   
   A list of items that can spawn in a container when the player opens it.
   The quality, type, and number of spawned items depends on the player's level.
   The list accepts entries of individual items or also of other Item
   Levelled Lists. Each entry has a corresponding level value that determines
   the lowest player level the item can be spawned at.

   When ``Calculate all levels <= player`` is enabled, all the items with
   a level value lower or equal to the player can be spawned. Otherwise,
   the item levelled list will only resolve an item that matches the
   player's level.

   ``Chance None`` is the percentage of possibility that no creatures will spawn. 

Light
   An object that illuminates the environment around itself, depending on the
   light's strength, range, and colour. Can be a static object in the environment,
   or when configured, can be picked up and carried by NPCs.

Lockpick
   Tool required to open locks without having the proper key or using a spell.
   Locks are found on various in-game objects, usually doors, cabinets, drawers,
   chests, and so on. 

Miscellaneous
   This is a category of objects with various in-game functions.
   
   * Soul Gems, used to hold trapped souls and enchant items.
   * Gold piles that add their value directly to the player's gold amount when picked up.
   * Keys to open locked doors.
   * Certain quest items.
   * Environment props such as plates, bowls, pots, tankards, and so on. Unlike environment objects of the Static type, these can be picked up.

NPC
   Player character and non-player characters. Their attributes and skills
   follow the game's character system, and they are made from multiple body parts.

Potion
   This is a potion which is not self-made. It has an Icon for your inventory,
   weight, coin value, and an attribute called *Auto Calc* set to ``False``.
   This means that the effects of this potion are pre-configured. This does not
   happen when the player makes their own potion.

Probe
   Tool required to disarm trapped doors, chests, or any other object that has
   a trap assigned.

Repair
   Tool required by the player to repair damaged objects of Armor and Weapon types.

Static
   Objects from which the world is made. Walls, furniture, foliage, statues,
   signs, rocks and rock formations, etc.

Weapon
   An object which the player or NPCs can carry and use it to deal damage
   to their opponents in combat. Swords, spears, axes, maces, staves, bows,
   crossbows, ammunition, and so on.
