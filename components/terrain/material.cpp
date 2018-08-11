#include "material.hpp"

#include <stdexcept>

#include <osg/Fog>
#include <osg/Depth>
#include <osg/TexEnvCombine>
#include <osg/Texture2D>
#include <osg/TexMat>
#include <osg/Material>
#include <osg/BlendFunc>

#include <components/shader/shadermanager.hpp>

#include <mutex>

namespace
{
    class BlendmapTexMat
    {
    public:
        static const osg::ref_ptr<osg::TexMat>& value(const int blendmapScale)
        {
            static BlendmapTexMat instance;
            return instance.get(blendmapScale);
        }

        const osg::ref_ptr<osg::TexMat>& get(const int blendmapScale)
        {
            const std::lock_guard<std::mutex> lock(mMutex);
            auto texMat = mTexMatMap.find(blendmapScale);
            if (texMat == mTexMatMap.end())
            {
                osg::Matrixf matrix;
                float scale = (blendmapScale/(static_cast<float>(blendmapScale)+1.f));
                matrix.preMultTranslate(osg::Vec3f(0.5f, 0.5f, 0.f));
                matrix.preMultScale(osg::Vec3f(scale, scale, 1.f));
                matrix.preMultTranslate(osg::Vec3f(-0.5f, -0.5f, 0.f));
                // We need to nudge the blendmap to look like vanilla.
                // This causes visible seams unless the blendmap's resolution is doubled, but Vanilla also doubles the blendmap, apparently.
                matrix.preMultTranslate(osg::Vec3f(1.0f/blendmapScale/4.0f, 1.0f/blendmapScale/4.0f, 0.f));

                texMat = mTexMatMap.insert(std::make_pair(blendmapScale, new osg::TexMat(matrix))).first;
            }
            return texMat->second;
        }

    private:
        std::mutex mMutex;
        std::map<float, osg::ref_ptr<osg::TexMat>> mTexMatMap;
    };

    class LayerTexMat
    {
    public:
        static const osg::ref_ptr<osg::TexMat>& value(const float layerTileSize)
        {
            static LayerTexMat instance;
            return instance.get(layerTileSize);
        }

        const osg::ref_ptr<osg::TexMat>& get(const float layerTileSize)
        {
            const std::lock_guard<std::mutex> lock(mMutex);
            auto texMat = mTexMatMap.find(layerTileSize);
            if (texMat == mTexMatMap.end())
            {
                texMat = mTexMatMap.insert(std::make_pair(layerTileSize,
                    new osg::TexMat(osg::Matrix::scale(osg::Vec3f(layerTileSize, layerTileSize, 1.f))))).first;
            }
            return texMat->second;
        }

    private:
        std::mutex mMutex;
        std::map<float, osg::ref_ptr<osg::TexMat>> mTexMatMap;
    };

    class EqualDepth
    {
    public:
        static const osg::ref_ptr<osg::Depth>& value()
        {
            static EqualDepth instance;
            return instance.mValue;
        }

    private:
        osg::ref_ptr<osg::Depth> mValue;

        EqualDepth()
            : mValue(new osg::Depth)
        {
            mValue->setFunction(osg::Depth::EQUAL);
        }
    };

    class LequalDepth
    {
    public:
        static const osg::ref_ptr<osg::Depth>& value()
        {
            static LequalDepth instance;
            return instance.mValue;
        }

    private:
        osg::ref_ptr<osg::Depth> mValue;

        LequalDepth()
            : mValue(new osg::Depth)
        {
            mValue->setFunction(osg::Depth::LEQUAL);
        }
    };

    class BlendFunc
    {
    public:
        static const osg::ref_ptr<osg::BlendFunc>& value()
        {
            static BlendFunc instance;
            return instance.mValue;
        }

    private:
        osg::ref_ptr<osg::BlendFunc> mValue;

        BlendFunc()
            : mValue(new osg::BlendFunc)
        {
            mValue->setFunction(osg::BlendFunc::SRC_ALPHA, osg::BlendFunc::ONE);
        }
    };

    class TexEnvCombine
    {
    public:
        static const osg::ref_ptr<osg::TexEnvCombine>& value()
        {
            static TexEnvCombine instance;
            return instance.mValue;
        }

    private:
        osg::ref_ptr<osg::TexEnvCombine> mValue;

