################
Mechanics Tables
################

Tables that belong into the mechanics category.


Scripts
*******

Scripts are useful to expand the base functionality offered by the engine
or to create complex quest lines. Entries can be freely added or removed
through OpenMW-CS. When creating a new script, it can be defined as local
or global.


Start Scripts
*************

A list of scripts that are automatically started as global scripts on game startup.
The script ``main`` is an exception, because it will be automatically started
even without being in the start script list.


Global Variables
****************

Global variables are needed to keep track of the the game's state. They can be 
accessed from anywhere in the game (in contrast to local variables) and can be 
altered at runtime (in contrast to GMST records). Some example of global 
variables are current day, month and year, rats killed, player's crime penalty, 
is player a werewolf, is player a vampire, and so on.


Game Settings (GMST)
********************

GMST are variables needed throughout the game. They can be either a float,
integer, boolean, or string. Float and integer variables affect all sorts of
behaviours of the game. String entries are the text that appears in the
user interface, dialogues, tooltips, buttons and so on. GMST records cannot
be altered at runtime.


Spells
******

Spells are combinations of magic effects with additional properties and are used 
in different ways depending on their type. Some spells are the usual abracadabra 
that characters cast. Other spells define immunities, diseases, modify 
attributes, or give special abilities. Entries in this table can be freely 
added, edited, or removed through the editor.

Name
    Name of the spell that will appear in the user interface.
   
Spell Type
    * Ability - Constant effect which does not need to be cast. Commonly used for racial or birthsign bonuses to attributes and skills.
    * Blight - Can be contracted in-game and treated with blight disease cures (common disease cures will not work). Applies a constant effect to the recepient. 
    * Curse
    * Disease - Can be contracted in-game and treated with common disease cures. Applies a constant effect to the recepient. 
    * Power - May be cast once per day at no magicka cost. Usually a racial or birthsign bonus.
    * Spell - Can be cast and costs magicka. The chance to successfully cast a spell depends on the caster's skill.
  
Cost
    The amount of magicka spent when casting this spell.

Auto Calc
    Automatically calculate the spell's magicka cost.

Starter Spell
    Starting spells are added to the player on character creation when certain 
    criteria are fulfilled. The player must be able to cast spells, there is a 
    certain chance for that spell to be added, and there is a maximum number
    of starter spells the player can have.


Always Succeeds
    When enabled, it will ensure this spell will always be cast regardless of
    the caster's skill.

Effects
    A table containing magic effects of this spell and their properties.
    New entries can be added and removed through the right click menu.


Enchantments
************

Enchantments are a way for magic effects to be assigned to in-game items.
Each enchantment can hold multiple magic effects along with other properties.
Entries can be freely added or removed through the editor.

Enchantment Type
    The way this enchantment is triggered.

    * Cast once - the enchantment is cast like a regular spell and afterwards the item is gone. Used to make scrolls. 
    * Constant effect - the effects of the enchantment will always apply as long as the enchanted item is equiped.
    * When Strikes - the effect will apply to whatever is hit by the weapon with the enchantment.
    * When Used - the enchantment is cast like a regular spell. Instead of spending character's magicka, it uses the item's available charges.

Cost
    How many points from the available charges are spent each time the
    enchantment is used. In-game the cost will also depend on character's
    enchanting skill.

Charges
    Total supply of points needed to use the enchantment. When there are
    less charges than the cost, the enchantment cannot be used and
    the item needs to be refilled.

Auto Calc
    Automatically calculate the enchantment's cost to cast.

Effects
    A table containing magic effects of this enchantment and their properties. 
    New entries can be added and removed through the right click menu.


Magic Efects
************

Magic effects define how in-game entities are affected when using magic.
They are required in spells, enchantments, and potions. The core gameplay
functionality of these effects is hardcoded and it's not possible to add
new entries through the editor. The existing entries can nonetheless have
their various parameters adjusted.

School
    Category this magic effect belongs to.
    
Base Cost
    Used when automatically calculating the spell's cost with "Auto Calc" feature.

Icon
    Which icon will be displayed in the user interface for this effect. Can only 
    insert records available from the icons table.
     
Particle
    Texture used by the particle system of this magic effect.
    
Casting Object
    Which object is displayed when this magic effect is cast.
        
Hit Object
    Which object is displayed when this magic effect hits a target.
    
Area Object
    Which object is displayed when this magic effect affects an area.
      
Bolt Object
    Which object is displayed as the projectile for this magic effect.
    
Casting Sound
    Sound played when this magic effect is cast.
      
Hit Sound
    Sound played when this magic effect hits a target.
       
Area Sound
    Sound played when this magic effect affects an area.
    
Bolt Sound
    Sound played by this magic effect's projectile.
      
Allow Spellmaking
    When enabled, this magic effect can be used to create spells.
        
Allow Enchanting
    When enabled, this magic effect can be used to create enchantments.
        
Negative Light
    This is a flag present in Morrowind, but is not actually used.
    It doesn’t do anything in OpenMW either.
      
Description
    Flavour text that appears in the user interface.
