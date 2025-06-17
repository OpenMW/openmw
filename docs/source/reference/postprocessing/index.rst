###############
Post Processing
###############

OpenMW supports a moddable post process framework for creating and
controlling screenspace effects. This is integrated into OpenMW's Lua API, see
`reference <../lua-scripting/openmw_postprocessing.html>`_ for details.

Basic concepts
==============

Pass
    Describes a single shader invocation pass. Currently only pixel (also known
    as fragment) shaders are supported.

Technique/Shader
    An ordered list of passes, techniques will encompass a single effect like
    bloom or SSAO. Technique is interchangeable with shader.

Installing and Activating
=========================

Shaders are managed through the virtual file system, simply install the associated
archive or folder as described in :ref:`mod-install<install>`. Shaders must be
in the `Shaders` directory to be discoverable. A shader can be activated in one
of two ways:

1. Adding the shaders filename (without its extension) to the end of the
   :ref:`chain` list in ``settings.cfg``.
2. Using the in game post processor HUD, which can be activated with the ``F2``
   key by default. This is the recommended method as manual editing can be error
   prone.

Localization
============

Output text (e.g. shader description) can use the ``#{ContextName:Key}`` tags.
In this case OpenMW replaces it for value of ``Key`` key from the
``Data Files\L10n\ContextName\used_language.yaml`` file.

Hot Reloading
=============

It is possible to modify a shader without restarting OpenMW, hot reloading
can be enabled by using the lua command `debug.setShaderHotReloadEnabled(true)`.
Whenever a file is modified and saved, the shader will automatically reload in game.
This allows shaders to be written in a text editor you are comfortable with. 
The only restriction is that the VFS is not aware of new files or changes in non-shader files, 
so new shaders and localization strings can not be used.


.. toctree::
    :caption: Table of Contents
    :includehidden:
    :hidden:
    :maxdepth: 2

    omwfx
    lua