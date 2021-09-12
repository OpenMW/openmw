#ifndef OPENMW_MWRENDER_OBJECTPAGING_H
#define OPENMW_MWRENDER_OBJECTPAGING_H

#include <components/terrain/quadtreeworld.hpp>
#include <components/resource/resourcemanager.hpp>
#include <components/esm/loadcell.hpp>

#include <mutex>

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

        unsigned int getNodeMask() override;

        /// @return true if view needs rebuild
        bool enableObject(int type, const ESM::RefNum & refnum, const osg::Vec3f& pos, const osg::Vec2i& cell, bool enabled);

        /// @return true if view needs rebuild
        bool blacklistObject(int type, const ESM::RefNum & refnum, const osg::Vec3f& pos, const osg::Vec2i& cell);

        void clear();

        /// Must be called after clear() before rendering starts.
        /// @return true if view needs rebuild
        bool unlockCache();

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

        std::mutex mRefTrackerMutex;
        struct RefTracker
        {
            std::set<ESM::RefNum> mDisabled;
            std::set<ESM::RefNum> mBlacklist;
            bool operator==(const RefTracker&other) const { return mDisabled == other.mDisabled && mBlacklist == other.mBlacklist; }
        };
        RefTracker mRefTracker;
        RefTracker mRefTrackerNew;
        bool mRefTrackerLocked;

        const RefTracker& getRefTracker() const { return mRefTracker; }
        RefTracker& getWritableRefTracker() { return mRefTrackerLocked ? mRefTrackerNew : mRefTracker; }

        std::mutex mSizeCacheMutex;
        typedef std::map<ESM::RefNum, float> SizeCache;
        SizeCache mSizeCache;
    };

    class RefnumMarker : public osg::Object
    {
    public:
        RefnumMarker() : mNumVertices(0) { mRefnum.unset(); }
        RefnumMarker(const RefnumMarker &copy, osg::CopyOp co) : mRefnum(copy.mRefnum), mNumVertices(copy.mNumVertices) {}
        META_Object(MWRender, RefnumMarker)

        ESM::RefNum mRefnum;
        unsigned int mNumVertices;
    };
}

#endif
