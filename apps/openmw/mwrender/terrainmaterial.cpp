#include "terrainmaterial.hpp"

#include <OgreTerrain.h>

#include <stdexcept>

#include <extern/shiny/Main/Factory.hpp>

namespace
{
    Ogre::String getComponent (int num)
    {
        if (num == 0)
            return "x";
        else if (num == 1)
            return "y";
        else if (num == 2)
            return "z";
        else
            return "w";
    }
}


namespace MWRender
{

    TerrainMaterial::TerrainMaterial()
    {
        mLayerDecl.samplers.push_back(Ogre::TerrainLayerSampler("albedo_specular", Ogre::PF_BYTE_RGBA));
        //mLayerDecl.samplers.push_back(Ogre::TerrainLayerSampler("normal_height", Ogre::PF_BYTE_RGBA));

        mLayerDecl.elements.push_back(
            Ogre::TerrainLayerSamplerElement(0, Ogre::TLSS_ALBEDO, 0, 3));
        //mLayerDecl.elements.push_back(
        //	Ogre::TerrainLayerSamplerElement(0, Ogre::TLSS_SPECULAR, 3, 1));
        //mLayerDecl.elements.push_back(
        //	Ogre::TerrainLayerSamplerElement(1, Ogre::TLSS_NORMAL, 0, 3));
        //mLayerDecl.elements.push_back(
        //	Ogre::TerrainLayerSamplerElement(1, Ogre::TLSS_HEIGHT, 3, 1));


        mProfiles.push_back(OGRE_NEW Profile(this, "SM2", "Profile for rendering on Shader Model 2 capable cards"));
        setActiveProfile("SM2");
    }

    // -----------------------------------------------------------------------------------------------------------------------

    TerrainMaterial::Profile::Profile(Ogre::TerrainMaterialGenerator* parent, const Ogre::String& name, const Ogre::String& desc)
        : Ogre::TerrainMaterialGenerator::Profile(parent, name, desc)
        , mGlobalColourMap(false)
        , mMaterial(0)
    {
    }

    TerrainMaterial::Profile::~Profile()
    {
        if (mMaterial)
            sh::Factory::getInstance().destroyMaterialInstance(mMaterial->getName());
    }

