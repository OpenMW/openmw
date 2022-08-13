############
World Tables
############

These are the tables in the World menu category. The contents of the game world 
can be changed by choosing one of the options in the appropriate menu at the top 
of the screen.


Objects
*******

This is a library of all the items, triggers, containers, NPCs, etc. in the game.
There are several kinds of Record Types. Depending on which type a record 
is, it will need specific information to function. For example, an NPC needs a 
value attached to its aggression level. A chest, of course, does not. All Record 
Types contain at least a 3D model or else the player would not see them. Usually 
they also have a *Name*, which is what the players sees when they hover their 
crosshair over the object during the game.

Please refer to the :doc:`record-types` chapter for an overview of what each
object type represents in the game's world.


Instances
*********

An instance is created every time an object is placed into a cell. While the 
object defines its own fundamental properties, an instance defines how and where 
this object appears in the world. When the object is modified, all of its 
instances will be modified as well.

Cell
    Which cell contains this instance. Is assigned automatically based on the 
    edit you make in the 3D view.

Original Cell
    If an object has been moved in-game this field keeps a track of the original 
    cell as defined through the editor. Is assigned automatically based on the edit 
    you make in the 3D view. 

Object ID
    ID of the object from which this instance is created.
    
Pos X, Y, Z
    Position coordinates in 3D space relative to the parent cell.

Rot X, Y, Z
    Rotation in 3D space.

Scale
    Size factor applied to this instance. It scales the instance uniformly on 
    all axes.

Owner
    NPC the instance belongs to. Picking up the instance by the player is 
    regarded as stealing.

Soul
    This field takes the object of a *Creature* type. Option applies only to 
    soul gems which will contain the creature's soul and allow enchanting. 
    
Faction
    Faction the instance belongs to. Picking up the instance without joining 
    this faction is regarded as stealing.
    
Faction Index
    The player's required rank in a faction to pick up this instance without it
    seen as stealing. It allows a reward mechanic where the higher the player
    is in a faction, the more of its items and resources are freely
    available for use.
    
Charges
    How many times can this item be used. Applies to lockpicks, probes, and 
    repair items. Typically used to add a "used" version of the object to the
    in-game world.
    
Enchantment
    Doesn't appear to do anything for instances. An identical field for Objects 
    takes an ID of an enchantment.
    
Coin Value
    This works only for instances created from objects with IDs ``gold_001``, 
    ``gold_005``, ``gold_010``, ``gold_025``, and ``gold_100``. Coin Value tells how 
    much gold is added to player's inventory when this instance is picked up. The 
    names and corresponding functionality are hardcoded into the engine.
    
    For all other instances this value does nothing and their price when buying 
    or selling is determined by the Coin Value of their Object.    
    
Teleport
    When enabled, this instance acts as a teleport to other locations in the world.
    Teleportation occurs when the player activates the instance.

Teleport Cell
    Destination cell where the player will appear.

Teleport Pos X, Y, Z
    Location coordinates where the player will appear relative to the 
    destination cell.

Teleport Rot X, Y, Z
    Initial orientation of the player after being teleported. 

Lock Level
    Is there a lock on this instance and how difficult it is to pick.
    
Key
    Which key is needed to unlock the lock on this instance.

Trap
    What spell will be cast on the player if the trap is triggered. The spell
    has an on touch magic effect.

Owner Global
    A global variable that lets you override ownership. This is used in original 
    Morrowind to make beds rentable.


Cells
*****

Cells are the basic world-building units that together make up the game's world. 
Each of these basic building blocks is a container for other objects to exist in.
Dividing an expansive world into smaller units is neccessary to be able to 
efficiently render and process it. Cells can be one of two types:
    
Exterior cells
    These represent the outside world. They all fit on a grid where cells have 
    unique coordinates and border one another. Each exterior cell contains a part of 
    the terrain and together they form a seamless, continuous landmass. Entering and 
    leaving these cells is as simple as walking beyond their boundary after which we 
    enter its neighbouring cell. It is also possible to move into another interior 
    or exterior cell through door objects.

Interior cells
    These represent enclosed spaces such as houses, dungeons, mines, etc. They 
    don't have a terrain, instead their whole environment is made from objects. 
    Interior cells only load when the player is in them. Entering and leaving these 
    cells is possible through door objects or teleportation abilities.

The Cells table provides you with a list of cells in the game and exposes 
their various parameters to edit.

Sleep Forbidden
   In most cities it is forbidden to sleep outside. Sleeping in the wilderness
   carries its own risks of attack, though. This entry lets you decide if a
   player should be allowed to sleep on the floor in this cell or not.

Interior Water
   Setting the cell’s Interior Water to ``true`` tells the game that there needs
   to be water at height 0 in this cell. This is useful for dungeons or mines
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

Interior
    When enabled, it allows to manually set *Ambient*, *Sunlight*, *Fog*, 
    and *Fog Density* values regardless of the main sky system.
    
Ambient
    Colour of the secondary light, that contributes to an overall shading of the 
    scene.
    
Sunlight
    Colour of the primary light that lights the scene.
    
Fog
    Colour of the distant fog effect.
    
Fog Density
    How quickly do objects start fading into the fog.

Water Level
    Height of the water plane. Only applies to interior cells
    when *Interior Water* is enabled.
    
Map Color
    This is a property present in Morrowind, but is not actually used.
    It doesn’t do anything in OpenMW either.


Lands
*****

Lands are records needed by exterior cells to show the terrain. Each exterior 
cell needs its own land record and they are paired by matching IDs. Land records
can be created manually in this table, but a better approach is to simply shape
the terrain in the 3D view and the land record of affected cells will be
created automatically.


Land Textures
*************

This is a list of textures that are specifically used to paint the terrain of 
exterior cells. By default, the terrain shows the ``_land_default.dds`` texture 
found in ``data/textures`` folder. Land texture entries can be added, edited or
removed.

Texture Nickname
    Name of this land texture.

Texture Index
    Assigned automatically and cannot be edited.
    
Texture
    Texture image file that is used for this land texture.


Pathgrids
*********

Pathgrids allow NPCs to navigate and move along complicated paths in their surroundings.
A pathgrid contains a list of *points* connected by *edges*. NPCs will
find their way from one point to another as long as there is a path of 
connecting edges between them. One pathgrid is used per cell.

When recast navigation is enabled pathgrids are still used and complement 
navigation meshes. Pathgrids help where original Morrowind content is not 
suitable for navigation mesh generation. In addition, the off-mesh connections 
generated from pathgrids are important for NPC AiWander package.


Regions
*******

Regions describe general areas of the exterior game world and define rules for 
random enemy encounters, ambient sounds, and weather. Regions can be assigned 
one per cell and the cells will inherit their rules.

Name
   This is how the game will show the player's location in-game.

MapColour
   This is a colour used to identify the region when viewed in *World* → *Region Map*.

Sleep Encounter
   This field takes an object of the *Creature Levelled List* type. This object 
   defines what kinds of enemies the player might encounter when sleeping outside 
   in the wilderness.

Weather
    A table listing all available weather types and their chance to occur while 
    the player is in this region. Entries cannot be added or removed.

Sounds
    A table listing ambient sounds that will randomly play while the player is 
    in this region. Entries can be freely added or removed.


Region Map
**********

The region map shows a grid of exterior cells, their relative positions to one 
another, and regions they belong to. In summary, it shows the world map. 
Compared to the cells table which is a list, this view helps vizualize the world.
Region map does not show interior cells.
