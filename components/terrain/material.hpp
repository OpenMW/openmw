#ifndef COMPONENTS_TERRAIN_MATERIAL_H
#define COMPONENTS_TERRAIN_MATERIAL_H

#include <osgFX/Technique>
#include <osgFX/Effect>

#include "defs.hpp"

namespace osg
{
    class Texture2D;
}

namespace Shader
{
    class ShaderManager;
}

namespace Terrain
{

    struct TextureLayer
    {
        osg::ref_ptr<osg::Texture2D> mDiffuseMap;
        osg::ref_ptr<osg::Texture2D> mNormalMap; // optional
        bool mParallax;
        bool mSpecular;
    };

    class FixedFunctionTechnique : public osgFX::Technique
    {
    public:
        FixedFunctionTechnique(
                const std::vector<TextureLayer>& layers,
                const std::vector<osg::ref_ptr<osg::Texture2D> >& blendmaps, int blendmapScale, float layerTileSize);

    protected:
        virtual void define_passes() {}
    };

    class ShaderTechnique : public osgFX::Technique
    {
    public:
        ShaderTechnique(Shader::ShaderManager& shaderManager, bool forcePerPixelLighting, bool clampLighting,
                const std::vector<TextureLayer>& layers,
                const std::vector<osg::ref_ptr<osg::Texture2D> >& blendmaps, int blendmapScale, float layerTileSize);

    protected:
        virtual void define_passes() {}
    };

    class Effect : public osgFX::Effect
    {
    public:
        Effect(bool useShaders, bool forcePerPixelLighting, bool clampLighting, Shader::ShaderManager* shaderManager,
                const std::vector<TextureLayer>& layers,
                const std::vector<osg::ref_ptr<osg::Texture2D> >& blendmaps, int blendmapScale, float layerTileSize);

        virtual bool define_techniques();

        virtual const char *effectName() const
        {
            return NULL;
        }
        virtual const char *effectDescription() const
        {
            return NULL;
        }
        virtual const char *effectAuthor() const
        {
            return NULL;
        }

    private:
        Shader::ShaderManager* mShaderManager;
        bool mUseShaders;
        bool mForcePerPixelLighting;
        bool mClampLighting;
        std::vector<TextureLayer> mLayers;
        std::vector<osg::ref_ptr<osg::Texture2D> > mBlendmaps;
        int mBlendmapScale;
        float mLayerTileSize;
    };

}

#endif
