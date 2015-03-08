/*
 * Copyright (c) 2015 scrawl <scrawl@baseoftrash.de>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include "material.hpp"

#include <OgreMaterialManager.h>
#include <OgreTechnique.h>
#include <OgrePass.h>

#include <boost/functional/hash.hpp>

#if TERRAIN_USE_SHADER
#include <extern/shiny/Main/Factory.hpp>
#endif

namespace
{

int getBlendmapIndexForLayer (int layerIndex)
{
    return static_cast<int>(std::floor((layerIndex - 1) / 4.f));
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

    MaterialGenerator::MaterialGenerator()
        : mShaders(true)
        , mShadows(false)
        , mSplitShadows(false)
        , mNormalMapping(true)
        , mParallaxMapping(true)
    {

    }

    Ogre::MaterialPtr MaterialGenerator::generate()
    {
        assert(!mLayerList.empty() && "Can't create material with no layers");

        return create(false, false);
    }

    Ogre::MaterialPtr MaterialGenerator::generateForCompositeMapRTT()
    {
        assert(!mLayerList.empty() && "Can't create material with no layers");

        return create(true, false);
    }

    Ogre::MaterialPtr MaterialGenerator::generateForCompositeMap()
    {
        return create(false, true);
    }

    Ogre::MaterialPtr MaterialGenerator::create(bool renderCompositeMap, bool displayCompositeMap)
    {
        assert(!renderCompositeMap || !displayCompositeMap);

        static int count = 0;
        std::stringstream name;
        name << "terrain/mat" << count++;

        if (!mShaders)
        {
            Ogre::MaterialPtr mat = Ogre::MaterialManager::getSingleton().create(name.str(),
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
                for (std::vector<LayerInfo>::iterator layer = mLayerList.begin(); layer != mLayerList.end(); ++layer)
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
                    tus = pass->createTextureUnitState(layer->mDiffuseMap);
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
#if TERRAIN_USE_SHADER
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
                p->mShaderProperties.setProperty ("normal_map_enabled", sh::makeProperty (new sh::BooleanValue(false)));
                p->mShaderProperties.setProperty ("parallax_enabled", sh::makeProperty (new sh::BooleanValue(false)));
                p->mShaderProperties.setProperty ("normal_maps",
                                                  sh::makeProperty (new sh::IntValue(0)));

                sh::MaterialInstanceTextureUnit* tex = p->createTextureUnit ("compositeMap");
                tex->setProperty ("direct_texture", sh::makeProperty (new sh::StringValue(mCompositeMap)));
                tex->setProperty ("tex_address_mode", sh::makeProperty (new sh::StringValue("clamp")));

                // shadow. TODO: repeated, put in function
                if (mShadows)
                {
                    for (int i = 0; i < (mSplitShadows ? 3 : 1); ++i)
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

                bool shadows = mShadows && !renderCompositeMap;

                int layerOffset = 0;
                while (layerOffset < (int)mLayerList.size())
                {
                    int blendmapOffset = (layerOffset == 0) ? 1 : 0; // the first layer of the first pass is the base layer and does not need a blend map

                    // Check how many layers we can fit in this pass
                    int numLayersInThisPass = 0;
                    int numBlendTextures = 0;
                    std::vector<std::string> blendTextures;
                    int remainingTextureUnits = OGRE_MAX_TEXTURE_LAYERS;
                    if (shadows)
                        remainingTextureUnits -= (mSplitShadows ? 3 : 1);
                    while (remainingTextureUnits && layerOffset + numLayersInThisPass < (int)mLayerList.size())
                    {
                        int layerIndex = numLayersInThisPass + layerOffset;

                        int neededTextureUnits=0;
                        int neededBlendTextures=0;

                        if (layerIndex != 0)
                        {
                            std::string blendTextureName = mBlendmapList[getBlendmapIndexForLayer(layerIndex)]->getName();
                            if (std::find(blendTextures.begin(), blendTextures.end(), blendTextureName) == blendTextures.end())
                            {
                                blendTextures.push_back(blendTextureName);
                                ++neededBlendTextures;
                                ++neededTextureUnits; // blend texture
                            }
                        }
                        ++neededTextureUnits; // layer texture

                        // Check if this layer has a normal map
                        if (mNormalMapping && !mLayerList[layerIndex].mNormalMap.empty() && !renderCompositeMap)
                            ++neededTextureUnits; // normal map
                        if (neededTextureUnits <= remainingTextureUnits)
                        {
                            // We can fit another!
                            remainingTextureUnits -= neededTextureUnits;
                            numBlendTextures += neededBlendTextures;
                            ++numLayersInThisPass;
                        }
                        else
                            break; // We're full
                    }


                    sh::MaterialInstancePass* p = material->createPass ();
                    p->setProperty ("vertex_program", sh::makeProperty<sh::StringValue>(new sh::StringValue("terrain_vertex")));
                    p->setProperty ("fragment_program", sh::makeProperty<sh::StringValue>(new sh::StringValue("terrain_fragment")));
                    if (layerOffset != 0)
                    {
                        p->setProperty ("scene_blend", sh::makeProperty(new sh::StringValue("alpha_blend")));
                        // Only write if depth is equal to the depth value written by the previous pass.
                        p->setProperty ("depth_func", sh::makeProperty(new sh::StringValue("equal")));
                    }

                    p->mShaderProperties.setProperty ("render_composite_map", sh::makeProperty(new sh::BooleanValue(renderCompositeMap)));
                    p->mShaderProperties.setProperty ("display_composite_map", sh::makeProperty(new sh::BooleanValue(displayCompositeMap)));

                    p->mShaderProperties.setProperty ("num_layers", sh::makeProperty (new sh::StringValue(Ogre::StringConverter::toString(numLayersInThisPass))));
                    p->mShaderProperties.setProperty ("num_blendmaps", sh::makeProperty (new sh::StringValue(Ogre::StringConverter::toString(numBlendTextures))));
                    p->mShaderProperties.setProperty ("normal_map_enabled",
                                                      sh::makeProperty (new sh::BooleanValue(false)));

                    // blend maps
                    // the index of the first blend map used in this pass
                    int blendmapStart;
                    if (mLayerList.size() == 1) // special case. if there's only one layer, we don't need blend maps at all
                        blendmapStart = 0;
                    else
                        blendmapStart = getBlendmapIndexForLayer(layerOffset+blendmapOffset);
                    for (int i = 0; i < numBlendTextures; ++i)
                    {
                        sh::MaterialInstanceTextureUnit* blendTex = p->createTextureUnit ("blendMap" + Ogre::StringConverter::toString(i));
                        blendTex->setProperty ("direct_texture", sh::makeProperty (new sh::StringValue(mBlendmapList[blendmapStart+i]->getName())));
                        blendTex->setProperty ("tex_address_mode", sh::makeProperty (new sh::StringValue("clamp")));
                    }

                    // layer maps
                    bool anyNormalMaps = false;
                    bool anyParallax = false;
                    size_t normalMaps = 0;
                    for (int i = 0; i < numLayersInThisPass; ++i)
                    {
                        const LayerInfo& layer = mLayerList[layerOffset+i];
                        // diffuse map
                        sh::MaterialInstanceTextureUnit* diffuseTex = p->createTextureUnit ("diffuseMap" + Ogre::StringConverter::toString(i));
                        diffuseTex->setProperty ("direct_texture", sh::makeProperty (new sh::StringValue(layer.mDiffuseMap)));

                        // normal map (optional)
                        bool useNormalMap = mNormalMapping && !mLayerList[layerOffset+i].mNormalMap.empty() && !renderCompositeMap;
                        bool useParallax = useNormalMap && mParallaxMapping && layer.mParallax;
                        bool useSpecular = layer.mSpecular;
                        if (useNormalMap)
                        {
                            anyNormalMaps = true;
                            anyParallax = anyParallax || useParallax;
                            sh::MaterialInstanceTextureUnit* normalTex = p->createTextureUnit ("normalMap" + Ogre::StringConverter::toString(i));
                            normalTex->setProperty ("direct_texture", sh::makeProperty (new sh::StringValue(layer.mNormalMap)));
                        }
                        p->mShaderProperties.setProperty ("use_normal_map_" + Ogre::StringConverter::toString(i),
                                                          sh::makeProperty (new sh::BooleanValue(useNormalMap)));
                        p->mShaderProperties.setProperty ("use_parallax_" + Ogre::StringConverter::toString(i),
                                                          sh::makeProperty (new sh::BooleanValue(useParallax)));
                        p->mShaderProperties.setProperty ("use_specular_" + Ogre::StringConverter::toString(i),
                                                          sh::makeProperty (new sh::BooleanValue(useSpecular)));
                        boost::hash_combine(normalMaps, useNormalMap);
                        boost::hash_combine(normalMaps, useNormalMap && layer.mParallax);
                        boost::hash_combine(normalMaps, useSpecular);

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
                    p->mShaderProperties.setProperty ("normal_map_enabled",
                                                      sh::makeProperty (new sh::BooleanValue(anyNormalMaps)));
                    p->mShaderProperties.setProperty ("parallax_enabled",
                                                      sh::makeProperty (new sh::BooleanValue(anyParallax)));
                    // Since the permutation handler can't handle dynamic property names,
                    // combine normal map settings for all layers into one value
                    p->mShaderProperties.setProperty ("normal_maps",
                                                      sh::makeProperty (new sh::IntValue(normalMaps)));

                    // shadow
                    if (shadows)
                    {
                        for (int i = 0; i < (mSplitShadows ? 3 : 1); ++i)
                        {
                            sh::MaterialInstanceTextureUnit* shadowTex = p->createTextureUnit ("shadowMap" + Ogre::StringConverter::toString(i));
                            shadowTex->setProperty ("content_type", sh::makeProperty<sh::StringValue> (new sh::StringValue("shadow")));
                        }
                    }
                    p->mShaderProperties.setProperty ("shadowtexture_offset", sh::makeProperty (new sh::StringValue(
                        Ogre::StringConverter::toString(numBlendTextures + numLayersInThisPass))));

                    // Make sure the pass index is fed to the permutation handler, because blendmap components may be different
                    p->mShaderProperties.setProperty ("pass_index", sh::makeProperty(new sh::IntValue(layerOffset)));

                    assert ((int)p->mTexUnits.size() == OGRE_MAX_TEXTURE_LAYERS - remainingTextureUnits);

                    layerOffset += numLayersInThisPass;
                }
            }
        }
#endif
        return Ogre::MaterialManager::getSingleton().getByName(name.str());
    }

}
