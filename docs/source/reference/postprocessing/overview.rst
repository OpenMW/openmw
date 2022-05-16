#####################################
Overview of Post Processing Framework
#####################################

Overview
========

OpenMW supports a moddable post process framework for creating and
controlling screenspace effects. This is integrated into OpenMW's Lua API, see
`reference <../lua-scripting/openmw_shader.html>`_ for details.

Basic concepts
==============

Pass
    Describes a single shader invocation pass. Currently only pixel (also known
    as fragment) shaders are supported.

Technique/Shader
    An ordered list of passes, techniques will encompass a single effect like
    bloom or SSAO. Technique is interchangable with shader.

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

Hot Reloading
==============

It is possible to modify a shader without restarting OpenMW, :ref:`live reload`
must be enabled in ``settings.cfg``. Whenever a file is modified and saved, the
shader will automatically reload in game. This allows shaders to be written in a
text editor you are comfortable with. The only restriction is that new shaders
cannot be added, as the VFS will not be rebuilt and OpenMW will not be aware of
the new file.
