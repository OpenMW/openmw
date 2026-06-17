Models Settings
###############

.. omw-setting::
   :title: load unsupported nif files
   :type: boolean
   :range: true, false
   :default: false

   Allows the engine to load arbitrary NIF files that appear valid.
   Support is limited and experimental; enabling may cause crashes or memory issues.
   Do not enable unless you understand the risks.

.. omw-setting::
   :title: xbaseanim
   :type: string
   :default: meshes/xbase_anim.nif

   Path to the 3rd person base animation model file; expects a matching KF file.
   For COLLADA, use the same file for all animation entries with corresponding textkeys.

.. omw-setting::
   :title: baseanim
   :type: string
   :default: meshes/base_anim.nif

   Path to the 3rd person base model file with textkeys data.

.. omw-setting::
   :title: xbaseanim1st
   :type: string
   :default: meshes/xbase_anim.1st.nif

   Path to the 1st person base animation model file; expects a matching KF file.

.. omw-setting::
   :title: baseanimkna
   :type: string
   :default: meshes/base_animkna.nif

   Path to the 3rd person beast race base model with textkeys data.

.. omw-setting::
   :title: baseanimkna1st
   :type: string
   :default: meshes/base_animkna.1st.nif

   Path to the 1st person beast race base animation model.

.. omw-setting::
   :title: xbaseanimfemale
   :type: string
   :default: meshes/xbase_anim_female.nif

   Path to the 3rd person female base animation model.

.. omw-setting::
   :title: baseanimfemale
   :type: string
   :default: meshes/base_anim_female.nif

   Path to the 3rd person female base model with textkeys data.

.. omw-setting::
   :title: baseanimfemale1st
   :type: string
   :default: meshes/base_anim_female.1st.nif

   Path to the 1st person female base model with textkeys data.

.. omw-setting::
   :title: wolfskin
   :type: string
   :default: meshes/wolf/skin.nif

   Path to the 3rd person werewolf skin model.

.. omw-setting::
   :title: wolfskin1st
   :type: string
   :default: meshes/wolf/skin.1st.nif

   Path to the 1st person werewolf skin model.

.. omw-setting::
   :title: xargonianswimkna
   :type: string
   :default: meshes/xargonian_swimkna.nif

   Path to the Argonian swimkna model.

.. omw-setting::
   :title: xbaseanimkf
   :type: string
   :default: meshes/xbase_anim.kf

   Animation file for xbaseanim 3rd person animations.

.. omw-setting::
   :title: xbaseanim1stkf
   :type: string
   :default: meshes/xbase_anim.1st.kf

   Animation file for xbaseanim 1st person animations.

.. omw-setting::
   :title: xbaseanimfemalekf
   :type: string
   :default: meshes/xbase_anim_female.kf

   Animation file for xbaseanim female animations.

.. omw-setting::
   :title: xargonianswimknakf
   :type: string
   :default: meshes/xargonian_swimkna.kf

   Animation file for xargonianswimkna animations.

.. omw-setting::
   :title: skyatmosphere
   :type: string
   :default: meshes/sky_atmosphere.nif

   Sky atmosphere mesh for the top half of the sky.

.. omw-setting::
   :title: skyclouds
   :type: string
   :default: meshes/sky_clouds_01.nif

   Sky clouds mesh displaying scrolling cloud textures.

.. omw-setting::
   :title: skynight01
   :type: string
   :default: meshes/sky_night_01.nif

   Sky stars mesh used during night if skynight02 is not present.

.. omw-setting::
   :title: skynight02
   :type: string
   :default: meshes/sky_night_02.nif

   Sky stars mesh used during night, takes priority over skynight01.

.. omw-setting::
   :title: weatherashcloud
   :type: string
   :default: meshes/ashcloud.nif

   Ash clouds weather effect file from Morrowind (not used by OpenMW).

.. omw-setting::
   :title: weatherblightcloud
   :type: string
   :default: meshes/blightcloud.nif

   Blight clouds weather effect file from Morrowind (not used by OpenMW).

.. omw-setting::
   :title: weathersnow
   :type: string
   :default: meshes/snow.nif

   Snow falling weather effect file from Morrowind (not used by OpenMW).

.. omw-setting::
   :title: weatherblizzard
   :type: string
   :default: meshes/blizzard.nif

   Blizzard weather effect file from Morrowind (not used by OpenMW).
