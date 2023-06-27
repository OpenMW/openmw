Doors and Connecting Cells
##########################

Cells are the basic world-building units in OpenMW and can be of either exterior 
or interior type. There are two ways players can go from one cell to another.

1. The common method is by using Door objects to instantly change player's destination cell and player's position coordinates. These need to be set-up manually in OpenMW-CS and are the topic of this article.
2. The other method is between exterior cells and is as simple as walking beyond their boundaries after which the player enters the neighbouring cell. This functionality is handled automatically by the engine and doesn't require any special setup.


Setting up Doors in OpenMW-CS
=============================

First, we need an object of a Door type, as well as:

- An associated model, to be rendered in the game world
- A name
- Sounds for when the door is opened or closed. Locked sound effect is defined in a different part of OpenMW-CS with more information available in :doc:`sound-effects`.

.. figure:: https://gitlab.com/OpenMW/openmw-docs/-/raw/master/docs/source/reference/modding/_static/door-connect-cell-object.webp
    :align: center
    
    An object of type Door.


This object is then placed in a cell and the instance of the object takes 
additional parameters defining where the door will lead. 

.. figure:: https://gitlab.com/OpenMW/openmw-docs/-/raw/master/docs/source/reference/modding/_static/door-connect-cell-instance.webp
    :align: center
    
    Instance of this object placed in the world then defines the teleport destination.


To interior cells
*****************

When a door instance leads to an interior cell:

* ``Teleport Cell`` requires ID of the destination **interior** cell.
* ``Teleport Pos X``, ``Teleport Pos Y``, and ``Teleport Pos Z`` define where relative to the cell the player will appear.
* ``Teleport Rot Z`` defines the direction the player will face.
* ``Teleport Rot X`` and ``Teleport Rot Y`` are not required as they don't do anything.


To exterior cells
*****************

When the door instance leads to an exterior cell:

* ``Teleport Cell`` **MUST** be left empty otherwise OpenMW will throw an error. This is despite the fact OpenMW-CS can suggest to input exterior cells into this field.
* ``Teleport Pos X``, ``Teleport Pos Y``, and ``Teleport Pos Z`` destination coordinates are not relative to a cell but are global to the exterior game world.
* ``Teleport Rot Z`` defines the direction the player will face.
* ``Teleport Rot X`` and ``Teleport Rot Y`` are not required as they don't do anything.
