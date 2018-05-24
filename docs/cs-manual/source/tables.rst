Tables
######

If you have launched OpenMW CS already and played around with it for a bit, you
will have noticed that the interface is made entirely of tables. This does not
mean it works just like a spreadsheet application though, it would be more
accurate to think of databases instead. Due to the vast amounts of information
involved with Morrowind tables made the most sense. You have to be able to spot
information quickly and be able to change them on the fly.


Used Terms
**********

Record
   An entry in OpenMW CS representing an item, location, sound, NPC or anything
   else.

Instance, Object
   When an item is placed in the world, it does not create a whole new record
   each time, but an *instance* of the *object*.
   
   For example, the game world might contain a lot of exquisite belts on
   different NPCs and in many crates, but they all refer to one specific
   instance: the Exquisite Belt record. In this case, all those belts in crates
   and on NPCs are instances. The central Exquisite Belt instance is called an
   *object*. This allows modders to make changes to all items of the same type
   in one place.
   
   If you wanted all exquisite belts to have 4000 enchantment points rather
   than 400, you would only need to change the object Exquisite Belt rather
   than all exquisite belt instances individually.

Some columns are recurring throughout OpenMW CS, they show up in (nearly) every
table.

ID
   Each item, location, sound, etc. gets the same unique identifier in both
   OpenMW CS and Morrowind. This is usually a very self-explanatory name. For
   example, the ID for the (unique) black pants of Caius Cosades is
   ``Caius_pants``. This allows players to manipulate the game in many ways.
   For example, they could add these pants to their inventory by opening the
   console and entering: ``player- >addItem Caius_pants``. In both Morrowind
   and OpenMW CS the ID is the primary way to identify all these different
   parts of the game.

Modified
   This column shows what has happened (if anything) to this record. There are
   four possible states in which it can exist:

   Base
      The record is unmodified and from a content file other than the one
      currently being edited.

   Added
      This record has been added in the currently content file.

   Modified
      Similar to *base*, but has been changed in some way.

   Deleted
      Similar to *base*, but has been removed as an entry. This does not mean,
      however, that the occurrences in the game itself have been removed! For
      example, if you were to remove the ``CharGen_Bed`` entry from
      ``morrowind.esm``, it does not mean the bedroll in the basement of the
      Census and Excise Office in Seyda Neen will be gone. You will have to
      delete that instance yourself or make sure that that object is replaced
      by something that still exists otherwise the player will get crashes in
      the worst case scenario.



World Screens
*************

The contents of the game world can be changed by choosing one of the options in
the appropriate menu at the top of the screen.


Regions
=======

This describes the general areas of Vvardenfell. Each of these areas has
different rules about things such as encounters and weather.

Name
   This is how the game will show the player's location in-game.

MapColour
   This is a six-digit hexadecimal representation of the colour used to
   identify the region on the map available in *World* → *Region Map*.

Sleep Encounter
   These are the rules for what kinds of enemies the player might encounter
   when sleeping outside in the wilderness.


Cells
=====

Expansive worlds such as Vvardenfell, with all its items, NPCs, etc. have a lot
going on simultaneously. But if the player is in Balmora, why would the
computer need to keep track the exact locations of NPCs walking through the
corridors in a Vivec canton? All that work would be quite useless and bring
the player's system down to its knees! So the world has been divided up into
squares we call *cells*.  Once your character enters a cell, the game will load
everything that is going on in that cell so the player can interact with it.

In the original Morrowind this could be seen when a small loading bar would
appear near the bottom of the screen while travelling; the player had just
entered a new cell and the game had to load all the items and NPCs. The *Cells*
screen in OpenMW CS provides you with a list of cells in the game, both the
interior cells (houses, dungeons, mines, etc.) and the exterior cells (the
outside world).

Sleep Forbidden
   Can the player sleep on the floor? In most cities it is forbidden to sleep
   outside. Sleeping in the wilderness carries its own risks of attack, though,
   and this entry lets you decide if a player should be allowed to sleep on the
   floor in this cell or not.

Interior Water
   Should water be rendered in this interior cell? The game world consists of
   an endless ocean at height 0, then the landscape is added. If part of the
   landscape goes below height 0, the player will see water.

   Setting the cell’s Interior Water to true tells the game that this cell that
   there needs to be water at height 0. This is useful for dungeons or mines
   that have water in them.

   Setting the cell’s Interior Water to ``false`` tells the game that the water
   at height 0 should not be used. This flag is useless for outside cells.

Interior Sky
   Should this interior cell have a sky? This is a rather unique case. The
   Tribunal expansion took place in a city on the mainland. Normally this would
   require the city to be composed of exterior cells so it has a sky, weather
   and the like. But if the player is in an exterior cell and were to look at
   their in-game map, they would see Vvardenfell with an overview of all
   exterior cells. The player would have to see the city’s very own map, as if
   they were walking around in an interior cell.
   
   So the developers decided to create a workaround and take a bit of both: The
   whole city would technically work exactly like an interior cell, but it
   would need a sky as if it was an exterior cell. That is what this is. This
   is why the vast majority of the cells you will find in this screen will have
   this option set to false: It is only meant for these "fake exteriors".

Region
   To which Region does this cell belong? This has an impact on the way the
   game handles weather and encounters in this area. It is also possible for a
   cell not to belong to any region.


Objects
=======

This is a library of all the items, triggers, containers, NPCs, etc. in the
game. There are several kinds of Record Types. Depending on which type a record
is, it will need specific information to function. For example, an NPC needs a
value attached to its aggression level. A chest, of course, does not. All
Record Types contain at least a 3D model or else the player would not see them.
Usually they also have a *Name*, which is what the players sees when they hover
their reticle over the object during the game.

Please refer to the Record Types chapter for an overview of what each type of
object does and what you can tell OpenMW CS about these objects.

