#include "material.hpp"

#include <OgreMaterialManager.h>
#include <OgreTechnique.h>
#include <OgrePass.h>

#include <extern/shiny/Main/Factory.hpp>

namespace
{

int getBlendmapIndexForLayer (int layerIndex)
{
    return std::floor((layerIndex-1)/4.f);
}

std::string getBlendmapComponentForLayer (int layerIndex)
{
    int n = (layerIndex-1)%4;
    if (n == 0)
        return "x";
    if (n == 1)
        return "y";
    if (n == 2)
        return "z";
    else
        return "w";
}

}

namespace Terrain
{

    MaterialGenerator::MaterialGenerator(bool shaders)
        : mShaders(shaders)
        , mShadows(false)
        , mSplitShadows(false)
    {

    }

    int MaterialGenerator::getMaxLayersPerPass ()
    {
        // count the texture units free
        Ogre::uint8 freeTextureUnits = 16;

        // first layer doesn't need blendmap
        --freeTextureUnits;

        if (mSplitShadows)
            freeTextureUnits -= 3;
        else if (mShadows)
            --freeTextureUnits;

        // each layer needs 1.25 units (1xdiffusespec, 0.25xblend)
        return static_cast<Ogre::uint8>(freeTextureUnits / (1.25f)) + 1;
    }

    int MaterialGenerator::getRequiredPasses ()
    {
        int maxLayersPerPass = getMaxLayersPerPass();
        return std::max(1.f, std::ceil(static_cast<float>(mLayerList.size()) / maxLayersPerPass));
    }

    Ogre::MaterialPtr MaterialGenerator::generate(Ogre::MaterialPtr mat)
    {
        return create(mat, false, false);
    }

    Ogre::MaterialPtr MaterialGenerator::generateForCompositeMapRTT(Ogre::MaterialPtr mat)
    {
        return create(mat, true, false);
    }

    Ogre::MaterialPtr MaterialGenerator::generateForCompositeMap(Ogre::MaterialPtr mat)
    {
        return create(mat, false, true);
    }

