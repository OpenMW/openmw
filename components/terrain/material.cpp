#include "material.hpp"

#include <iostream>
#include <stdexcept>

#include <osg/Depth>
#include <osg/TexEnvCombine>
#include <osg/Texture2D>
#include <osg/TexMat>
#include <osg/Material>

#include <components/shader/shadermanager.hpp>


namespace Terrain
{

    osg::ref_ptr<osg::TexMat> getBlendmapTexMat(int blendmapScale)
    {
        static std::map<int, osg::ref_ptr<osg::TexMat> > texMatMap;
        osg::ref_ptr<osg::TexMat> texMat = texMatMap[blendmapScale];
        if (!texMat)
        {
            texMat = new osg::TexMat;
            osg::Matrixf matrix;
            float scale = (blendmapScale/(static_cast<float>(blendmapScale)+1.f));
            matrix.preMultTranslate(osg::Vec3f(0.5f, 0.5f, 0.f));
            matrix.preMultScale(osg::Vec3f(scale, scale, 1.f));
            matrix.preMultTranslate(osg::Vec3f(-0.5f, -0.5f, 0.f));

            texMatMap[blendmapScale] = texMat;
        }
        return texMat;
    }

    osg::ref_ptr<osg::TexMat> getLayerTexMat(float layerTileSize)
    {
        static std::map<float, osg::ref_ptr<osg::TexMat> > texMatMap;
        osg::ref_ptr<osg::TexMat> texMat = texMatMap[layerTileSize];
        if (!texMat)
        {
            texMat = new osg::TexMat;
            texMat->setMatrix(osg::Matrix::scale(osg::Vec3f(layerTileSize,layerTileSize,1.f)));

            texMatMap[layerTileSize] = texMat;
        }
        return texMat;
    }

    osg::ref_ptr<osg::Depth> getEqualDepth()
    {
        static osg::ref_ptr<osg::Depth> depth;
        if (!depth)
        {
            depth = new osg::Depth;
            depth->setFunction(osg::Depth::EQUAL);
        }
        return depth;
    }

    FixedFunctionTechnique::FixedFunctionTechnique(const std::vector<TextureLayer>& layers,
                                                   const std::vector<osg::ref_ptr<osg::Texture2D> >& blendmaps, int blendmapScale, float layerTileSize)
    {
        bool firstLayer = true;
        int i=0;
        for (std::vector<TextureLayer>::const_iterator it = layers.begin(); it != layers.end(); ++it)
        {
            osg::ref_ptr<osg::StateSet> stateset (new osg::StateSet);

            if (!firstLayer)
            {
                stateset->setMode(GL_BLEND, osg::StateAttribute::ON);

                stateset->setAttributeAndModes(getEqualDepth(), osg::StateAttribute::ON);
            }

            int texunit = 0;
            if(!firstLayer)
            {
                osg::ref_ptr<osg::Texture2D> blendmap = blendmaps.at(i++);

                stateset->setTextureAttributeAndModes(texunit, blendmap.get());

                // This is to map corner vertices directly to the center of a blendmap texel.
                stateset->setTextureAttributeAndModes(texunit, getBlendmapTexMat(blendmapScale));

                static osg::ref_ptr<osg::TexEnvCombine> texEnvCombine;
                if (!texEnvCombine)
                {
                    texEnvCombine = new osg::TexEnvCombine;
                    texEnvCombine->setCombine_RGB(osg::TexEnvCombine::REPLACE);
                    texEnvCombine->setSource0_RGB(osg::TexEnvCombine::PREVIOUS);
                }

                stateset->setTextureAttributeAndModes(texunit, texEnvCombine, osg::StateAttribute::ON);

                ++texunit;
            }

            // Add the actual layer texture multiplied by the alpha map.
            osg::ref_ptr<osg::Texture2D> tex = it->mDiffuseMap;
            stateset->setTextureAttributeAndModes(texunit, tex.get());

            stateset->setTextureAttributeAndModes(texunit, getLayerTexMat(layerTileSize), osg::StateAttribute::ON);

            firstLayer = false;

            addPass(stateset);
        }
    }

