#############################
Animated Creature via COLLADA
#############################

This tutorial shows how to get an animated creature from Blender to OpenMW using 
the COLLADA format. It does not cover using Blender itself, as there are many 
better resources for that. The focus is solely on the animation pipeline and its 
specific requirements. Most of them are related to how the model, rig, and 
animations are set up in Blender.

.. note::
    This tutorial builds upon the :doc:`pipeline-blender-collada-static-models` tutorial. All fundamentals of exporting static objects apply to animated ones as well.

Requirements
************

To use the Blender to OpenMW pipeline via COLLADA, you will need the following.

* `OpenMW 0.48 <https://openmw.org/downloads/>`_ or later
* `Blender 2.83 <https://www.blender.org/download/>`_ or later. Latest confirmed, working version is Blender 3.0
* `OpenMW COLLADA Exporter <https://github.com/openmw/collada-exporter>`_
* An animated model you would like to export. In our case the flamboyant Land Racer!

The Land Racer
**************

The creature, and its revelant files, are available from the `Example Suite repository <https://gitlab.com/OpenMW/example-suite/-/tree/master/example_animated_creature>`_.
This should be useful for further study of how to set things up in case this 
tutorial is not clear on any particular thing.

* ``data/meshes/land_racer.dae`` – exported model
* ``data/meshes/land_racer.txt`` – animation textkeys
* ``data/textures/land_racer.dds`` – diffuse texture
* ``data/textures/land_racer_n.dds`` - normal map
* ``source_assets/land_racer.blend`` – source file configured as this tutorial specifies

Model
*****

The model needs to be a child of the rig and have an Armature modifier asigned. 
Bone weights are limited to a maximum of 4 bones per vertex. The model needs to 
have default location, rotation, and scale.

.. image:: https://gitlab.com/OpenMW/openmw-docs/-/raw/master/docs/source/reference/modding/custom-models/_static/landracer-in-blender.jpg
    :align: center


Collision Shape
***************

Collision is set up with an empty named ``Collision`` or ``collision`` with a 
single child mesh. OpenMW will use the bounding box of this mesh for physics 
collision shape so a simple, cuboid mesh is enough. If no collision shape is 
defined, the bounding box of the animated model will be used instead.

.. image:: https://gitlab.com/OpenMW/openmw-docs/-/raw/master/docs/source/reference/modding/custom-models/_static/landracer-collision-shape.jpg
    :align: center


Armature / Rig
**************

.. note::
  Only a single rig object should be included in the exported file. Exporting multiple rigs is not reliably supported and can lead to errors.

Root
====

The rig needs to be structured in a specific way. There should be a single top 
bone in the rig’s hierarchy, the root bone named ``Bip01``. The name is 
required so OpenMW recognizes and uses it for root movement. For this same 
reason, the bone should be by default aligned with the world. The root bone 
needs to have its ``Deform`` flag **enabled**.

.. image:: https://gitlab.com/OpenMW/openmw-docs/-/raw/master/docs/source/reference/modding/custom-models/_static/landracer-root-bone.jpg
    :align: center


Deform Bones
============

Below the root bone, the bones are divided into two branches. One branch 
contains the deform bones which get included in the final exported file. These 
are otherwise not animated directly but inherit motion from other bones via 
constraints. They have their ``Deform`` flag **enabled**. For creatures, the 
deform bones can be named as you desire and don’t need to follow the naming 
convention used for NPC and player models.

.. image:: https://gitlab.com/OpenMW/openmw-docs/-/raw/master/docs/source/reference/modding/custom-models/_static/landracer-rig-hierarchy.jpg
    :align: center

Control Bones
=============

The other branch holds control and helper bones that enable comfortable 
animation in Blender, but are neither required nor included in the exported 
file. They have their ``Deform`` flag **disabled**. How these bones are 
structured is a big separate topic on its own that this tutorial does not cover, 
but you can study the provided source file.

  
Animations
**********

A creature in OpenMW is expected to have a set of animations to display its 
various actions. These animations are recognized and used by their name. 

