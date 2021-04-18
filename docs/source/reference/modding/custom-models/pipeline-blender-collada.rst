##############################
Blender to OpenMW with COLLADA
##############################

Requirements
************
To use the Blender to OpenMW pipeline via COLLADA, you will need the following.

* `OpenMW 0.47 <https://openmw.org/downloads/>`_ or later
* `Blender 2.81 <https://www.blender.org/download/>`_ or later. Latest confirmed, working version is Blender 2.91
* `Better COLLADA Exporter <https://github.com/unelsson/collada-exporter>`_ tuned for OpenMW
* A model you would like to export


Static Models
*************
Static models are those that don't have any animations included in the exported file. First, let's take a look at how the fundamental properties of a scene in Blender translate to a COLLADA model suitable for use in OpenMW. These apply the same to static and animated models.

Location
========

Objects keep their visual location and origin they had in the original scene.

Rotation
========

* Blender’s +Z axis is up axis in OpenMW
* Blender’s +Y axis is front axis in OpenMW
* Blender’s X axis is left-right axis in OpenMW

Scale
=====

Scale ratio between Blender and OpenMW is 70 to 1. This means 70 units in Blender translate to 1 m in OpenMW.

However, a scale factor like this is impractical to work with. A better approach is to work with a scale of 1 Blender unit = 1 m and apply the 70 scale factor at export. The exporter will automatically scale all object, mesh, armature and animation data.

Materials
=========

OpenMW uses the classic, specular material setup and currently doesn't use the more mainstream `PBR <https://en.wikipedia.org/wiki/Physically_based_rendering>`_ way. In Blender, the mesh needs a default material with a diffuse texture connected to the ``Base Color`` socket. This is enough for the material to be included in the exported COLLADA file.

Additional texture types, such as specular or normal maps, will be automatically recognized and used by OpenMW. They need an identical base name as the diffuse texture, a suffix, and be in the same folder. How to enable this and what suffix is used for what texture type is covered in more detail in :doc:`../../modding/texture-modding/texture-basics`.

Collision Shapes
================

In Blender, create an empty and name it ``collision``. Any mesh that is a child of this empty will be used for physics collision and will not be visible. There can be multiple child meshes under ``collision`` and they will all contribute to the collision shapes. The meshes themselves can have an arbitrary name, it's only the name of the empty that is important. The ``tcb`` command in OpenMW's in-game console will make the collision shapes visible and you will be able to inspect them.

Exporter Settings
=================

For static models, use the following exporter settings. Before export, select all objects you wish to include in the exported file and have the "Selected Objects" option enabled. Without this, the exporter could fail.


.. image:: https://gitlab.com/OpenMW/openmw-docs/-/raw/master/docs/source/reference/modding/custom-models/_static/dae_exporter_static.jpg
    :align: center


Animated Models
***************

Animated models are those where a hierarchy of bones, known as armature, deforms the mesh and makes things move. Besides the topics covered above, the following requirements apply.

Armature
========

* A single armature per COLLADA file is advised to avoid any potential problems.
* There needs to be a single top-most bone in the armature’s hierarchy, where both the deformation and control bones fall under it.
* Not all bones need to be exported. By disabing the bone’s “Deform” property and using the corresponding option in the exporter, it is possible to export only the bones needed for animation.


Animations
==========

Every action in Blender is exported as its own animation clip in COLLADA. Actions you don't wish to export need to have "-noexp" added to their name, with the corresponding option enabled in the exporter.

Due to current limitations of the format and exporter, the keyframes of individual actions must not overlap with other actions. The keyframes need to be manually offset to a unique range on the timeline as shown in this example.

.. image:: https://gitlab.com/OpenMW/openmw-docs/-/raw/master/docs/source/reference/modding/custom-models/_static/dae_animations_on_timeline.jpg
    :align: center

Textkeys
--------

The exported COLLADA file requires a corresponding textkeys file for OpenMW to properly read the animations. Textkeys is a .txt file containing animation definitions. Textkeys file is placed in the same folder as the model and uses a name matching the model. 

    - ``OpenMWDude.dae``
    - ``OpenMWDude.txt``

Textkeys use a simple format as shown in the example. Name, start and stop values can be taken from the corresponding COLLADA file for each ``<animation_clip>``.
    
.. code::

    idle: start 0.03333333333333333
    idle: stop 2.033333333333333
    runforward: start 2.0666666666666664
    runforward: stop 3.0666666666666664
    runback: start 3.1
    runback: stop 4.1
    ...
    

Root Motion
===========

OpenMW can read the movement of the root (top-most in hierarchy) bone and use it to move objects in the game world. For this to work, the root bone must be animated to move through space. The root bone must, in its default pose, be alligned with the world.

.. image:: https://gitlab.com/OpenMW/openmw-docs/-/raw/master/docs/source/reference/modding/custom-models/_static/dae_rig_root.jpg
    :align: center


Exporter Settings
=================

For animated models, use the following exporter settings. Before export, select all objects you wish to include in the exported file and have the "Selected Objects" option enabled. Without this, the exporter could fail.

.. image:: https://gitlab.com/OpenMW/openmw-docs/-/raw/master/docs/source/reference/modding/custom-models/_static/dae_exporter_animated.jpg
    :align: center





 
 