        TexEnvCombine()
            : mValue(new osg::TexEnvCombine)
        {
            mValue->setCombine_RGB(osg::TexEnvCombine::REPLACE);
            mValue->setSource0_RGB(osg::TexEnvCombine::PREVIOUS);
        }
    };
}

namespace Terrain
{
    std::vector<osg::ref_ptr<osg::StateSet> > createPasses(bool useShaders, bool forcePerPixelLighting, bool clampLighting, Shader::ShaderManager* shaderManager, const std::vector<TextureLayer> &layers,
                                                           const std::vector<osg::ref_ptr<osg::Texture2D> > &blendmaps, int blendmapScale, float layerTileSize)
    {
        std::vector<osg::ref_ptr<osg::StateSet> > passes;

        unsigned int blendmapIndex = 0;
        unsigned int passIndex = 0;
        for (std::vector<TextureLayer>::const_iterator it = layers.begin(); it != layers.end(); ++it)
        {
            bool firstLayer = (it == layers.begin());

            osg::ref_ptr<osg::StateSet> stateset (new osg::StateSet);

            if (!firstLayer)
            {
                stateset->setMode(GL_BLEND, osg::StateAttribute::ON);
                stateset->setAttributeAndModes(BlendFunc::value(), osg::StateAttribute::ON);
                stateset->setAttributeAndModes(EqualDepth::value(), osg::StateAttribute::ON);
            }
            // disable fog if we're the first layer of several - supposed to be completely black
            if (firstLayer && blendmaps.size() > 0)
            {
                osg::ref_ptr<osg::Fog> fog (new osg::Fog);
                fog->setStart(10000000);
                fog->setEnd(10000000);
                stateset->setAttributeAndModes(fog, osg::StateAttribute::OFF|osg::StateAttribute::OVERRIDE);
                stateset->setAttributeAndModes(LequalDepth::value(), osg::StateAttribute::ON);
            }

            int texunit = 0;

            if (useShaders)
            {
                stateset->setTextureAttributeAndModes(texunit, it->mDiffuseMap);

                if (layerTileSize != 1.f)
                    stateset->setTextureAttributeAndModes(texunit, LayerTexMat::value(layerTileSize), osg::StateAttribute::ON);

                stateset->addUniform(new osg::Uniform("diffuseMap", texunit));

                if(!firstLayer)
                {
                    ++texunit;
                    osg::ref_ptr<osg::Texture2D> blendmap = blendmaps.at(blendmapIndex++);

                    stateset->setTextureAttributeAndModes(texunit, blendmap.get());
                    stateset->setTextureAttributeAndModes(texunit, BlendmapTexMat::value(blendmapScale));
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

                osg::ref_ptr<osg::Shader> vertexShader = shaderManager->getShader("terrain_vertex.glsl", defineMap, osg::Shader::VERTEX);
                osg::ref_ptr<osg::Shader> fragmentShader = shaderManager->getShader("terrain_fragment.glsl", defineMap, osg::Shader::FRAGMENT);
                if (!vertexShader || !fragmentShader)
                {
                    // Try again without shader. Error already logged by above
                    return createPasses(false, forcePerPixelLighting, clampLighting, shaderManager, layers, blendmaps, blendmapScale, layerTileSize);
                }

                stateset->setAttributeAndModes(shaderManager->getProgram(vertexShader, fragmentShader));
            }
            else
            {
                if(!firstLayer)
                {
                    osg::ref_ptr<osg::Texture2D> blendmap = blendmaps.at(blendmapIndex++);

                    stateset->setTextureAttributeAndModes(texunit, blendmap.get());

                    // This is to map corner vertices directly to the center of a blendmap texel.
                    stateset->setTextureAttributeAndModes(texunit, BlendmapTexMat::value(blendmapScale));
                    stateset->setTextureAttributeAndModes(texunit, TexEnvCombine::value(), osg::StateAttribute::ON);

                    ++texunit;
                }

                // Add the actual layer texture multiplied by the alpha map.
                osg::ref_ptr<osg::Texture2D> tex = it->mDiffuseMap;
                stateset->setTextureAttributeAndModes(texunit, tex.get());

                if (layerTileSize != 1.f)
                    stateset->setTextureAttributeAndModes(texunit, LayerTexMat::value(layerTileSize), osg::StateAttribute::ON);
            }

            stateset->setRenderBinDetails(passIndex++, "RenderBin");

            passes.push_back(stateset);
        }
        return passes;
    }

}
