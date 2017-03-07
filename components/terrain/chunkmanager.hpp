#ifndef OPENMW_COMPONENTS_TERRAIN_CHUNKMANAGER_H
#define OPENMW_COMPONENTS_TERRAIN_CHUNKMANAGER_H

#include <components/resource/resourcemanager.hpp>

#include "buffercache.hpp"

namespace osg
{
    class Group;
    class Texture2D;
}

namespace SceneUtil
{
    class PositionAttitudeTransform;
}

namespace Resource
{
    class SceneManager;
}

namespace Terrain
{

    class TextureManager;
    class CompositeMapRenderer;
    class Storage;

    /// @brief Handles loading and caching of terrain chunks
    class ChunkManager : public Resource::ResourceManager
    {
    public:
        ChunkManager(Storage* storage, Resource::SceneManager* sceneMgr, TextureManager* textureManager, CompositeMapRenderer* renderer);

        osg::ref_ptr<osg::Node> getChunk(float size, const osg::Vec2f& center);

        virtual void reportStats(unsigned int frameNumber, osg::Stats* stats) const;

    private:
        osg::ref_ptr<osg::Node> createChunk(float size, const osg::Vec2f& center);

        osg::ref_ptr<osg::Group> createCompositeMapRTT(osg::ref_ptr<osg::Texture2D>& texture);

        Terrain::Storage* mStorage;
        Resource::SceneManager* mSceneManager;
        TextureManager* mTextureManager;
        CompositeMapRenderer* mCompositeMapRenderer;
        BufferCache mBufferCache;

        unsigned int mCompositeMapSize;
    };

}

#endif
