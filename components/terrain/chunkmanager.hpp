#ifndef OPENMW_COMPONENTS_TERRAIN_CHUNKMANAGER_H
#define OPENMW_COMPONENTS_TERRAIN_CHUNKMANAGER_H

#include <components/resource/resourcemanager.hpp>

#include "buffercache.hpp"

namespace osg
{
    class Group;
    class Texture2D;
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
    class CompositeMap;

    /// @brief Handles loading and caching of terrain chunks
    class ChunkManager : public Resource::ResourceManager
    {
    public:
        ChunkManager(Storage* storage, Resource::SceneManager* sceneMgr, TextureManager* textureManager, CompositeMapRenderer* renderer);

        osg::ref_ptr<osg::Node> getChunk(float size, const osg::Vec2f& center, int lod, unsigned int lodFlags);

        void reportStats(unsigned int frameNumber, osg::Stats* stats) const override;

        void clearCache() override;

        void releaseGLObjects(osg::State* state) override;

        void setCullingActive(bool active);

    private:
        osg::ref_ptr<osg::Node> createChunk(float size, const osg::Vec2f& center, int lod, unsigned int lodFlags);

        osg::ref_ptr<osg::Texture2D> createCompositeMapRTT();

        void createCompositeMapGeometry(float chunkSize, const osg::Vec2f& chunkCenter, const osg::Vec4f& texCoords, CompositeMap& map);

        std::vector<osg::ref_ptr<osg::StateSet> > createPasses(float chunkSize, const osg::Vec2f& chunkCenter, bool forCompositeMap);

        Terrain::Storage* mStorage;
        Resource::SceneManager* mSceneManager;
        TextureManager* mTextureManager;
        CompositeMapRenderer* mCompositeMapRenderer;
        BufferCache mBufferCache;

        unsigned int mCompositeMapSize;

        bool mCullingActive;
    };

}

#endif
