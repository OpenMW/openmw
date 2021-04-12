##############################
Blender to OpenMW with Collada
##############################

First, let's take a look at the pipeline requirements and how the fundamental properties of a scene translate from Blender to OpenMW.

Requirements
------------
* `OpenMW 0.47 <https://openmw.org/downloads/>`_ or later
* `Blender 2.81 <https://www.blender.org/download/>`_ or later. Latest confirmed, working version is Blender 2.91
* `Better COLLADA Exporter <https://github.com/unelsson/collada-exporter>`_ tuned for OpenMW
* A model you would like to export

In addition, OpenMW needs to be configured to read COLLADA (dae) files instead of the default format (nif). In settings.cfg under [Models] section... TODO

Location
--------

Objects keep their visual location and origin they had in the original scene.


Rotation
--------

* Blender’s +Z axis is up axis in OpenMW
* Blender’s +Y axis is front axis in OpenMW
* Blender’s X axis is left-right axis in OpenMW


Scale
-----

Scale ratio between Blender and OpenMW is 70 to 1. This means 70 blender units translate to 1 m in OpenMW.

However, a scale factor like this is impractical to work with. A better approach is to work with a scale of 1 Blender unit = 1m and apply the 70 scale factor in the Better COLLADA Exporter. The exporter will automatically scale all object, mesh, armature and animation data.


Exporter settings - static models
---------------------------------

Better COLLADA Exporter offers various options which are rather straightforward for static models. The important one is last in the list, to apply a scaling factor of 70 to the whole scene, so 1 blender unit equals 1 m in OpenMW. The following settings should be good for general use.
It's also very important to have "export selected" box checked, as otherwise the exporter may just fail with an error message. It's also important to have the correct window open, and the models selected before exporting.


Animated models to OpenMW
-------------------------

Animated models are those where a hierarchy of bones, known as armature, deforms the mesh and makes things move. Besides the topics covered above, the following requirements apply.

Armature
--------

* For animated models, a single armature per COLLADA file is advised to avoid any potential problems.
* There needs to be a single top-most bone in the armature’s hierarchy, where both the deformation and control bones fall under it.
* Not all bones need to be exported. By disabing the bone’s “Deform” property and using the corresponding option in the exporter, it is possible to export only the bones needed for animation.


Animations
----------

Every action in Blender is exported as its own animation clip in COLLADA. Actions you don't wish to export need to have "-noexp" added to their name, with the corresponding option enabled in the exporter.

Due to current limitations of the format / exporter, the keyframes of any action must not overlap the keyframes of any other action. Thus in practice, the keyframes for each action need to be manually offset to their unique range on the timeline.

An animated .dae file needs a corresponding animation definition file, or textkeys, for OpenMW to understand. Textkeys are set in .txt file with the same name as the model. E.g. OpenMWDude.dae -> OpenMWDude.txt , each line having a textkey and a double number for timesignature. E.g. idle: start 0.03333333333333333.

Root Motion
-----------

OpenMW can read the movement of the root (top-most) bone and use it to move objects in the game world. For this to work, the root bone must be animated to move through space. The root bone must, in its default pose, be alligned with the world.


Exporter Settings
-----------------

For animated models, use the following exporter settings. Before export, select all objects you wish to include in the exported file. TODO





 
 

