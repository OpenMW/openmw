====================================
Normal maps from Morrowind to OpenMW
====================================

- `General introduction to normal map conversion`_
    - `OpenMW normal-mapping`_
    - `Activating normal-mapping shaders in OpenMW`_
    - `Morrowind bump-mapping`_
    - `MGE XE normal-mapping`_
- `Converting PeterBitt's Scamp Replacer`_ (Mod made for the MGE XE PBR prototype)
    - `Tutorial - MGE`_
- `Converting Lougian's Hlaalu Bump mapped`_ (Morrowind's bump-mapping, part 1: *without* custom models)
    - `Tutorial - Morrowind, Part 1`_
- `Converting Apel's Various Things - Sacks`_ (Morrowind's bump-mapping, part 2: *with* custom models)
    - `Tutorial - Morrowind, Part 2`_

General introduction to normal map conversion
------------------------------------------------

:Authors: Joakim (Lysol) Berg, Alexei (Capo) Dobrohotov
:Updated: 2020-03-03

This page has general information and tutorials on how normal-mapping works in OpenMW and how you can make mods using
the old environment-mapped bump-mapping technique (such as `Netch Bump mapped`_ and `Hlaalu Bump mapped`_, and maybe the most
(in)famous one to previously give shiny rocks in OpenMW, the mod `On the Rocks`_!, featured in MGSO and Morrowind Rebirth) work better in OpenMW.

*Note:* The conversion made in the `Converting Apel's Various Things - Sacks`_-part of this tutorial require the use of the application NifSkope_.

*Another note:* I will use the terms bump-mapping and normal-mapping simultaneously.
Normal-mapping is one form of bump-mapping. In other words, normal-mapping is bump-mapping,
but bump-mapping isn't necessarily normal-mapping.
There are several techniques for bump-mapping, and normal-mapping is the most common one today.

So let's get on with it.

OpenMW normal-mapping
************************

Normal-mapping in OpenMW works in a very simple way: The engine just looks for a texture with a *_n.dds* suffix,
and you're done.

So to expand on this a bit, let's take a look at how a model looks up textures.

Let us assume we have the model *example.nif*. In this model file,
there should be a tag (NiTexturingProperty) that states what textures it should use and where to find them. Typically,
the model's base (diffuse) texture reference will point to something named like *exampletexture_01.dds*. This texture is supposed to be located directly in the
Textures folder since it does not state anything else.
Modders tend to group textures for custom-made models in dedicated folders to keep track of them easily,
so it might be something like *./Textures/moddername/exampletexture_02.dds*.

OpenMW will pick the diffuse map file path from the mesh, e.g.
*exampletexture_01.dds*, and look up a texture named *exampletexture_01_n.dds*.
That file will be the normal map if it's present. Simple.

Activating normal-mapping shaders in OpenMW
*******************************************

Before normal (and specular and parallax) maps can show up in OpenMW, their auto-detection needs to be turned on in
settings.cfg_-file. Add these rows where it would make sense:

::

    [Shaders]
    auto use object normal maps = true
    auto use terrain normal maps = true

    auto use object specular maps = true
    auto use terrain specular maps = true

See OpenMW's wiki page about `texture modding`_ to read more about it.

Morrowind bump-mapping
*****************************************************

**Conversion difficulty:**
*Varies. Sometimes quick and easy, sometimes time-consuming and hard.*

You might have bumped (pun intended) on a few bump-mapped texture packs for Morrowind that require
Morrowind Code Patch (MCP). OpenMW supports them, and like MCP can optionally apply lighting after environment maps
are processed which makes bump-mapped models look a bit better,
can make use of the gloss map channel in the bump map and can apply bump-mapping to skinned models.
Add this to settings.cfg_-file:

::

    [Shaders]
    apply lighting to environment maps = true

But sometimes you may want them to look a bit better than in vanilla.
Technically you aren't supposed to convert bump maps because they shouldn't be normal maps that are supported by OpenMW as well,
but artists may use actual normal maps as bump maps either because they look better in vanilla... or because they're lazy.
In this case you can benefit from OpenMW's normal-mapping support by using these bump maps the way normal maps are used.
This means that you will have to drop the bump-mapping references from the model and sometimes rename the texture.

MGE XE normal-mapping
***************************************

**Conversion difficulty:**
*Easy*

The most recent feature on this topic is that the Morrowind Graphics Extender (MGE) finally started to support real
normal-mapping in an experimental version available here: `MGE XE`_ (you can't use MGE with OpenMW!).
Not only this but it also adds full support for physically based rendering (PBR),
making it one step ahead of OpenMW in terms of texturing techniques. However,
OpenMW will probably have this feature in the future too – and let's hope that OpenMW and MGE will handle PBR in a
similar fashion in the future so that mods can be used for both MGE and OpenMW without any hassle.

I haven't researched that much on the MGE variant yet but it does support real implementation of normal-mapping,
making it really easy to convert mods made for MGE into OpenMW (I'm only talking about the normal map textures though).
There's some kind of text file if I understood it correctly that MGE uses to find the normal map.
OpenMW does not need this, you just have to make sure the normal map has the same name as the diffuse texture but with
the correct suffix after.

Now, on to the tutorials.

Converting PeterBitt's Scamp Replacer
-------------------------------------
**Mod made for the MGE XE PBR prototype**

:Authors: Joakim (Lysol) Berg
:Updated: 2016-11-11

So, let's say you've found out that PeterBitt_ makes awesome models and textures featuring physically based rendering
(PBR) and normal maps. Let's say that you tried to run his `PBR Scamp Replacer`_ in OpenMW and that you were greatly
disappointed when the normal map didn't seem to work. Lastly, let's say you came here, looking for some answers.
Am I right? Great. Because you've come to the right place!

*A quick note before we begin*: Please note that you can only use the normal map texture and not the rest of the materials,
since PBR isn't implemented in OpenMW yet. Sometimes PBR textures can look dull without all of the texture files,
so have that in mind.

Tutorial - MGE
**************

In this tutorial, I will use PeterBitt's `PBR Scamp Replacer`_ as an example,
but any mod featuring PBR that requires the PBR version of MGE will do,
provided it also includes a normal map (which it probably does).

So, follow these steps:

#. Go to the Nexus page for PeterBitt's `PBR Scamp Replacer`_
#. Go to the *files* tab and download the main file and the "PBR materials" file.
#. Extract the main file as if you'd install a normal mod (**Pro tip**: Install using OpenMW's `Multiple data folders`_ function!)
#. Now, open the PBR materials file:
    - Go to ``./Materials/PB/``.
    - Select the ``tx_Scamp_normals.dds`` file, which is, obviously, the normal map texture.
    - Extract this file to the place you extracted the main file to, but in the subdirectory ``./Textures/PB/``.
#. Rename your newly extracted file (``tx_Scamp_normals.dds``) to ``tx_Scamp_n.dds`` (which is exactly the same name as the diffuse texture file, except for the added *_n* suffix before the filename extention).
#. You're actually done!

So as you might notice, converting these mods is very simple and takes just a couple of minutes.
It's more or less just a matter of renaming and moving a few files.

I totally recommend you to also try this on PeterBitt's Nix Hound replacer and Flash3113's various replacers.
It should be the same principle to get those to work.

And let's hope that some one implements PBR shaders to OpenMW too,
so that we can use all the material files of these mods in the future.

Converting Lougian's Hlaalu Bump mapped
---------------------------------------
**Mod made for Morrowind's bump-mapping, without custom models**

:Authors: Joakim (Lysol) Berg, Alexei (Capo) Dobrohotov
:Updated: 2020-03-03

Converting normal maps made for the Morrowind's bump-mapping can be really easy or a real pain,
depending on a few circumstances. In this tutorial, we will look at a very easy,
although in some cases a bit time-consuming, example.

Tutorial - Morrowind, Part 1
**********************

We will be converting a quite popular texture replacer of the Hlaalu architecture, namely Lougian's `Hlaalu Bump mapped`_.
Since this is just a texture pack and not a model replacer,
we can convert the mod in a few minutes by just renaming a few dozen files and by *not* extracting the included model
(``.nif``) files when installing the mod.

#. Download Lougian's `Hlaalu Bump mapped`_.
#. Install the mod by extracting the ``./Textures`` folder to a data folder the way you usually install mods (**Pro tip**: Install using OpenMW's `Multiple data folders`_ function!).
    - Again, yes, *only* the ``./Textures`` folder. Do not extract the Meshes folder. They are there to make Morrowind bump-mapping work.
#. Go to your new texture folder. If you installed the mod like I recommended, you won't have any trouble finding the files. If you instead placed all your files in Morrowinds main Data Files folder (sigh), you need to check with the mod's .rar file to see what files you should look for. Because you'll be scrolling through a lot of files.
#. Find all the textures related to the texture pack in the Textures folder and take note of all the ones that ends with a *_nm.dds*.
#. The *_nm.dds* files are normal map files. OpenMW's standard format is to have the normal maps with a *_n.dds* instead. Rename all the normal map textures to only have a *_n.dds* instead of the *_nm.dds*.
    - As a nice bonus to this tutorial, this pack actually included one specularity texture too. We should use it of course. It's the one called "``tx_glass_amber_02_reflection.dds``". For OpenMW to recognize this file and use it as a specular map, you need to change the *_reflection.dds* part to *_spec.dds*, resulting in the name ``tx_glass_amber_01_spec.dds``.
#. That should be it. Really simple, but I do know that it takes a few minutes to rename all those files.

Now – if the mod you want to change includes custom made models it gets a bit more complicated I'm afraid.
But that is for the next tutorial.

Converting Apel's Various Things - Sacks
----------------------------------------
**Mod made for Morrowind bump-mapping, with custom models**

:Authors: Joakim (Lysol) Berg, Alexei (Capostrophic) Dobrohotov
:Updated: 2020-03-03

In part one of this tutorial, we converted a mod that only included modified Morrowind model (``.nif``)
files so that the bump maps could be loaded as normal maps.
We ignored those model files since they are not needed with OpenMW. In this tutorial however,
we will convert a mod that includes new, custom-made models. In other words, we cannot just ignore those files this time.

Tutorial - Morrowind, Part 2
**********************

The sacks included in Apel's `Various Things - Sacks`_ come in two versions – without bump-mapping, and with bump-mapping.
Since we want the glory of normal-mapping in our OpenMW setup, we will go with the bump-mapped version.

#. Start by downloading Apel's `Various Things - Sacks`_ from Nexus.
#. Once downloaded, install it the way you'd normally install your mods (**Pro tip**: Install using OpenMW's `Multiple data folders`_ function!).
#. Now, if you ran the mod right away, your sacks may look... wetter than expected. This is because the mod assumes you have the MCP feature which makes the sacks less shiny enabled. You can have its equivalent enabled to make the sacks look like in Morrowind with MCP, or you may proceed on the tutorial.
#. We need to fix this by removing some tags in the model files. You need to download NifSkope_ for this, which, again, only have binaries available for Windows.
#. Go the place where you installed the mod and go to ``./Meshes/o/`` to find the model files.
    - If you installed the mod like I suggested, finding the files will be easy as a pie, but if you installed it by dropping everything into your main Morrowind Data Files folder, then you'll have to scroll a lot to find them. Check the mod's zip file for the file names of the models if this is the case. The same thing applies to when fixing the textures.
#. Open up each of the models in NifSkope and look for these certain blocks_:
    - NiTextureEffect
    - NiSourceTexture with the value that appears to be a normal map file, in this mod, they have the suffix *_nm.dds*.
#. Remove all these tags by selecting them one at a time and press right click>Block>Remove Branch. (Ctrl-Del)
#. Repeat this on all the affected models.
#. If you launch OpenMW now, you'll `no longer have wet models`_. But one thing is missing. Can you see it? It's actually hard to spot on still pictures, but we have no normal maps here.
#. Now, go back to the root of where you installed the mod. Now go to ``./Textures/`` and you'll find the texture files in question.
#. OpenMW detects normal maps if they have the same name as the base diffuse texture, but with a *_n.dds* suffix. In this mod, the normal maps has a suffix of *_nm.dds*. Change all the files that ends with *_nm.dds* to instead end with *_n.dds*.
#. Finally, `we are done`_!

Since these models have one or two textures applied to them, the fix was not that time-consuming. The process continues to work for more complex models that use more textures, but looking through each category for texture effects and normal mapped textures rapidly becomes tedious. Luckily, NifSkope provides a feature to do the same automatically.

Right-click in NifSkope to access the *Spells* dropdown menu, also available via the top bar, hover over the *Blocks* section, and `choose the action to Remove by ID`_. You can then input the RegEx expression ``^NiTextureEffect`` (directing it to remove any block whose name starts with "NiTextureEffect") to automatically remove all texture effect blocks within the NIF. This also has the helpful side effect of listing `all the blocks within the NIF in the bottom section`_, allowing you to additionally root out any blocks referencing *_nm.dds* textures without having to painstakingly open each category.

.. _`Netch Bump mapped`: https://www.nexusmods.com/morrowind/mods/42851/?
.. _`Hlaalu Bump mapped`: https://www.nexusmods.com/morrowind/mods/42396/?
.. _`On the Rocks`: http://mw.modhistory.com/download-44-14107
.. _`texture modding`: https://wiki.openmw.org/index.php?title=TextureModding
.. _`MGE XE`: https://www.nexusmods.com/morrowind/mods/26348/?
.. _PeterBitt: https://www.nexusmods.com/morrowind/users/4381248/?
.. _`PBR Scamp Replacer`: https://www.nexusmods.com/morrowind/mods/44314/?
.. _settings.cfg: https://wiki.openmw.org/index.php?title=Settings
.. _`Multiple data folders`: https://wiki.openmw.org/index.php?title=Mod_installation
.. _`Various Things - Sacks`: https://www.nexusmods.com/morrowind/mods/42558/?
.. _NifSkope: https://wiki.openmw.org/index.php?title=Tools#NifSkope
.. _Blocks: https://imgur.com/VmQC0WG
.. _`no longer have wet models`: https://imgur.com/vu1k7n1
.. _`we are done`: https://imgur.com/yyZxlTw
.. _`choose the action to Remove by ID`: https://imgur.com/a/qs2t0tC
.. _`all the blocks within the NIF in the bottom section`: https://imgur.com/a/UFFNyWt
