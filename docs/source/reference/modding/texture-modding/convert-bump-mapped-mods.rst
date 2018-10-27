====================================
Normal maps from Morrowind to OpenMW
====================================

- `General introduction to normal map conversion`_
    - `Normal Mapping in OpenMW`_
    - `Activating normal mapping shaders in OpenMW`_
    - `Normal mapping in Morrowind with Morrowind Code Patch`_
    - `Normal mapping in Morrowind with MGE XE`_
- `Converting PeterBitt's Scamp Replacer`_ (Mod made for the MGE XE PBR prototype)
    - `Tutorial - MGE`_
- `Converting Lougian's Hlaalu Bump mapped`_ (MCP's fake bump map function, part 1: *without* custom models)
    - `Tutorial - MCP, Part 1`_
- `Converting Apel's Various Things - Sacks`_ (MCP's fake bump map function, part 2: *with* custom models)
    - `Tutorial - MCP, Part 2`_

General introduction to normal map conversion
------------------------------------------------

:Authors: Joakim (Lysol) Berg
:Updated: 2016-11-11

This page has general information and tutorials on how normal mapping works in OpenMW and how you can make mods using 
the old fake normal mapping technique (such as `Netch Bump mapped`_ and `Hlaalu Bump mapped`_, and maybe the most 
(in)famous one to give shiny rocks in OpenMW, the mod `On the Rocks`_!, featured in MGSO and Morrowind Rebirth) work in OpenMW.

*Note:* The conversion made in the `Converting Apel's Various Things - Sacks`_-part of this tutorial require the use of the application NifSkope_.

*Another note:* I will use the terms bump mapping and normal mapping simultaneously. 
Normal mapping is one form of bump mapping. In other words, normal mapping is bump mapping, 
but bump mapping isn't necessarily normal mapping. 
There are several techniques for bump mapping, and normal mapping is the most common one today.

So let's get on with it.

Normal Mapping in OpenMW
************************

Normal mapping in OpenMW works in a very simple way: The engine just looks for a texture with a *_n.dds* suffix, 
and you're done.

So to expand on this a bit, let's take a look at how a model seeks for textures.

Let us assume we have the model *example.nif*. In this model file, 
there should be a tag (NiSourceTexture) that states what texture it should use and where to find it. Typically, 
it will point to something like *exampletexture_01.dds*. This texture is supposed to be located directly in the 
Textures folder since it does not state anything else. If the model is a custom made one, modders tend to group 
their textures in separate folders, just to easily keep track of them. 
It might be something like *./Textures/moddername/exampletexture_02.dds*.

When OpenMW finally adds normal mapping, it simply takes the NiSourceTexture file path, e.g., 
*exampletexture_01.dds*, and looks for a *exampletexture_01_n.dds*. If it can't find this file, no normal mapping is added. 
If it *does* find this file, the model will use this texture as a normal map. Simple.

Activating normal mapping shaders in OpenMW
*******************************************

Before normal (and specular and parallax) maps will show up in OpenMW, you'll need to activate them in the 
settings.cfg_-file. Add these rows where it would make sense:

::

    [Shaders]
    auto use object normal maps = true
    auto use terrain normal maps = true

And while we're at it, why not activate specular maps too just for the sake of it?

::

    auto use object specular maps = true
    auto use terrain specular maps = true

Lastly, if you want really nice lights in OpenMW, add these rows:

::

    force shaders = true
    clamp lighting = false

See OpenMW's wiki page about `texture modding`_ to read further about this.

Normal mapping in Morrowind with Morrowind Code Patch
*****************************************************

**Conversion difficulty:**
*Varies. Sometimes quick and easy, sometimes time-consuming and hard.*

You might have bumped (pun intended) on a few bump-mapped texture packs for Morrowind that require the 
Morrowind Code Patch (MCP). You might even be thinking: Why doesn't OpenMW just support these instead of reinventing 
the wheel? I know it sounds strange, but it will make sense. Here's how MCP handles normal maps:

Morrowind does not recognize normal maps (they weren't really a "thing" yet in 2002), so even if you have a normal map,
Morrowind will not load and display it. MCP has a clever way to solve this issue, by using something Morrowind *does* support, 
namely environment maps. You could add a tag for an environment map and then add a normal map as the environment map, 
but you'd end up with a shiny ugly model in the game. MCP solves this by turning down the brightness of the environment maps, 
making the model look *kind of* as if it had a normal map applied to it. 
I say kind of because it does not really look as good as normal mapping usually does. It was a hacky way to do it, 
but it was the only way at the time, and therefore the best way.

The biggest problem with this is not that it doesn't look as good as it could – no, 
the biggest problem in my opinion is that it requires you to state the file paths for your normal map textures *in the models*!
For buildings, which often use several textures for one single model file, it could take *ages* to do this, 
and you had to do it for dozens of model files too. You also had to ship your texture pack with model files, 
making your mod bigger in file size.

These are basically the reasons why OpenMW does not support fake bump maps like MCP does. 
It is just a really bad way to enhance your models, all the more when you have the possibility to do it in a better way.

Normal mapping in Morrowind with MGE XE
***************************************

**Conversion difficulty:**
*Easy*

The most recent feature on this topic is that the Morrowind Graphics Extender (MGE) finally started to support real 
normal mapping in an experimental version available here: `MGE XE`_ (you can't use MGE with OpenMW!). 
Not only this but it also adds full support for physically based rendering (PBR), 
making it one step ahead of OpenMW in terms of texturing techniques. However, 
OpenMW will probably have this feature in the future too – and let's hope that OpenMW and MGE will handle PBR in a 
similar fashion in the future so that mods can be used for both MGE and OpenMW without any hassle.

I haven't researched that much on the MGE variant yet but it does support real implementation of normal mapping, 
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
**Mod made for MCP's fake bump function, without custom models**

:Authors: Joakim (Lysol) Berg
:Updated: 2016-11-11

Converting textures made for the Morrowind Code Patch (MCP) fake bump mapping can be really easy or a real pain, 
depending on a few circumstances. In this tutorial, we will look at a very easy, 
although in some cases a bit time-consuming, example.

Tutorial - MCP, Part 1
**********************

We will be converting a quite popular texture replacer of the Hlaalu architecture, namely Lougian's `Hlaalu Bump mapped`_.
Since this is just a texture pack and not a model replacer, 
we can convert the mod in a few minutes by just renaming a few dozen files and by *not* extracting the included model 
(``.nif``) files when installing the mod.

#. Download Lougian's `Hlaalu Bump mapped`_.
#. Install the mod by extracting the ``./Textures`` folder to a data folder the way you usually install mods (**Pro tip**: Install using OpenMW's `Multiple data folders`_ function!).
    - Again, yes, *only* the ``./Textures`` folder. Do *not* extract the Meshes folder. They are only there to make the MCP hack work, which is not of any interest to us.
#. Go to your new texture folder. If you installed the mod like I recommended, you won't have any trouble finding the files. If you instead placed all your files in Morrowinds main Data Files folder (sigh), you need to check with the mod's .rar file to see what files you should look for. Because you'll be scrolling through a lot of files.
#. Find all the textures related to the texture pack in the Textures folder and take note of all the ones that ends with a *_nm.dds*.
#. The *_nm.dds* files are normal map files. OpenMW's standard format is to have the normal maps with a *_n.dds* instead. Rename all the normal map textures to only have a *_n.dds* instead of the *_nm.dds*.
    - As a nice bonus to this tutorial, this pack actually included one specularity texture too. We should use it of course. It's the one called "``tx_glass_amber_02_reflection.dds``". For OpenMW to recognize this file and use it as a specular map, you need to change the *_reflection.dds* part to *_spec.dds*, resulting in the name ``tx_glass_amber_01_spec.dds``.
#. That should be it. Really simple, but I do know that it takes a few minutes to rename all those files.

Now – if the mod you want to change includes custom made models it gets a bit more complicated I'm afraid. 
But that is for the next tutorial.

Converting Apel's Various Things - Sacks
----------------------------------------
**Mod made for MCP's fake bump function, with custom models**

:Authors: Joakim (Lysol) Berg
:Updated: 2016-11-09

In part one of this tutorial, we converted a mod that only included modified Morrowind model (``.nif``) 
files so that the normal maps could be loaded in Morrowind with MCP. 
We ignored those model files since they are not needed with OpenMW. In this tutorial however, 
we will convert a mod that includes new, custom made models. In other words, we cannot just ignore those files this time.

Tutorial - MCP, Part 2
**********************

The sacks included in Apel's `Various Things - Sacks`_ come in two versions – Without bump mapping, and with bump mapping. 
Since we want the glory of normal mapping in our OpenMW setup, we will go with the bump-mapped version.

#. Start by downloading Apel's `Various Things - Sacks`_ from Nexus.
#. Once downloaded, install it the way you'd normally install your mods (**Pro tip**: Install using OpenMW's `Multiple data folders`_ function!).
#. Now, if you ran the mod right away, your sacks will be made out of lead_. This is because the normal map is loaded as an environment map which MCP fixes so that it looks less shiny. We don't use MCP, so therefore, it looks kind of like the shack was made out of lead.
#. We need to fix this by removing some tags in the model files. You need to download NifSkope_ for this, which, again, only have binaries available for Windows.
#. Go the place where you installed the mod and go to ``./Meshes/o/`` to find the model files.
    - If you installed the mod like I suggested, finding the files will be easy as a pie, but if you installed it by dropping everything into your main Morrowind Data Files folder, then you'll have to scroll a lot to find them. Check the mod's zip file for the file names of the models if this is the case. The same thing applies to when fixing the textures.
#. Open up each of the models in NifSkope and look for these certain blocks_:
    - NiTextureEffect
    - NiSourceTexture with the value that appears to be a normal map file, in this mod, they have the suffix *_nm.dds*.
#. Remove all these tags by selecting them one at a time and press right click>Block>Remove Branch. (Ctrl-Del)
#. Repeat this on all the affected models.
#. If you launch OpenMW now, you'll `no longer have shiny models`_. But one thing is missing. Can you see it? It's actually hard to spot on still pictures, but we have no normal maps here.
#. Now, go back to the root of where you installed the mod. Now go to ``./Textures/`` and you'll find the texture files in question.
#. OpenMW detects normal maps if they have the same name as the base diffuse texture, but with a *_n.dds* suffix. In this mod, the normal maps has a suffix of *_nm.dds*. Change all the files that ends with *_nm.dds* to instead end with *_n.dds*.
#. Finally, `we are done`_!

Since these models have one or two textures applied to them, the fix was not that time-consuming. 
It gets worse when you have to fix a model that uses loads of textures. The principle is the same, 
it just requires more manual work which is annoying and takes time.

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
.. _Lead: https://imgur.com/bwpcYlc
.. _NifSkope: https://wiki.openmw.org/index.php?title=Tools#NifSkope
.. _Blocks: https://imgur.com/VmQC0WG
.. _`no longer have shiny models`: https://imgur.com/vu1k7n1
.. _`we are done`: https://imgur.com/yyZxlTw
