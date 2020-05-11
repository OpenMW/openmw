#ifndef OPENMW_COMPONENTS_ESMPAGING_CHUNKMANAGER_H
#define OPENMW_COMPONENTS_ESMPAGING_CHUNKMANAGER_H

#include <components/terrain/quadtreeworld.hpp>
#include <components/resource/resourcemanager.hpp>
#include <components/esm/loadcell.hpp>

#include <OpenThreads/Mutex>

namespace Resource
{
    class SceneManager;
}
namespace MWWorld
{
    class ESMStore;
}

namespace MWRender
{

    typedef std::tuple<osg::Vec2f, float, bool> ChunkId; // Center, Size, ActiveGrid

    class ObjectPaging : public Resource::GenericResourceManager<ChunkId>, public Terrain::QuadTreeWorld::ChunkManager
    {
    public:
        ObjectPaging(Resource::SceneManager* sceneManager);
        ~ObjectPaging() = default;

        osg::ref_ptr<osg::Node> getChunk(float size, const osg::Vec2f& center, unsigned char lod, unsigned int lodFlags, bool activeGrid, const osg::Vec3f& viewPoint, bool compile) override;

        osg::ref_ptr<osg::Node> createChunk(float size, const osg::Vec2f& center, bool activeGrid, const osg::Vec3f& viewPoint, bool compile);

        virtual unsigned int getNodeMask() override;

        /// @return true if view needs rebuild
        bool enableObject(int type, const ESM::RefNum & refnum, const osg::Vec3f& pos, bool enabled);

        /// @return true if view needs rebuild
        bool blacklistObject(int type, const ESM::RefNum & refnum, const osg::Vec3f& pos);

        void clear();

        void reportStats(unsigned int frameNumber, osg::Stats* stats) const override;

        void getPagedRefnums(const osg::Vec4i &activeGrid, std::set<ESM::RefNum> &out);

    private:
        Resource::SceneManager* mSceneManager;
        bool mActiveGrid;
        bool mDebugBatches;
        float mMergeFactor;
        float mMinSize;
        float mMinSizeMergeFactor;
        float mMinSizeCostMultiplier;

        OpenThreads::Mutex mDisabledMutex;
        std::set<ESM::RefNum> mDisabled;
        std::set<ESM::RefNum> mBlacklist;

        OpenThreads::Mutex mSizeCacheMutex;
        typedef std::map<ESM::RefNum, float> SizeCache;
        SizeCache mSizeCache;
    };

}

#endif