    ShaderTechnique::ShaderTechnique(Shader::ShaderManager& shaderManager, bool forcePerPixelLighting, bool clampLighting, const std::vector<TextureLayer>& layers,
                                                   const std::vector<osg::ref_ptr<osg::Texture2D> >& blendmaps, int blendmapScale, float layerTileSize)
    {
        bool firstLayer = true;
        int i=0;
        for (std::vector<TextureLayer>::const_iterator it = layers.begin(); it != layers.end(); ++it)
        {
            osg::ref_ptr<osg::StateSet> stateset (new osg::StateSet);

            if (!firstLayer)
            {
                stateset->setMode(GL_BLEND, osg::StateAttribute::ON);
                stateset->setAttributeAndModes(getEqualDepth(), osg::StateAttribute::ON);
            }

            int texunit = 0;

            stateset->setTextureAttributeAndModes(texunit, it->mDiffuseMap);

            stateset->setTextureAttributeAndModes(texunit, getLayerTexMat(layerTileSize), osg::StateAttribute::ON);

            stateset->addUniform(new osg::Uniform("diffuseMap", texunit));

            if(!firstLayer)
            {
                ++texunit;
                osg::ref_ptr<osg::Texture2D> blendmap = blendmaps.at(i++);

                stateset->setTextureAttributeAndModes(texunit, blendmap.get());

                stateset->setTextureAttributeAndModes(texunit, getBlendmapTexMat(blendmapScale));
                stateset->addUniform(new osg::Uniform("blendMap", texunit));
            }

            if (it->mNormalMap)
            {
                ++texunit;
                stateset->setTextureAttributeAndModes(texunit, it->mNormalMap);
                stateset->addUniform(new osg::Uniform("normalMap", texunit));
            }

            Shader::ShaderManager::DefineMap defineMap;
            defineMap["forcePPL"] = forcePerPixelLighting ? "1" : "0";
            defineMap["clamp"] = clampLighting ? "1" : "0";
            defineMap["normalMap"] = (it->mNormalMap) ? "1" : "0";
            defineMap["blendMap"] = !firstLayer ? "1" : "0";
            defineMap["colorMode"] = "2";
            defineMap["specularMap"] = it->mSpecular ? "1" : "0";
            defineMap["parallax"] = (it->mNormalMap && it->mParallax) ? "1" : "0";

            osg::ref_ptr<osg::Shader> vertexShader = shaderManager.getShader("terrain_vertex.glsl", defineMap, osg::Shader::VERTEX);
            osg::ref_ptr<osg::Shader> fragmentShader = shaderManager.getShader("terrain_fragment.glsl", defineMap, osg::Shader::FRAGMENT);
            if (!vertexShader || !fragmentShader)
                throw std::runtime_error("Unable to create shader");

            stateset->setAttributeAndModes(shaderManager.getProgram(vertexShader, fragmentShader));

            firstLayer = false;

            addPass(stateset);
        }
    }

    Effect::Effect(bool useShaders, bool forcePerPixelLighting, bool clampLighting, Shader::ShaderManager* shaderManager, const std::vector<TextureLayer> &layers, const std::vector<osg::ref_ptr<osg::Texture2D> > &blendmaps,
                   int blendmapScale, float layerTileSize)
        : mShaderManager(shaderManager)
        , mUseShaders(useShaders)
        , mForcePerPixelLighting(forcePerPixelLighting)
        , mClampLighting(clampLighting)
        , mLayers(layers)
        , mBlendmaps(blendmaps)
        , mBlendmapScale(blendmapScale)
        , mLayerTileSize(layerTileSize)
    {
        selectTechnique(0);
    }

    bool Effect::define_techniques()
    {
        try
        {
            if (mUseShaders && mShaderManager)
                addTechnique(new ShaderTechnique(*mShaderManager, mForcePerPixelLighting, mClampLighting, mLayers, mBlendmaps, mBlendmapScale, mLayerTileSize));
            else
                addTechnique(new FixedFunctionTechnique(mLayers, mBlendmaps, mBlendmapScale, mLayerTileSize));
        }
        catch (std::exception& e)
        {
            std::cerr << "Error: " << e.what() << std::endl;
            addTechnique(new FixedFunctionTechnique(mLayers, mBlendmaps, mBlendmapScale, mLayerTileSize));
        }

        return true;
    }

}
