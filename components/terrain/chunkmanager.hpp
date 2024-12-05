#ifndef OPENMW_COMPONENTS_TERRAIN_CHUNKMANAGER_H
#define OPENMW_COMPONENTS_TERRAIN_CHUNKMANAGER_H

#include <tuple>

#include <components/resource/resourcemanager.hpp>

#include "buffercache.hpp"
#include "quadtreeworld.hpp"

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
    class TerrainDrawable;

    struct TemplateKey
    {
        osg::Vec2f mCenter;
        unsigned char mLod;
    };

    inline auto tie(const TemplateKey& v)
    {
        return std::tie(v.mCenter, v.mLod);
    }

    inline bool operator<(const TemplateKey& l, const TemplateKey& r)
    {
        return tie(l) < tie(r);
    }

    inline bool operator==(const TemplateKey& l, const TemplateKey& r)
    {
        return tie(l) == tie(r);
    }

    struct ChunkKey
    {
        osg::Vec2f mCenter;
        unsigned char mLod;
        unsigned mLodFlags;
    };

    inline auto tie(const ChunkKey& v)
    {
        return std::tie(v.mCenter, v.mLod, v.mLodFlags);
    }

    inline bool operator<(const ChunkKey& l, const ChunkKey& r)
    {
        return tie(l) < tie(r);
    }

    inline bool operator<(const ChunkKey& l, const TemplateKey& r)
    {
        return TemplateKey{ .mCenter = l.mCenter, .mLod = l.mLod } < r;
    }

    /// @brief Handles loading and caching of terrain chunks
    class ChunkManager : public Resource::GenericResourceManager<ChunkKey>, public QuadTreeWorld::ChunkManager
    {
    public:
        explicit ChunkManager(Storage* storage, Resource::SceneManager* sceneMgr, TextureManager* textureManager,
            CompositeMapRenderer* renderer, ESM::RefId worldspace, double expiryDelay);

        osg::ref_ptr<osg::Node> getChunk(float size, const osg::Vec2f& center, unsigned char lod, unsigned int lodFlags,
            bool activeGrid, const osg::Vec3f& viewPoint, bool compile) override;

        void setCompositeMapSize(unsigned int size) { mCompositeMapSize = size; }
        void setCompositeMapLevel(float level) { mCompositeMapLevel = level; }
        void setMaxCompositeGeometrySize(float maxCompGeometrySize) { mMaxCompGeometrySize = maxCompGeometrySize; }

        void setNodeMask(unsigned int mask) { mNodeMask = mask; }
        unsigned int getNodeMask() override { return mNodeMask; }

        void reportStats(unsigned int frameNumber, osg::Stats* stats) const override;

        void clearCache() override;

        void releaseGLObjects(osg::State* state) override;

    private:
        osg::ref_ptr<osg::Node> createChunk(float size, const osg::Vec2f& center, unsigned char lod,
            unsigned int lodFlags, bool compile, const TerrainDrawable* templateGeometry);

        osg::ref_ptr<osg::Texture2D> createCompositeMapRTT();

        void createCompositeMapGeometry(
            float chunkSize, const osg::Vec2f& chunkCenter, const osg::Vec4f& texCoords, CompositeMap& map);

        std::vector<osg::ref_ptr<osg::StateSet>> createPasses(
            float chunkSize, const osg::Vec2f& chunkCenter, bool forCompositeMap);

        Terrain::Storage* mStorage;
        Resource::SceneManager* mSceneManager;
        TextureManager* mTextureManager;
        CompositeMapRenderer* mCompositeMapRenderer;
        BufferCache mBufferCache;

        osg::ref_ptr<osg::StateSet> mMultiPassRoot;

        unsigned int mNodeMask;

        unsigned int mCompositeMapSize;
        float mCompositeMapLevel;
        float mMaxCompGeometrySize;
    };

}

#endif
