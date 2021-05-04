Models Settings
###############

load unsupported nif files
--------------------------

:Type:		boolean
:Range:		True/False
:Default:	False

Allow the engine to load arbitrary NIF files as long as they appear to be valid.

OpenMW has limited and **experimental** support for NIF files
that Morrowind itself cannot load, which normally goes unused.

If enabled, this setting allows the NIF loader to make use of that functionality.

.. warning::
	You must keep in mind that since the mentioned support is experimental,
	loading unsupported NIF files may fail, and the degree of this failure may vary.
	
	In milder cases, OpenMW will reject the file anyway because
	it lacks a definition for a certain record type that the file may use.
	
	In more severe cases OpenMW's incomplete understanding of a record type
	can lead to memory corruption, freezes or even crashes.
	
	**Do not enable** this if you're not so sure that you know what you're doing.

To help debug possible issues OpenMW will log its progress in loading
every file that uses an unsupported NIF version.

xbaseanim
---------

:Type:		string
:Range:		
:Default:	meshes/xbase_anim.nif

Path to the file used for 3rd person base animation model that looks also for the corresponding kf-file.

.. note::
	If you are using the COLLADA format, you don't need to separate the files as they are separated between .nif and .kf files. It works if you plug the same COLLADA file into all animation-related entries, just make sure there is a corresponding textkeys file. You can read more about the textkeys in :doc:`../../modding/custom-models/pipeline-blender-collada`.

baseanim
--------

:Type:		string
:Range:		
:Default:	meshes/base_anim.nif

Path to the file used for 3rd person base model with textkeys-data.

xbaseanim1st
------------

:Type:		string
:Range:		
:Default:	meshes/xbase_anim.1st.nif

Path to the file used for 1st person base animation model that looks also for corresponding kf-file.

baseanimkna
-----------

:Type:		string
:Range:		
:Default:	meshes/base_animkna.nif

Path to the file used for 3rd person beast race base model with textkeys-data.

baseanimkna1st
--------------

:Type:		string
:Range:		
:Default:	meshes/base_animkna.1st.nif

Path to the file used for 1st person beast race base animation model.

xbaseanimfemale
---------------

:Type:		string
:Range:		
:Default:	meshes/xbase_anim_female.nif

Path to the file used for 3rd person female base animation model.

baseanimfemale
--------------

:Type:		string
:Range:		
:Default:	meshes/base_anim_female.nif

Path to the file used for 3rd person female base model with textkeys-data.

baseanimfemale1st
-----------------

:Type:		string
:Range:		
:Default:	meshes/base_anim_female.1st.nif

Path to the file used for 1st person female base model with textkeys-data.

wolfskin
--------

:Type:		string
:Range:		
:Default:	meshes/wolf/skin.nif

Path to the file used for 3rd person werewolf skin.

wolfskin1st
-----------

:Type:		string
:Range:		
:Default:	meshes/wolf/skin.1st.nif

Path to the file used for 1st person werewolf skin.

xargonianswimkna
----------------

:Type:		string
:Range:		
:Default:	meshes/xargonian_swimkna.nif

Path to the file used for Argonian swimkna.

xbaseanimkf
-----------

:Type:		string
:Range:		
:Default:	meshes/xbase_anim.kf

File to load xbaseanim 3rd person animations.

xbaseanim1stkf
--------------

:Type:		string
:Range:		
:Default:	meshes/xbase_anim.1st.kf

File to load xbaseanim 3rd person animations.

xbaseanimfemalekf
-----------------

:Type:		string
:Range:		
:Default:	meshes/xbase_anim_female.kf

File to load xbaseanim animations from.

xargonianswimknakf
------------------

:Type:		string
:Range:		
:Default:	meshes/xargonian_swimkna.kf

File to load xargonianswimkna animations from.

skyatmosphere
-------------

:Type:		string
:Range:		
:Default:	meshes/sky_atmosphere.nif

Path to the file used for the sky atmosphere mesh, which is one of the three meshes needed to render the sky. It's used to make the top half of the sky blue and renders in front of the background color.

skyclouds
---------

:Type:		string
:Range:		
:Default:	meshes/sky_clouds_01.nif.

Path to the file used for the sky clouds mesh, which is one of the three meshes needed to render the sky. It displays a scrolling texture of clouds in front of the atmosphere mesh and background color

skynight01
----------

:Type:		string
:Range:		
:Default:	meshes/sky_night_01.nif

Path to the file used for the sky stars mesh, which is one of the three meshes needed to render the sky. During night, it displays a texture with stars in front of the atmosphere and behind the clouds. If skynight02 is present, skynight01 will not be used.

skynight02
----------

:Type:		string
:Range:		
:Default:	meshes/sky_night_02.nif

Path to the file used for the sky stars mesh, which is one of the three meshes needed to render the sky. During night, it displays a texture with stars in front of the atmosphere and behind the clouds. If it's present it will be used instead of skynight01.

weatherashcloud
---------------

:Type:		string
:Range:		
:Default:	meshes/ashcloud.nif

Path to the file used for the ash clouds weather effect in Morrowind. OpenMW doesn't use this file, instead it renders a similar looking particle effect. Changing this won't have any effect.

weatherblightcloud
------------------

:Type:		string
:Range:		
:Default:	meshes/blightcloud.nif

Path to the file used for the blight clouds weather effect in Morrowind. OpenMW doesn't use this file, instead it renders a similar looking particle effect. Changing this won't have any effect.

weathersnow
-----------

:Type:		string
:Range:		
:Default:	meshes/snow.nif

Path to the file used for the snow falling weather effect in Morrowind. OpenMW doesn't use this file, instead it renders a similar looking particle effect. Changing this won't have any effect.

weatherblizzard
---------------

:Type:		string
:Range:		
:Default:	meshes/blizzard.nif

Path to the file used for the blizzard clouds weather effect in Morrowind. OpenMW doesn't use this file, instead it renders a similar looking particle effect. Changing this won't have any effect.
