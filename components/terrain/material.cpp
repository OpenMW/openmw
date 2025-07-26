#include "material.hpp"

#include <osg/BlendFunc>
#include <osg/Capability>
#include <osg/Depth>
#include <osg/Fog>
#include <osg/TexEnvCombine>
#include <osg/TexMat>
#include <osg/Texture2D>

#include <components/resource/scenemanager.hpp>
#include <components/sceneutil/depth.hpp>
#include <components/sceneutil/util.hpp>
#include <components/shader/shadermanager.hpp>
#include <components/stereo/stereomanager.hpp>

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
                float scale = (blendmapScale / (static_cast<float>(blendmapScale) + 1.f));
                matrix.preMultTranslate(osg::Vec3f(0.5f, 0.5f, 0.f));
                matrix.preMultScale(osg::Vec3f(scale, scale, 1.f));
                matrix.preMultTranslate(osg::Vec3f(-0.5f, -0.5f, 0.f));
                // We need to nudge the blendmap to look like vanilla.
                // This causes visible seams unless the blendmap's resolution is doubled, but Vanilla also doubles the
                // blendmap, apparently.
                matrix.preMultTranslate(osg::Vec3f(1.0f / blendmapScale / 4.0f, 1.0f / blendmapScale / 4.0f, 0.f));

                texMat = mTexMatMap.insert(std::make_pair(static_cast<float>(blendmapScale), new osg::TexMat(matrix)))
                             .first;
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
                texMat = mTexMatMap
                             .insert(std::make_pair(layerTileSize,
                                 new osg::TexMat(osg::Matrix::scale(osg::Vec3f(layerTileSize, layerTileSize, 1.f)))))
                             .first;
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
            : mValue(new SceneUtil::AutoDepth)
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
            : mValue(new SceneUtil::AutoDepth(osg::Depth::LEQUAL))
        {
        }
    };

    class BlendFuncFirst
    {
    public:
        static const osg::ref_ptr<osg::BlendFunc>& value()
        {
            static BlendFuncFirst instance;
            return instance.mValue;
        }

    private:
        osg::ref_ptr<osg::BlendFunc> mValue;

        BlendFuncFirst()
            : mValue(new osg::BlendFunc(osg::BlendFunc::SRC_ALPHA, osg::BlendFunc::ZERO))
        {
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
            : mValue(new osg::BlendFunc(osg::BlendFunc::SRC_ALPHA, osg::BlendFunc::ONE))
        {
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

    class DiscardAlphaCombine
    {
    public:
        static const osg::ref_ptr<osg::TexEnvCombine>& value()
        {
            static DiscardAlphaCombine instance;
            return instance.mValue;
        }

    private:
        osg::ref_ptr<osg::TexEnvCombine> mValue;

        DiscardAlphaCombine()
            : mValue(new osg::TexEnvCombine)
        {
            mValue->setCombine_Alpha(osg::TexEnvCombine::REPLACE);
            mValue->setSource0_Alpha(osg::TexEnvCombine::CONSTANT);
            mValue->setConstantColor(osg::Vec4(0.0, 0.0, 0.0, 1.0));
        }
    };

    class UniformCollection
    {
    public:
        static const UniformCollection& value()
        {
            static UniformCollection instance;
            return instance;
        }

        osg::ref_ptr<osg::Uniform> mDiffuseMap;
        osg::ref_ptr<osg::Uniform> mBlendMap;
        osg::ref_ptr<osg::Uniform> mNormalMap;
        osg::ref_ptr<osg::Uniform> mColorMode;

        UniformCollection()
            : mDiffuseMap(new osg::Uniform("diffuseMap", 0))
            , mBlendMap(new osg::Uniform("blendMap", 1))
            , mNormalMap(new osg::Uniform("normalMap", 2))
            , mColorMode(new osg::Uniform("colorMode", 2))
        {
        }
    };
}

namespace Terrain
{
    std::vector<osg::ref_ptr<osg::StateSet>> createPasses(bool useShaders, Resource::SceneManager* sceneManager,
        const std::vector<TextureLayer>& layers, const std::vector<osg::ref_ptr<osg::Texture2D>>& blendmaps,
        int blendmapScale, float layerTileSize)
    {
        auto& shaderManager = sceneManager->getShaderManager();
        std::vector<osg::ref_ptr<osg::StateSet>> passes;

        unsigned int blendmapIndex = 0;
        for (std::vector<TextureLayer>::const_iterator it = layers.begin(); it != layers.end(); ++it)
        {
            bool firstLayer = (it == layers.begin());

            osg::ref_ptr<osg::StateSet> stateset(new osg::StateSet);

            if (!blendmaps.empty())
            {
                stateset->setMode(GL_BLEND, osg::StateAttribute::ON);
                if (sceneManager->getSupportsNormalsRT())
                    stateset->setAttribute(new osg::Disablei(GL_BLEND, 1));
                stateset->setRenderBinDetails(firstLayer ? 0 : 1, "RenderBin");
                if (!firstLayer)
                {
                    stateset->setAttributeAndModes(BlendFunc::value(), osg::StateAttribute::ON);
                    stateset->setAttributeAndModes(EqualDepth::value(), osg::StateAttribute::ON);
                }
                else
                {
                    stateset->setAttributeAndModes(BlendFuncFirst::value(), osg::StateAttribute::ON);
                    stateset->setAttributeAndModes(LequalDepth::value(), osg::StateAttribute::ON);
                }
            }

            if (useShaders)
            {
                stateset->setTextureAttributeAndModes(0, it->mDiffuseMap);

                if (layerTileSize != 1.f)
                    stateset->setTextureAttributeAndModes(
                        0, LayerTexMat::value(layerTileSize), osg::StateAttribute::ON);

                stateset->addUniform(UniformCollection::value().mDiffuseMap);

                if (!blendmaps.empty())
                {
                    osg::ref_ptr<osg::Texture2D> blendmap = blendmaps.at(blendmapIndex++);

                    stateset->setTextureAttributeAndModes(1, blendmap.get());
                    stateset->setTextureAttributeAndModes(1, BlendmapTexMat::value(blendmapScale));
                    stateset->addUniform(UniformCollection::value().mBlendMap);
                }

                bool parallax = it->mNormalMap && it->mParallax;
                bool reconstructNormalZ = false;

                if (it->mNormalMap)
                {
                    stateset->setTextureAttributeAndModes(2, it->mNormalMap);
                    stateset->addUniform(UniformCollection::value().mNormalMap);

                    // Special handling for red-green normal maps (e.g. BC5 or R8G8).
                    const osg::Image* image = it->mNormalMap->getImage(0);
                    if (image)
                    {
                        switch (SceneUtil::computeUnsizedPixelFormat(image->getPixelFormat()))
                        {
                            case GL_RG:
                            case GL_RG_INTEGER:
                            {
                                reconstructNormalZ = true;
                                parallax = false;
                            }
                        }
                    }
                }

                Shader::ShaderManager::DefineMap defineMap;
                defineMap["normalMap"] = (it->mNormalMap) ? "1" : "0";
                defineMap["blendMap"] = (!blendmaps.empty()) ? "1" : "0";
                defineMap["specularMap"] = it->mSpecular ? "1" : "0";
                defineMap["parallax"] = parallax ? "1" : "0";
                defineMap["writeNormals"] = (it == layers.end() - 1) ? "1" : "0";
                defineMap["reconstructNormalZ"] = reconstructNormalZ ? "1" : "0";
                Stereo::shaderStereoDefines(defineMap);

                stateset->setAttributeAndModes(shaderManager.getProgram("terrain", defineMap));
                stateset->addUniform(UniformCollection::value().mColorMode);
            }
            else
            {
                // Add the actual layer texture
                osg::ref_ptr<osg::Texture2D> tex = it->mDiffuseMap;
                stateset->setTextureAttributeAndModes(0, tex.get());

                if (layerTileSize != 1.f)
                    stateset->setTextureAttributeAndModes(
                        0, LayerTexMat::value(layerTileSize), osg::StateAttribute::ON);

                stateset->setTextureAttributeAndModes(0, DiscardAlphaCombine::value(), osg::StateAttribute::ON);

                // Multiply by the alpha map
                if (!blendmaps.empty())
                {
                    osg::ref_ptr<osg::Texture2D> blendmap = blendmaps.at(blendmapIndex++);

                    stateset->setTextureAttributeAndModes(1, blendmap.get());

                    // This is to map corner vertices directly to the center of a blendmap texel.
                    stateset->setTextureAttributeAndModes(1, BlendmapTexMat::value(blendmapScale));
                    stateset->setTextureAttributeAndModes(1, TexEnvCombine::value(), osg::StateAttribute::ON);
                }
            }

            passes.push_back(stateset);
        }
        return passes;
    }

}