.. list-table:: 
   :widths: 25 25 50
   :header-rows: 1

   * - Animation name
     - Possible variations
     - Purpose
   * - attack1
     - attack2, attack3
     - The creature performs an attack
   * - death1
     - death2 - death5
     - The creature dies while upright
   * - hit1
     - hit2 - hit5
     - The creature is hit in combat
   * - idle
     - idle2 - idle9
     - Flavour animations when the creature does nothing in particular
   * - knockout
     - /
     - When creature's fatigue goes below 0 and it staggers to the ground
   * - deathknockout
     - /
     - The creature dies while knocked out
   * - knockdown
     - /
     - When the creatures receives a heavy hit in combat or lands from a considerable height
   * - deathknockdown
     - /
     - The creature dies while knocked down 
   * - runforward
     - /
     - Moving forward fast
   * - walkforward
     - /
     - Moving forward at regular speed

Animating in Blender is done at 30 fps. Specific to how OpenMW works, each 
exported animation needs to take a unique range on the timeline. To achieve 
this, actions are placed as strips in the NLA editor with an obligatory one 
frame gap between them.

.. image:: https://gitlab.com/OpenMW/openmw-docs/-/raw/master/docs/source/reference/modding/custom-models/_static/landracer-nla-strips.jpg
    :align: center
    
NLA strips affect the exported result based on their scale, name, frame range, 
repetition, or any other factor affecting the end animation result. It's
*What you see is what you get* principle.

Root movement is required for animations such as ``walkforward`` and 
``runforward`` and is likely to work for other animations if needed.
Root movement works only when the root bone is named ``Bip01``.

Textkeys
********

The exported COLLADA file requires a corresponding textkeys file for OpenMW to 
properly read the animations. Textkeys is a ``.txt`` file containing animation 
definitions and events. At a minimum it needs to include at least animation 
``start`` and ``stop`` values in a format as shown in this example.
    
.. code::

    idle: start 0.033333
    idle: stop 2.033333
    walkforward: start 2.066667
    walkforward: stop 3.666667
    runforward: start 3.7
    runforward: stop 4.433333
    attack1: start 4.466667
    attack1: stop 5.433333

The textkeys file is placed in the same folder as the model and matches the model's name.

* ``meshes/land_racer.dae``
* ``meshes/land_racer.txt``

While it's possible to write textkeys by hand, OpenMW's COLLADA Exporter offers 
a convenient option to export them based on Blender's pose markers. **Pose markers 
are stored per action** and shouldn't be be confused with timeline markers which 
are global to the Blender file. When actions are present in the NLA Editor as 
strips, their containing pose markers will be used to write the textkeys. Any 
frame offset and scaling of the strips is taken into account and affects the 
final textkey values. Be sure to have ``Export Textkeys`` option enabled in
the exporter.


.. image:: https://gitlab.com/OpenMW/openmw-docs/-/raw/master/docs/source/reference/modding/custom-models/_static/landracer-textkey-markers.jpg
    :align: center

In the example of ``attack1`` action, it needs to contain pose markers named 
``attack1: start`` at **frame 1** and ``attack1: stop`` at **frame 30**.


Exporter Settings
*****************

For animated models, use the following exporter settings. Before export, select 
all objects you wish to include in the exported file and have the ``Selected 
Objects`` option enabled. Without this, the exporter could fail.

.. image:: https://gitlab.com/OpenMW/openmw-docs/-/raw/master/docs/source/reference/modding/custom-models/_static/landracer-exporter-settings.jpg
    :align: center


Getting the Model In-game
*************************

Once the Land Racer is exported, both of its ``.dae`` and ``.txt`` files need to 
be placed in the correct folder where OpenMW will read it. Afterwards in 
OpenMW-CS, it should be visible in the Assets->Meshes table and can be assigned 
to the ``Model/Animation`` field of a creature.

.. image:: https://gitlab.com/OpenMW/openmw-docs/-/raw/master/docs/source/reference/modding/custom-models/_static/landracer-in-openmwcs.jpg
    :align: center