    Ogre::MaterialPtr TerrainMaterial::Profile::generate(const Ogre::Terrain* terrain)
    {
        const Ogre::String& matName = terrain->getMaterialName();

        sh::Factory::getInstance().destroyMaterialInstance (matName);

        Ogre::MaterialPtr mat = Ogre::MaterialManager::getSingleton().getByName(matName);
        if (!mat.isNull())
            Ogre::MaterialManager::getSingleton().remove(matName);

        mMaterial = sh::Factory::getInstance().createMaterialInstance (matName);
        mMaterial->setProperty ("allow_fixed_function", sh::makeProperty<sh::BooleanValue>(new sh::BooleanValue(false)));

        int numPasses = getRequiredPasses(terrain);
        int maxLayersInOnePass = getMaxLayersPerPass(terrain);

        for (int pass=0; pass<numPasses; ++pass)
        {
            int layerOffset = maxLayersInOnePass * pass;
            int blendmapOffset = (pass == 0) ? 1 : 0; // the first layer of the first pass is the base layer and does not need a blend map

            sh::MaterialInstancePass* p = mMaterial->createPass ();

            p->setProperty ("vertex_program", sh::makeProperty<sh::StringValue>(new sh::StringValue("terrain_vertex")));
            p->setProperty ("fragment_program", sh::makeProperty<sh::StringValue>(new sh::StringValue("terrain_fragment")));
            if (pass != 0)
            {
                p->setProperty ("scene_blend", sh::makeProperty(new sh::StringValue("alpha_blend")));
                // Only write if depth is equal to the depth value written by the previous pass.
                p->setProperty ("depth_func", sh::makeProperty(new sh::StringValue("equal")));
            }

            p->mShaderProperties.setProperty ("colour_map", sh::makeProperty(new sh::BooleanValue(mGlobalColourMap)));
            p->mShaderProperties.setProperty ("is_first_pass", sh::makeProperty(new sh::BooleanValue(pass == 0)));

            // global colour map
            sh::MaterialInstanceTextureUnit* colourMap = p->createTextureUnit ("colourMap");
            colourMap->setProperty ("texture_alias", sh::makeProperty<sh::StringValue> (new sh::StringValue(mMaterial->getName() + "_colourMap")));
            colourMap->setProperty ("tex_address_mode", sh::makeProperty<sh::StringValue> (new sh::StringValue("clamp")));

            // global normal map
            sh::MaterialInstanceTextureUnit* normalMap = p->createTextureUnit ("normalMap");
            normalMap->setProperty ("direct_texture", sh::makeProperty<sh::StringValue> (new sh::StringValue(terrain->getTerrainNormalMap ()->getName())));
            normalMap->setProperty ("tex_address_mode", sh::makeProperty<sh::StringValue> (new sh::StringValue("clamp")));

            Ogre::uint numLayersInThisPass = std::min(maxLayersInOnePass, terrain->getLayerCount()-layerOffset);

            // HACK: Terrain::getLayerBlendTextureIndex should be const, but it is not.
            // Remove this once ogre got fixed.
            Ogre::Terrain* nonconstTerrain = const_cast<Ogre::Terrain*>(terrain);

            // a blend map might be shared between two passes
            // so we can't just use terrain->getBlendTextureCount()
            Ogre::uint numBlendTextures=0;
            std::vector<std::string> blendTextures;
            for (unsigned int layer=blendmapOffset; layer<numLayersInThisPass; ++layer)
            {
                std::string blendTextureName = terrain->getBlendTextureName(nonconstTerrain->getLayerBlendTextureIndex(
                                                                                static_cast<Ogre::uint8>(layerOffset+layer)).first);
                if (std::find(blendTextures.begin(), blendTextures.end(), blendTextureName) == blendTextures.end())
                {
                    blendTextures.push_back(blendTextureName);
                    ++numBlendTextures;
                }
            }

            p->mShaderProperties.setProperty ("num_layers", sh::makeProperty (new sh::StringValue(Ogre::StringConverter::toString(numLayersInThisPass))));
            p->mShaderProperties.setProperty ("num_blendmaps", sh::makeProperty (new sh::StringValue(Ogre::StringConverter::toString(numBlendTextures))));

            // blend maps
            // the index of the first blend map used in this pass
            int blendmapStart;
            if (terrain->getLayerCount() == 1) // special case. if there's only one layer, we don't need blend maps at all
                blendmapStart = 0;
            else
                blendmapStart = nonconstTerrain->getLayerBlendTextureIndex(static_cast<Ogre::uint8>(layerOffset+blendmapOffset)).first;
            for (Ogre::uint i = 0; i < numBlendTextures; ++i)
            {
                sh::MaterialInstanceTextureUnit* blendTex = p->createTextureUnit ("blendMap" + Ogre::StringConverter::toString(i));
                blendTex->setProperty ("direct_texture", sh::makeProperty (new sh::StringValue(terrain->getBlendTextureName(blendmapStart+i))));
                blendTex->setProperty ("tex_address_mode", sh::makeProperty (new sh::StringValue("clamp")));
            }

            // layer maps
            for (Ogre::uint i = 0; i < numLayersInThisPass; ++i)
            {
                sh::MaterialInstanceTextureUnit* diffuseTex = p->createTextureUnit ("diffuseMap" + Ogre::StringConverter::toString(i));
                diffuseTex->setProperty ("direct_texture", sh::makeProperty (new sh::StringValue(terrain->getLayerTextureName(layerOffset+i, 0))));

                if (i+layerOffset > 0)
                {
                    int blendTextureIndex = nonconstTerrain->getLayerBlendTextureIndex(static_cast<Ogre::uint8>(layerOffset+i)).first;
                    int blendTextureComponent = nonconstTerrain->getLayerBlendTextureIndex(static_cast<Ogre::uint8>(layerOffset+i)).second;
                    p->mShaderProperties.setProperty ("blendmap_component_" + Ogre::StringConverter::toString(i),
                        sh::makeProperty (new sh::StringValue(Ogre::StringConverter::toString(blendTextureIndex-blendmapStart) + "." + getComponent(blendTextureComponent))));
                }
                else
                {
                    // just to make it shut up about blendmap_component_0 not existing in the first pass.
                    // it might be retrieved, but will never survive the preprocessing step.
                    p->mShaderProperties.setProperty ("blendmap_component_" + Ogre::StringConverter::toString(i),
                        sh::makeProperty (new sh::StringValue("")));
                }
            }

            // shadow
            for (Ogre::uint i = 0; i < 3; ++i)
            {
                sh::MaterialInstanceTextureUnit* shadowTex = p->createTextureUnit ("shadowMap" + Ogre::StringConverter::toString(i));
                shadowTex->setProperty ("content_type", sh::makeProperty<sh::StringValue> (new sh::StringValue("shadow")));
            }

            p->mShaderProperties.setProperty ("shadowtexture_offset", sh::makeProperty (new sh::StringValue(
                Ogre::StringConverter::toString(numBlendTextures + numLayersInThisPass + 2))));

            // make sure the pass index is fed to the permutation handler, because blendmap components may be different
            p->mShaderProperties.setProperty ("pass_index", sh::makeProperty(new sh::IntValue(pass)));
        }

        return Ogre::MaterialManager::getSingleton().getByName(matName);
    }

