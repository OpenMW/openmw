#ifndef COMPONENTS_TERRAIN_MATERIAL_H
#define COMPONENTS_TERRAIN_MATERIAL_H

#include <osg/StateSet>

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

    std::vector<osg::ref_ptr<osg::StateSet> > createPasses(bool useShaders, Shader::ShaderManager* shaderManager,
                                                           const std::vector<TextureLayer>& layers,
                                                           const std::vector<osg::ref_ptr<osg::Texture2D> >& blendmaps, int blendmapScale, float layerTileSize);

}

#endif
