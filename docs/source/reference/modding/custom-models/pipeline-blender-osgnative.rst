#################################
Blender to OpenMW with OSG native
#################################

This article explains how to export a model from Blender to OpenMW using the OSG model format. It supports only basic, static models.
For more details on the format, refer to `this forum post <https://forum.openmw.org/viewtopic.php?f=20&t=2949&p=35514#p35514>`_.

Prerequisites
#############

- OpenMW 0.38 or later
- Blender 2.60 or later
- OSG exporter add-on for Blender
- A Blender model you would like to export

Installing the exporter
#######################

#.	Download the
	`OSG export script
	<https://github.com/openmw/osgexport/blob/release/blender-2.5/build/osgexport-0.14.2.zip?raw=true>`_
#.	Open Blender and go to File -> User Preferences
#.	Select Add-ons -> Install from File, then select the downloaded `.zip`
#.	Enter "osg" into the search bar, then tick the checkbox next to the add-on to enable it
#.	Now click Save user setting so the exporter stays enabled when you re-start Blender

You can now export your models using the OSG model (osgt) entry in the File -> Export menu.

Model's location
################

The model needs to be at 0,0,0 coordinates in Blender,
as this is where its origin will always be when exported.
If the model is offset from 0,0,0 in Blender,
it will be offset from its origin in the exported file as well.

Model's rotation
################

- Blender's Z axis is up axis in OpenMW
- Blender's Y axis is front axis in OpenMW
- Blender's X axis is left-right axis in OpenMW
- Visual rotation is taken into account when exporting

Model's scale
#############

Blender:OpenMW model scale is 70:1,
which means 70 Blender units (BU) translate into 1m in OpenMW.
Using this scale, any models you make will fit with the existing ones.
The scale does not need to be applied,
the exporter will always use the visual scale of the model.
However, 70 is an odd number to work with so here's an alternative workflow:

-	In Blender, use a scale of 1BU = 1m which is a nice scale and ratio to work with.
	Have all models use this scale.
-	Before exporting a model, scale it up by a factor of 70.
-	After exporting, undo the model's scale change and continue working as normal
	(in the future a preferable way would be to apply the scale through the exporter)

Putting the model in-game
#########################

Place the exported model in the Meshes sub-folder of a data folder recognized by the game,
e.g. the Morrowind Data Files folder, or the local data folder.
Place all required textures in the Textures sub-folder.
Now start OpenMW-CS, create a new addon file
and you should see your mesh in the Assets -> Meshes table.
Go ahead and make some object use your mesh.
You can now preview by right clicking on the object -> Preview to see what the mesh will look like.

Converting the model to binary
##############################

When the model behaves to our liking,
we can think about converting it from the "osgt" text format to the "osgb" binary format
so that it's smaller in size - and thus faster to load.

To do this, simply invoke the osgconv tool.
This tool should be included with your distribution of OpensceneGraph.

`osgconv -O WriteImageHint=UseExternal model.osgt model.osgb`

Of course, you can convert the binary model back to text format as well:

`osgconv -O WriteImageHint=UseExternal model.osgb model.osgt`

Note the use of `-O WriteHint=UseExternal` option.
Enabling this option is desirable because it keeps the textures as external file references,
rather than embedding the textures within the model file.
Embedded textures have disadvantages such as being hard to inspect,
and impossible to share between model files.

Using shaders/normal maps
#########################

See :ref:`OSG Native Files`
 