    void TerrainMaterial::Profile::setGlobalColourMapEnabled (bool enabled)
    {
        mGlobalColourMap = enabled;
        mParent->_markChanged();
    }

    void TerrainMaterial::Profile::setGlobalColourMap (Ogre::Terrain* terrain, const std::string& name)
    {
        sh::Factory::getInstance ().setTextureAlias (terrain->getMaterialName () + "_colourMap", name);
    }

    Ogre::MaterialPtr TerrainMaterial::Profile::generateForCompositeMap(const Ogre::Terrain* terrain)
    {
        throw std::runtime_error ("composite map not supported");
    }

    Ogre::uint8 TerrainMaterial::Profile::getMaxLayers(const Ogre::Terrain* terrain) const
    {
        return 255;
    }

    int TerrainMaterial::Profile::getMaxLayersPerPass (const Ogre::Terrain* terrain)
    {
        // count the texture units free
        Ogre::uint8 freeTextureUnits = 16;
        // normalmap
        --freeTextureUnits;
        // colourmap
        --freeTextureUnits;
        // shadow
        --freeTextureUnits;
        --freeTextureUnits;
        --freeTextureUnits;

        // each layer needs 1.25 units (1xdiffusespec, 0.25xblend)
        return static_cast<Ogre::uint8>(freeTextureUnits / (1.25f));
    }

    int TerrainMaterial::Profile::getRequiredPasses (const Ogre::Terrain* terrain)
    {
        int maxLayersPerPass = getMaxLayersPerPass(terrain);
        assert(terrain->getLayerCount());
        assert(maxLayersPerPass);
        return std::ceil(static_cast<float>(terrain->getLayerCount()) / maxLayersPerPass);
    }

    void TerrainMaterial::Profile::updateParams(const Ogre::MaterialPtr& mat, const Ogre::Terrain* terrain)
    {
    }

    void TerrainMaterial::Profile::updateParamsForCompositeMap(const Ogre::MaterialPtr& mat, const Ogre::Terrain* terrain)
    {
    }

    void TerrainMaterial::Profile::requestOptions(Ogre::Terrain* terrain)
    {
        terrain->_setMorphRequired(true);
        terrain->_setNormalMapRequired(true); // global normal map
        terrain->_setLightMapRequired(false);
        terrain->_setCompositeMapRequired(false);
    }

}
