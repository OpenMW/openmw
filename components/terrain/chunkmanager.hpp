#ifndef OPENMW_COMPONENTS_TERRAIN_CHUNKMANAGER_H
#define OPENMW_COMPONENTS_TERRAIN_CHUNKMANAGER_H

#include <components/resource/resourcemanager.hpp>

#include "buffercache.hpp"

namespace osg
{
    class Geometry;
}

namespace SceneUtil
{
    class PositionAttitudeTransform;
}

namespace Resource
{
    class SceneManager;
}

namespace Shader
{
    class ShaderManager;
}

namespace Terrain
{

    class TextureManager;
    class Storage;

    /// @brief Handles loading and caching of terrain chunks
    class ChunkManager : public Resource::ResourceManager
    {
    public:
        ChunkManager(Storage* storage, Resource::SceneManager* sceneMgr, TextureManager* textureManager);

        osg::ref_ptr<osg::Node> getChunk(float size, const osg::Vec2f& center);

        // Optional
        void setShaderManager(Shader::ShaderManager* shaderManager);

        virtual void reportStats(unsigned int frameNumber, osg::Stats* stats) const;

    private:
        osg::ref_ptr<osg::Node> createChunk(float size, const osg::Vec2f& center);

        Terrain::Storage* mStorage;
        Resource::SceneManager* mSceneManager;
        TextureManager* mTextureManager;
        Shader::ShaderManager* mShaderManager;
        BufferCache mBufferCache;
    };

}

#endif
