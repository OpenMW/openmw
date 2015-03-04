#include "material.hpp"

#include <components/nif/node.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/settings/settings.hpp>
#include <components/nifoverrides/nifoverrides.hpp>

#include <extern/shiny/Main/Factory.hpp>

#include <OgreMaterialManager.h>
#include <OgreMaterial.h>

#include <boost/functional/hash.hpp>


namespace NifOgre
{

// Conversion of blend / test mode from NIF
static const char *getBlendFactor(int mode)
{
    switch(mode)
    {
    case 0: return "one";
    case 1: return "zero";
    case 2: return "src_colour";
    case 3: return "one_minus_src_colour";
    case 4: return "dest_colour";
    case 5: return "one_minus_dest_colour";
    case 6: return "src_alpha";
    case 7: return "one_minus_src_alpha";
    case 8: return "dest_alpha";
    case 9: return "one_minus_dest_alpha";
    case 10: return "src_alpha_saturate";
    }
    std::cerr<< "Unexpected blend mode: "<<mode <<std::endl;
    return "src_alpha";
}

static const char *getTestMode(int mode)
{
    switch(mode)
    {
    case 0: return "always_pass";
    case 1: return "less";
    case 2: return "equal";
    case 3: return "less_equal";
    case 4: return "greater";
    case 5: return "not_equal";
    case 6: return "greater_equal";
    case 7: return "always_fail";
    }
    std::cerr<< "Unexpected test mode: "<<mode <<std::endl;
    return "less_equal";
}

static void setTextureProperties(sh::MaterialInstance* material, const std::string& textureSlotName, const Nif::NiTexturingProperty::Texture& tex)
{
    material->setProperty(textureSlotName + "UVSet", sh::makeProperty(new sh::IntValue(tex.uvSet)));
    const std::string clampMode = textureSlotName + "ClampMode";
    switch (tex.clamp)
    {
    case 0:
        material->setProperty(clampMode, sh::makeProperty(new sh::StringValue("clamp clamp")));
        break;
    case 1:
        material->setProperty(clampMode, sh::makeProperty(new sh::StringValue("clamp wrap")));
        break;
    case 2:
        material->setProperty(clampMode, sh::makeProperty(new sh::StringValue("wrap clamp")));
        break;
    case 3:
    default:
        material->setProperty(clampMode, sh::makeProperty(new sh::StringValue("wrap wrap")));
        break;
    }
}

Ogre::String NIFMaterialLoader::getMaterial(const Nif::ShapeData *shapedata,
                                            const Ogre::String &name, const Ogre::String &group,
                                            const Nif::NiTexturingProperty *texprop,
                                            const Nif::NiMaterialProperty *matprop,
                                            const Nif::NiAlphaProperty *alphaprop,
                                            const Nif::NiVertexColorProperty *vertprop,
                                            const Nif::NiZBufferProperty *zprop,
                                            const Nif::NiSpecularProperty *specprop,
                                            const Nif::NiWireframeProperty *wireprop,
                                            const Nif::NiStencilProperty *stencilprop,
                                            bool &needTangents, bool particleMaterial)
{
    Ogre::MaterialManager &matMgr = Ogre::MaterialManager::getSingleton();
    Ogre::MaterialPtr material = matMgr.getByName(name);
    if(!material.isNull())
        return name;

    Ogre::Vector3 ambient(1.0f);
    Ogre::Vector3 diffuse(1.0f);
    Ogre::Vector3 specular(0.0f);
    Ogre::Vector3 emissive(0.0f);
    float glossiness = 0.0f;
    float alpha = 1.0f;
    int alphaFlags = 0;
    int alphaTest = 0;
    int vertMode = 2;
    //int lightMode = 1;
    int depthFlags = 3;
    // Default should be 1, but Bloodmoon's models are broken
    int specFlags = 0;
    int wireFlags = 0;
    int drawMode = 1;
    Ogre::String texName[7];

    bool vertexColour = (shapedata->colors.size() != 0);

    // Texture
    if(texprop)
    {
        for(int i = 0;i < 7;i++)
        {
            if(!texprop->textures[i].inUse)
                continue;
            if(texprop->textures[i].texture.empty())
            {
                warn("Texture layer "+Ogre::StringConverter::toString(i)+" is in use but empty in "+name);
                continue;
            }

            const Nif::NiSourceTexture *st = texprop->textures[i].texture.getPtr();
            if(st->external)
                texName[i] = Misc::ResourceHelpers::correctTexturePath(st->filename);
            else
                warn("Found internal texture, ignoring.");
        }

        Nif::ControllerPtr ctrls = texprop->controller;
        while(!ctrls.empty())
        {
            if (ctrls->recType != Nif::RC_NiFlipController) // Handled in ogrenifloader
                warn("Unhandled texture controller "+ctrls->recName+" in "+name);
            ctrls = ctrls->next;
        }
    }

    // Alpha modifiers
    if(alphaprop)
    {
        alphaFlags = alphaprop->flags;
        alphaTest = alphaprop->data.threshold;

        Nif::ControllerPtr ctrls = alphaprop->controller;
        while(!ctrls.empty())
        {
            warn("Unhandled alpha controller "+ctrls->recName+" in "+name);
            ctrls = ctrls->next;
        }
    }

    // Vertex color handling
    if(vertprop)
    {
        vertMode = vertprop->data.vertmode;
        // FIXME: Handle lightmode?
        //lightMode = vertprop->data.lightmode;

        Nif::ControllerPtr ctrls = vertprop->controller;
        while(!ctrls.empty())
        {
            warn("Unhandled vertex color controller "+ctrls->recName+" in "+name);
            ctrls = ctrls->next;
        }
    }

    if(zprop)
    {
        depthFlags = zprop->flags;
        // Depth function???

        Nif::ControllerPtr ctrls = zprop->controller;
        while(!ctrls.empty())
        {
            warn("Unhandled depth controller "+ctrls->recName+" in "+name);
            ctrls = ctrls->next;
        }
    }

    if(specprop)
    {
        specFlags = specprop->flags;

        Nif::ControllerPtr ctrls = specprop->controller;
        while(!ctrls.empty())
        {
            warn("Unhandled specular controller "+ctrls->recName+" in "+name);
            ctrls = ctrls->next;
        }
    }

    if(wireprop)
    {
        wireFlags = wireprop->flags;

        Nif::ControllerPtr ctrls = wireprop->controller;
        while(!ctrls.empty())
        {
            warn("Unhandled wireframe controller "+ctrls->recName+" in "+name);
            ctrls = ctrls->next;
        }
    }

    if(stencilprop)
    {
        drawMode = stencilprop->data.drawMode;
        if (stencilprop->data.enabled)
            warn("Unhandled stencil test in "+name);

        Nif::ControllerPtr ctrls = stencilprop->controller;
        while(!ctrls.empty())
        {
            warn("Unhandled stencil controller "+ctrls->recName+" in "+name);
            ctrls = ctrls->next;
        }
    }

    // Material
    if(matprop)
    {
        ambient = matprop->data.ambient;
        diffuse = matprop->data.diffuse;
        specular = matprop->data.specular;
        emissive = matprop->data.emissive;
        glossiness = matprop->data.glossiness;
        alpha = matprop->data.alpha;

        Nif::ControllerPtr ctrls = matprop->controller;
        while(!ctrls.empty())
        {
            if (ctrls->recType != Nif::RC_NiAlphaController && ctrls->recType != Nif::RC_NiMaterialColorController)
                warn("Unhandled material controller "+ctrls->recName+" in "+name);
            ctrls = ctrls->next;
        }
    }

    if (particleMaterial)
    {
        alpha = 1.f; // Apparently ignored, might be overridden by particle vertex colors?
    }

    {
        // Generate a hash out of all properties that can affect the material.
        size_t h = 0;
        boost::hash_combine(h, ambient.x);
        boost::hash_combine(h, ambient.y);
        boost::hash_combine(h, ambient.z);
        boost::hash_combine(h, diffuse.x);
        boost::hash_combine(h, diffuse.y);
        boost::hash_combine(h, diffuse.z);
        boost::hash_combine(h, alpha);
        boost::hash_combine(h, specular.x);
        boost::hash_combine(h, specular.y);
        boost::hash_combine(h, specular.z);
        boost::hash_combine(h, glossiness);
        boost::hash_combine(h, emissive.x);
        boost::hash_combine(h, emissive.y);
        boost::hash_combine(h, emissive.z);
        for(int i = 0;i < 7;i++)
        {
            if(!texName[i].empty())
            {
                boost::hash_combine(h, texName[i]);
                boost::hash_combine(h, texprop->textures[i].clamp);
                boost::hash_combine(h, texprop->textures[i].uvSet);
            }
        }
        boost::hash_combine(h, drawMode);
        boost::hash_combine(h, vertexColour);
        boost::hash_combine(h, alphaFlags);
        boost::hash_combine(h, alphaTest);
        boost::hash_combine(h, vertMode);
        boost::hash_combine(h, depthFlags);
        boost::hash_combine(h, specFlags);
        boost::hash_combine(h, wireFlags);

        std::map<size_t,std::string>::iterator itr = sMaterialMap.find(h);
        if (itr != sMaterialMap.end())
        {
            // a suitable material exists already - use it
            sh::MaterialInstance* instance = sh::Factory::getInstance().getMaterialInstance(itr->second);
            needTangents = !sh::retrieveValue<sh::StringValue>(instance->getProperty("normalMap"), instance).get().empty();
            return itr->second;
        }
        // not found, create a new one
        sMaterialMap.insert(std::make_pair(h, name));
    }

    // No existing material like this. Create a new one.
    sh::MaterialInstance *instance = sh::Factory::getInstance().createMaterialInstance(name, "openmw_objects_base");
    if(vertMode == 0 || !vertexColour)
    {
        instance->setProperty("ambient", sh::makeProperty(new sh::Vector4(ambient.x, ambient.y, ambient.z, 1)));
        instance->setProperty("diffuse", sh::makeProperty(new sh::Vector4(diffuse.x, diffuse.y, diffuse.z, alpha)));
        instance->setProperty("emissive", sh::makeProperty(new sh::Vector4(emissive.x, emissive.y, emissive.z, 1)));
        instance->setProperty("vertmode", sh::makeProperty(new sh::StringValue("0")));
    }
    else if(vertMode == 1)
    {
        instance->setProperty("ambient", sh::makeProperty(new sh::Vector4(ambient.x, ambient.y, ambient.z, 1)));
        instance->setProperty("diffuse", sh::makeProperty(new sh::Vector4(diffuse.x, diffuse.y, diffuse.z, alpha)));
        instance->setProperty("emissive", sh::makeProperty(new sh::StringValue("vertexcolour")));
        instance->setProperty("vertmode", sh::makeProperty(new sh::StringValue("1")));
    }
    else if(vertMode == 2)
    {
        instance->setProperty("ambient", sh::makeProperty(new sh::StringValue("vertexcolour")));
        instance->setProperty("diffuse", sh::makeProperty(new sh::StringValue("vertexcolour")));
        instance->setProperty("emissive", sh::makeProperty(new sh::Vector4(emissive.x, emissive.y, emissive.z, 1)));
        instance->setProperty("vertmode", sh::makeProperty(new sh::StringValue("2")));
    }
    else
        std::cerr<< "Unhandled vertex mode: "<<vertMode <<std::endl;

    if(specFlags)
    {
        instance->setProperty("specular", sh::makeProperty(
            new sh::Vector4(specular.x, specular.y, specular.z, glossiness)));
    }

    if(wireFlags)
    {
        instance->setProperty("polygon_mode", sh::makeProperty(new sh::StringValue("wireframe")));
    }

    if (drawMode == 1)
        instance->setProperty("cullmode", sh::makeProperty(new sh::StringValue("clockwise")));
    else if (drawMode == 2)
        instance->setProperty("cullmode", sh::makeProperty(new sh::StringValue("anticlockwise")));
    else if (drawMode == 3)
        instance->setProperty("cullmode", sh::makeProperty(new sh::StringValue("none")));

    instance->setProperty("diffuseMap", sh::makeProperty(texName[Nif::NiTexturingProperty::BaseTexture]));
    instance->setProperty("normalMap", sh::makeProperty(texName[Nif::NiTexturingProperty::BumpTexture]));
    instance->setProperty("detailMap", sh::makeProperty(texName[Nif::NiTexturingProperty::DetailTexture]));
    instance->setProperty("emissiveMap", sh::makeProperty(texName[Nif::NiTexturingProperty::GlowTexture]));
    instance->setProperty("darkMap", sh::makeProperty(texName[Nif::NiTexturingProperty::DarkTexture]));
    if (!texName[Nif::NiTexturingProperty::BaseTexture].empty())
    {
        instance->setProperty("use_diffuse_map", sh::makeProperty(new sh::BooleanValue(true)));
        setTextureProperties(instance, "diffuseMap", texprop->textures[Nif::NiTexturingProperty::BaseTexture]);
    }
    if (!texName[Nif::NiTexturingProperty::GlowTexture].empty())
    {
        instance->setProperty("use_emissive_map", sh::makeProperty(new sh::BooleanValue(true)));
        setTextureProperties(instance, "emissiveMap", texprop->textures[Nif::NiTexturingProperty::GlowTexture]);
    }
    if (!texName[Nif::NiTexturingProperty::DetailTexture].empty())
    {
        instance->setProperty("use_detail_map", sh::makeProperty(new sh::BooleanValue(true)));
        setTextureProperties(instance, "detailMap", texprop->textures[Nif::NiTexturingProperty::DetailTexture]);
    }
    if (!texName[Nif::NiTexturingProperty::DarkTexture].empty())
    {
        instance->setProperty("use_dark_map", sh::makeProperty(new sh::BooleanValue(true)));
        setTextureProperties(instance, "darkMap", texprop->textures[Nif::NiTexturingProperty::DarkTexture]);
    }

    bool useParallax = !texName[Nif::NiTexturingProperty::BumpTexture].empty()
            && texName[Nif::NiTexturingProperty::BumpTexture].find("_nh.") != std::string::npos;
    instance->setProperty("use_parallax", sh::makeProperty(new sh::BooleanValue(useParallax)));

    for(int i = 0;i < 7;i++)
    {
        if(i == Nif::NiTexturingProperty::BaseTexture ||
           i == Nif::NiTexturingProperty::DetailTexture ||
           i == Nif::NiTexturingProperty::DarkTexture ||
           i == Nif::NiTexturingProperty::BumpTexture ||
           i == Nif::NiTexturingProperty::GlowTexture)
            continue;
        if(!texName[i].empty())
            warn("Ignored texture "+texName[i]+" on layer "+Ogre::StringConverter::toString(i) + " in " + name);
    }

    if (vertexColour)
        instance->setProperty("has_vertex_colour", sh::makeProperty(new sh::BooleanValue(true)));

    // Override alpha flags based on our override list (transparency-overrides.cfg)
    if ((alphaFlags&1) && !texName[0].empty())
    {
        NifOverrides::TransparencyResult result = NifOverrides::Overrides::getTransparencyOverride(texName[0]);
        if (result.first)
        {
            alphaFlags = (1<<9) | (6<<10); /* alpha_rejection enabled, greater_equal */
            alphaTest = result.second;
            depthFlags = (1<<0) | (1<<1); // depth_write on, depth_check on
        }
    }

    // Add transparency if NiAlphaProperty was present
    if((alphaFlags&1))
    {
        std::string blend_mode;
        blend_mode += getBlendFactor((alphaFlags>>1)&0xf);
        blend_mode += " ";
        blend_mode += getBlendFactor((alphaFlags>>5)&0xf);
        instance->setProperty("scene_blend", sh::makeProperty(new sh::StringValue(blend_mode)));
    }

    if((alphaFlags>>9)&1)
    {
#ifndef ANDROID
        std::string reject;
        reject += getTestMode((alphaFlags>>10)&0x7);
        reject += " ";
        reject += Ogre::StringConverter::toString(alphaTest);
        instance->setProperty("alpha_rejection", sh::makeProperty(new sh::StringValue(reject)));
#else
        // alpha test not supported in OpenGL ES 2, use manual implementation in shader
        instance->setProperty("alphaTestMode", sh::makeProperty(new sh::IntValue((alphaFlags>>10)&0x7)));
        instance->setProperty("alphaTestValue", sh::makeProperty(new sh::FloatValue(alphaTest/255.f)));
#endif
    }
    else
        instance->getMaterial()->setShadowCasterMaterial("openmw_shadowcaster_noalpha");

    // Ogre usually only sorts if depth write is disabled, so we want "force" instead of "on"
    instance->setProperty("transparent_sorting", sh::makeProperty(new sh::StringValue(
        ((alphaFlags&1) && !((alphaFlags>>13)&1)) ? "force" : "off")));

    instance->setProperty("depth_check", sh::makeProperty(new sh::StringValue((depthFlags&1) ? "on" : "off")));
    instance->setProperty("depth_write", sh::makeProperty(new sh::StringValue(((depthFlags>>1)&1) ? "on" : "off")));
    // depth_func???

    if (!texName[0].empty())
        NifOverrides::Overrides::getMaterialOverrides(texName[0], instance);

    // Don't use texName, as it may be overridden
    needTangents = !sh::retrieveValue<sh::StringValue>(instance->getProperty("normalMap"), instance).get().empty();

    return name;
}

std::map<size_t,std::string> NIFMaterialLoader::sMaterialMap;

}