    Ogre::MaterialPtr MaterialGenerator::create(Ogre::MaterialPtr mat, bool renderCompositeMap, bool displayCompositeMap)
    {
        assert(!renderCompositeMap || !displayCompositeMap);
        if (!mat.isNull())
        {
            sh::Factory::getInstance().destroyMaterialInstance(mat->getName());
            Ogre::MaterialManager::getSingleton().remove(mat->getName());
        }

        static int count = 0;
        std::stringstream name;
        name << "terrain/mat" << count++;

        if (!mShaders)
        {
            mat = Ogre::MaterialManager::getSingleton().create(name.str(),
                                                               Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
            Ogre::Technique* technique = mat->getTechnique(0);
            technique->removeAllPasses();

            if (displayCompositeMap)
            {
                Ogre::Pass* pass = technique->createPass();
                pass->setVertexColourTracking(Ogre::TVC_AMBIENT|Ogre::TVC_DIFFUSE);
                pass->createTextureUnitState(mCompositeMap)->setTextureAddressingMode(Ogre::TextureUnitState::TAM_CLAMP);
            }
            else
            {
                assert(mLayerList.size() == mBlendmapList.size()+1);
                std::vector<Ogre::TexturePtr>::iterator blend = mBlendmapList.begin();
                for (std::vector<std::string>::iterator layer = mLayerList.begin(); layer != mLayerList.end(); ++layer)
                {
                    Ogre::Pass* pass = technique->createPass();
                    pass->setLightingEnabled(false);
                    pass->setVertexColourTracking(Ogre::TVC_NONE);
                    // TODO: How to handle fog?
                    pass->setFog(true, Ogre::FOG_NONE);

                    bool first = (layer == mLayerList.begin());

                    Ogre::TextureUnitState* tus;

                    if (!first)
                    {
                        pass->setSceneBlending(Ogre::SBT_TRANSPARENT_ALPHA);
                        pass->setDepthFunction(Ogre::CMPF_EQUAL);

                        tus = pass->createTextureUnitState((*blend)->getName());
                        tus->setAlphaOperation(Ogre::LBX_BLEND_TEXTURE_ALPHA,
                                               Ogre::LBS_TEXTURE,
                                               Ogre::LBS_TEXTURE);
                        tus->setColourOperationEx(Ogre::LBX_BLEND_DIFFUSE_ALPHA,
                                                  Ogre::LBS_TEXTURE,
                                                  Ogre::LBS_TEXTURE);
                        tus->setIsAlpha(true);
                        tus->setTextureAddressingMode(Ogre::TextureUnitState::TAM_CLAMP);

                        float scale = (16/(16.f+1.f));
                        tus->setTextureScale(1.f/scale,1.f/scale);
                    }

                    // Add the actual layer texture on top of the alpha map.
                    tus = pass->createTextureUnitState("textures\\" + *layer);
                    if (!first)
                        tus->setColourOperationEx(Ogre::LBX_BLEND_DIFFUSE_ALPHA,
                                                  Ogre::LBS_TEXTURE,
                                                  Ogre::LBS_CURRENT);

                    tus->setTextureScale(1/16.f,1/16.f);

                    if (!first)
                        ++blend;
                }

                if (!renderCompositeMap)
                {
                    Ogre::Pass* lightingPass = technique->createPass();
                    lightingPass->setSceneBlending(Ogre::SBT_MODULATE);
                    lightingPass->setVertexColourTracking(Ogre::TVC_AMBIENT|Ogre::TVC_DIFFUSE);
                    lightingPass->setFog(true, Ogre::FOG_NONE);
                }
            }

            return mat;
        }
        else
        {
            sh::MaterialInstance* material = sh::Factory::getInstance().createMaterialInstance (name.str());
            material->setProperty ("allow_fixed_function", sh::makeProperty<sh::BooleanValue>(new sh::BooleanValue(false)));

            if (displayCompositeMap)
            {
                sh::MaterialInstancePass* p = material->createPass ();

                p->setProperty ("vertex_program", sh::makeProperty<sh::StringValue>(new sh::StringValue("terrain_vertex")));
                p->setProperty ("fragment_program", sh::makeProperty<sh::StringValue>(new sh::StringValue("terrain_fragment")));
                p->mShaderProperties.setProperty ("is_first_pass", sh::makeProperty(new sh::BooleanValue(true)));
                p->mShaderProperties.setProperty ("render_composite_map", sh::makeProperty(new sh::BooleanValue(false)));
                p->mShaderProperties.setProperty ("display_composite_map", sh::makeProperty(new sh::BooleanValue(true)));
                p->mShaderProperties.setProperty ("num_layers", sh::makeProperty (new sh::StringValue("0")));
                p->mShaderProperties.setProperty ("num_blendmaps", sh::makeProperty (new sh::StringValue("0")));

                sh::MaterialInstanceTextureUnit* tex = p->createTextureUnit ("compositeMap");
                tex->setProperty ("direct_texture", sh::makeProperty (new sh::StringValue(mCompositeMap)));
                tex->setProperty ("tex_address_mode", sh::makeProperty (new sh::StringValue("clamp")));

                // shadow. TODO: repeated, put in function
                if (mShadows)
                {
                    for (Ogre::uint i = 0; i < (mSplitShadows ? 3 : 1); ++i)
                    {
                        sh::MaterialInstanceTextureUnit* shadowTex = p->createTextureUnit ("shadowMap" + Ogre::StringConverter::toString(i));
                        shadowTex->setProperty ("content_type", sh::makeProperty<sh::StringValue> (new sh::StringValue("shadow")));
                    }
                }
                p->mShaderProperties.setProperty ("shadowtexture_offset", sh::makeProperty (new sh::StringValue(
                    Ogre::StringConverter::toString(1))));

                p->mShaderProperties.setProperty ("pass_index", sh::makeProperty(new sh::IntValue(0)));
            }
            else
            {

                int numPasses = getRequiredPasses();
                assert(numPasses);
                int maxLayersInOnePass = getMaxLayersPerPass();

                for (int pass=0; pass<numPasses; ++pass)
                {
                    int layerOffset = maxLayersInOnePass * pass;
                    int blendmapOffset = (pass == 0) ? 1 : 0; // the first layer of the first pass is the base layer and does not need a blend map

                    sh::MaterialInstancePass* p = material->createPass ();

                    p->setProperty ("vertex_program", sh::makeProperty<sh::StringValue>(new sh::StringValue("terrain_vertex")));
                    p->setProperty ("fragment_program", sh::makeProperty<sh::StringValue>(new sh::StringValue("terrain_fragment")));
                    if (pass != 0)
                    {
                        p->setProperty ("scene_blend", sh::makeProperty(new sh::StringValue("alpha_blend")));
                        // Only write if depth is equal to the depth value written by the previous pass.
                        p->setProperty ("depth_func", sh::makeProperty(new sh::StringValue("equal")));
                    }

                    p->mShaderProperties.setProperty ("is_first_pass", sh::makeProperty(new sh::BooleanValue(pass == 0)));
                    p->mShaderProperties.setProperty ("render_composite_map", sh::makeProperty(new sh::BooleanValue(renderCompositeMap)));
                    p->mShaderProperties.setProperty ("display_composite_map", sh::makeProperty(new sh::BooleanValue(displayCompositeMap)));

                    Ogre::uint numLayersInThisPass = std::min(maxLayersInOnePass, (int)mLayerList.size()-layerOffset);

                    // a blend map might be shared between two passes
                    Ogre::uint numBlendTextures=0;
                    std::vector<std::string> blendTextures;
                    for (unsigned int layer=blendmapOffset; layer<numLayersInThisPass; ++layer)
                    {
                        std::string blendTextureName = mBlendmapList[getBlendmapIndexForLayer(layerOffset+layer)]->getName();
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
                    if (mLayerList.size() == 1) // special case. if there's only one layer, we don't need blend maps at all
                        blendmapStart = 0;
                    else
                        blendmapStart = getBlendmapIndexForLayer(layerOffset+blendmapOffset);
                    for (Ogre::uint i = 0; i < numBlendTextures; ++i)
                    {
                        sh::MaterialInstanceTextureUnit* blendTex = p->createTextureUnit ("blendMap" + Ogre::StringConverter::toString(i));
                        blendTex->setProperty ("direct_texture", sh::makeProperty (new sh::StringValue(mBlendmapList[blendmapStart+i]->getName())));
                        blendTex->setProperty ("tex_address_mode", sh::makeProperty (new sh::StringValue("clamp")));
                    }

                    // layer maps
                    for (Ogre::uint i = 0; i < numLayersInThisPass; ++i)
                    {
                        sh::MaterialInstanceTextureUnit* diffuseTex = p->createTextureUnit ("diffuseMap" + Ogre::StringConverter::toString(i));
                        diffuseTex->setProperty ("direct_texture", sh::makeProperty (new sh::StringValue("textures\\"+mLayerList[layerOffset+i])));

                        if (i+layerOffset > 0)
                        {
                            int blendTextureIndex = getBlendmapIndexForLayer(layerOffset+i);
                            std::string blendTextureComponent = getBlendmapComponentForLayer(layerOffset+i);
                            p->mShaderProperties.setProperty ("blendmap_component_" + Ogre::StringConverter::toString(i),
                                sh::makeProperty (new sh::StringValue(Ogre::StringConverter::toString(blendTextureIndex-blendmapStart) + "." + blendTextureComponent)));
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
                    if (mShadows)
                    {
                        for (Ogre::uint i = 0; i < (mSplitShadows ? 3 : 1); ++i)
                        {
                            sh::MaterialInstanceTextureUnit* shadowTex = p->createTextureUnit ("shadowMap" + Ogre::StringConverter::toString(i));
                            shadowTex->setProperty ("content_type", sh::makeProperty<sh::StringValue> (new sh::StringValue("shadow")));
                        }
                    }
                    p->mShaderProperties.setProperty ("shadowtexture_offset", sh::makeProperty (new sh::StringValue(
                        Ogre::StringConverter::toString(numBlendTextures + numLayersInThisPass))));

                    // Make sure the pass index is fed to the permutation handler, because blendmap components may be different
                    p->mShaderProperties.setProperty ("pass_index", sh::makeProperty(new sh::IntValue(pass)));
                }
            }
        }
        return Ogre::MaterialManager::getSingleton().getByName(name.str());
    }

}
