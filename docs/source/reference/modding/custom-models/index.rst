#############
Custom Models
#############

Custom models can be imported into OpenMW using a variety of formats. Models for the majority of in-game objects are assigned through openMW-CS :doc:`/manuals/openmw-cs/index`.

Some models, however, are essential for OpenMW to even run. These include player and NPC animations, and meshes for the sky. They are assigned in the ``settings.cfg`` file, with more information available in :doc:`/reference/modding/settings/models` .

Below is a quick overview of supported formats, followed by separate articles with further look at the pipelines.

* **COLLADA** has no license restrictions and is suitable for modding as well as standalone games based on the OpenMW engine. It supports static and animated models. While it doesn't yet work in all parts of the engine, work is being done to resolve the remaining limitations.

* **OSG native** has no license restrictions, but currently supports only static, non-animated models.

* **NIF** is the proprietary format used in the original Morrowind game. It supports static and animated models and everything else the format included in the original game.

.. toctree::
	:caption: Table of Contents
	:maxdepth: 1

	pipeline-blender-collada
	pipeline-blender-osgnative
	pipeline-blender-nif
