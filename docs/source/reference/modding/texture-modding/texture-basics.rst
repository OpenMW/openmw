autosectionlabel_prefix_document = True

######################
Texture Modding Basics
######################

OpenMW supports new texture mapping techniques
that were not available in the vanilla Morrowind engine.

`Normal mapping`_ is a technique used to fake lighting of bumps,
cracks and other small details.

`Specular mapping`_ is used to vary the shininess/specularity along the surface of an object.

The prerequisite to using these techniques are
`shaders <https://en.wikipedia.org/wiki/Shader>`_.
OpenMW automatically uses shaders for objects with these mapping techniques.

Normal Mapping
##############

To plug in a normal map, you name the normal map as the diffuse texture but with a specified suffix after. 
OpenMW will then recognise the file and load it as a normal map, provided you have set up your settings file correctly. 
See the section `Automatic use`_ further down below for detailed information.

Specular Mapping
################

The RGB channels of the specular map are used as the specular color.
The alpha channel specifies shininess in range [0, 255].
If a specular map is used, it will override the shininess and specular color
set in the NiMaterialProperty / osg::Material.

Morrowind format NIF files do not support normal maps or specular maps.
In order to use them anyway, see the next section.

Automatic Use
#############

Simply create the textures with appropriate naming convention
(e.g. when the base texture is called foo.dds,
the normal map would have to be called foo_n.dds).
To enable this automatic use based on filename pattern,
you will have to add the following to your
`settings.cfg </source/reference/modding/paths>`_ file::

	[Shaders]
	auto use object normal maps = true

	auto use object specular maps = true

	normal map pattern = _n
	normal height map pattern = _nh

	specular map pattern = _spec

Additionally, a normal map with the `_nh` pattern enables
the use of the normal map's alpha channel as height information.
This can be used by a `parallax mapping <https://en.wikipedia.org/wiki/Parallax_mapping>`_
shader to offset the texture depending on the viewing angle and height,
creating a fake 3D effect.

The above settings are not enabled by default to prevent incompatibilities
with mods that may be inadvertently using these naming schemes.

On the topic of shader settings,
you may be interested in these three settings as well: :ref:`force shaders`,
:ref:`force per pixel lighting`, and :ref:`clamp lighting`.
In particular, `clamp lighting = false` makes normal maps look much better!

Terrain
#######

The terrain shader also supports normal, normal-height and specular maps,
with one difference compared to objects:
the specular value must be packed into the layer texture's alpha channel.

For example, if you wanted to add specular mapping to a terrain layer called rock.dds,
you would copy this texture to a new file called rock_diffusespec.dds,
and then edit its alpha channel to set the specular intensity.

The relevant settings are::

	[Shaders]
	auto use terrain normal maps = true

	auto use terrain specular maps = true

	terrain specular map pattern = _diffusespec

	# Also used for terrain normal maps
	normal map pattern = _n
	normal height map pattern = _nh

OSG native files
################

OpenMW supports all the above shader features for meshes in the Native Mesh Format.
To have the shader generator recognize specific textures,
the `osg::Texture2D` must be named appropriately.

Available texture types are the following (most of which also have NIF equivalents):

:diffuseMap: base texture
:normalMap: normal map, as described earlier
:normalHeightMap: same as normal map, but including height information in alpha channel to be used for parallax effects
:emissiveMap: controls the material's emission, useful for objects that glow in the dark
:darkMap: multiplied onto the base texture
:detailMap: multiplied by 2 and then multiplied onto the base texture
:envMap: spherical environment map
:specularMap: specular map, as described earlier

The first texture unit automatically acts as diffuseMap if no recognized type is specified.

Example: `.osgt` file excerpt of a normal mapped mesh::

	TextureModeList 2 {
		Data 1 {
			GL_TEXTURE_2D ON
		}
		Data 1 {
			GL_TEXTURE_2D ON
		}
	}
	TextureAttributeList 2 {
		Data 1 {
			osg::Texture2D {
				UniqueID 37
				Name "diffuseMap"
				WRAP_S REPEAT
				WRAP_T REPEAT
				WRAP_R REPEAT
				MIN_FILTER LINEAR_MIPMAP_LINEAR
				MAG_FILTER LINEAR
				Image TRUE {
					UniqueID 60
					FileName "textures/BuddhaStatue_Dif.jpg"
					WriteHint 2 2
				}
			}
			Value OFF
		}
		Data 1 {
			osg::Texture2D {
				UniqueID 38
				Name "normalMap"
				WRAP_S REPEAT
				WRAP_T REPEAT
				WRAP_R REPEAT
				MIN_FILTER LINEAR_MIPMAP_LINEAR
				MAG_FILTER LINEAR
				Image TRUE {
					UniqueID 61
					FileName "textures/BuddhaStatue_Nor.jpg"
					WriteHint 2 2
				}
			}
			Value OFF
		}
	}

