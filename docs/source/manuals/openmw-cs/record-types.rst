Record Types
############

A game world contains many items, such as chests, weapons and monsters. All
these items are merely instances of templates we call *Objects*. The OpenMW CS
*Objects* table contains information about each of these template objects, such
as its value and weight in the case of items, or an aggression level in the
case of NPCs.

The following is a list of all Record Types and what you can tell OpenMW CS
about each of them.

Activator
   Activators can have a script attached to them. As long as the cell this
   object is in is active the script will be run once per frame.

Potion
   This is a potion which is not self-made. It has an Icon for your inventory,
   weight, coin value, and an attribute called *Auto Calc* set to ``False``.
   This means that the effects of this potion are pre-configured. This does not
   happen when the player makes their own potion.

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
   amount of time they will reset to their default contents, meaning that
   everything in them is gone forever.

Creature
   These can be monsters, animals and the like.

