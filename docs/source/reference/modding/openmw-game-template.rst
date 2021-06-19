####################
OpenMW Game Template
####################

OpenMW Game Template, or simply, the Template, is a set of base assets required
for OpenMW to run. These include ``template.omwgame`` along with models, textures,
fonts, and UI content. The Template can be used as a foundation for a standalone
game in OpenMW, without requiring any of the original, copyrighted Morrowind assets.
With the exception of ``Pelagiad.ttf`` font file, the Template is released as
`public domain <https://creativecommons.org/publicdomain/zero/1.0/>`_.


Installation
************

The Template is installed the same way you would install a mod, with general
instructions available at :doc:`mod-install`. It can be downloaded from
`its repository <https://gitlab.com/OpenMW/example-suite>`_ and requires
OpenMW 0.47 or later.

After getting the Template, extract its ``/data`` folder to somewhere on your disk.

.. note::

    It's adviseable to not put the Template files in the same folder as your
    Morrowind files. This is especially valid when you don't wish to mix the two games
    and use the Template as a foundation for a standalone game.


Define paths to .omwgame and data files
=======================================

OpenMW needs to be told where to look for the Template files. This is done in
``openmw.cfg`` file where ``content=`` tells OpenMW which .omwgame file to use
and ``data=`` tells OpenMW what folders to look for meshes, textures, audio,
and other assets. The required lines would look like this, but with the paths
of course different on your system.

.. code::

    content=template.omwgame
    data="/home/someuser/example-suite/data"
    data="/home/someuser/example-suite"


Remove references to Morrowind files
====================================

In case you have Morrowind installed and have run OpenMW's installation wizard,
you need to remove or comment out the following lines from ``openmw.cfg``.
Not doing so will either produce errors or load Morrowind content, which you
probably do not want when you are making your own game.

.. code::

    fallback-archive=Morrowind.bsa
    fallback-archive=Tribunal.bsa
    fallback-archive=Bloodmoon.bsa
    content=Morrowind.esm
    content=Tribunal.esm
    content=Bloodmoon.esm
    data="/home/someuser/.wine/dosdevices/c:/Morrowind/Data Files"


Define paths to essential models
================================

Certain models, essential to OpenMW, cannot be assigned through OpenMW-CS but
are instead assigned through ``settings.cfg``. These models are player and NPC
animations, and meshes for the sky. In ``settings.cfg`` used by your OpenMW
install, add the following lines under the ``[Models]`` section.

.. code::

    xbaseanim = meshes/BasicPlayer.dae
    baseanim = meshes/BasicPlayer.dae
    xbaseanim1st = meshes/BasicPlayer.dae
    baseanimkna = meshes/BasicPlayer.dae
    baseanimkna1st = meshes/BasicPlayer.dae
    xbaseanimfemale = meshes/BasicPlayer.dae
    baseanimfemale = meshes/BasicPlayer.dae
    baseanimfemale1st = meshes/BasicPlayer.dae
    xargonianswimkna = meshes/BasicPlayer.dae
    xbaseanimkf = meshes/BasicPlayer.dae
    xbaseanim1stkf = meshes/BasicPlayer.dae
    xbaseanimfemalekf = meshes/BasicPlayer.dae
    xargonianswimknakf = meshes/BasicPlayer.dae
    skyatmosphere = meshes/sky_atmosphere.dae
    skyclouds = meshes/sky_clouds_01.osgt
    skynight01 = meshes/sky_night_01.osgt


As a convenience the Template repository includes a ``settings.cfg`` containing
these same lines which can be copied and pasted. However, do not use the file
to simply overwrite the ``settings.cfg`` used by your OpenMW installation.


Copying the UI files
====================

The Template includes a ``resources/mygui`` folder. The contents of this folder
need to be copied to ``resources/mygui`` folder found in your OpenMW installation
folder. Overwrite any files aready in this folder. These files provide the
UI font, its definition, and some minor UI tweaks.

.. code::

    openmw_box.skin.xml
    openmw_button.skin.xml
    openmw_font.xml
    openmw_windows.skin.xml
    Pelagiad.ttf


Run OpenMW Launcher
*******************

After completing all the steps, run OpenMW Launcher and make sure ``template.omwgame``
is selected in *Data Files* tab. Then, run the game and enjoy an empty island. It is not
empty though! It is full of potential to start making your very own game on the
OpenMW engine. Good luck! 

