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
