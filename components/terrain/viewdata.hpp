#ifndef OPENMW_COMPONENTS_TERRAIN_VIEWDATA_H
#define OPENMW_COMPONENTS_TERRAIN_VIEWDATA_H

#include <vector>
#include <deque>

#include <osg/Node>

#include "world.hpp"

namespace Terrain
{

    class QuadTreeNode;

    class ViewData : public View
    {
    public:
        ViewData();
        ~ViewData();

        void add(QuadTreeNode* node);

        void reset() override;

        bool suitableToUse(const osg::Vec4i& activeGrid) const;

        void clear();

        bool contains(QuadTreeNode* node) const;

        void copyFrom(const ViewData& other);

        struct Entry
        {
            Entry();

            bool set(QuadTreeNode* node);

            QuadTreeNode* mNode;

            unsigned int mLodFlags;
            osg::ref_ptr<osg::Node> mRenderingNode;
        };

        unsigned int getNumEntries() const;

        Entry& getEntry(unsigned int i);

        double getLastUsageTimeStamp() const { return mLastUsageTimeStamp; }
        void setLastUsageTimeStamp(double timeStamp) { mLastUsageTimeStamp = timeStamp; }

        /// @return Have any nodes changed since the last frame
        bool hasChanged() const;
        void markUnchanged() { mChanged = false; }

        bool hasViewPoint() const;

        void setViewPoint(const osg::Vec3f& viewPoint);
        const osg::Vec3f& getViewPoint() const;

        void setActiveGrid(const osg::Vec4i &grid) { if (grid != mActiveGrid) {mActiveGrid = grid;mEntries.clear();mNumEntries=0;} }
        const osg::Vec4i &getActiveGrid() const { return mActiveGrid;}

        unsigned int getWorldUpdateRevision() const { return mWorldUpdateRevision; }
        void setWorldUpdateRevision(int updateRevision) { mWorldUpdateRevision = updateRevision; }

    private:
        std::vector<Entry> mEntries;
        unsigned int mNumEntries;
        double mLastUsageTimeStamp;
        bool mChanged;
        osg::Vec3f mViewPoint;
        bool mHasViewPoint;
        osg::Vec4i mActiveGrid;
        unsigned int mWorldUpdateRevision;
    };

    class ViewDataMap : public osg::Referenced
    {
    public:
        ViewDataMap()
            : mReuseDistance(150) // large value should be safe because the visibility of each node is still updated individually for each camera even if the base view was reused.
                                  // this value also serves as a threshold for when a newly loaded LOD gets unloaded again so that if you hover around an LOD transition point the LODs won't keep loading and unloading all the time.
            , mExpiryDelay(1.f)
            , mWorldUpdateRevision(0)
        {}

        ViewData* getViewData(osg::Object* viewer, const osg::Vec3f& viewPoint, const osg::Vec4i &activeGrid, bool& needsUpdate);

        ViewData* createOrReuseView();
        ViewData* createIndependentView() const;

        void clearUnusedViews(double referenceTime);
        void rebuildViews();
        bool storeView(const ViewData* view, double referenceTime);

    private:
        std::list<ViewData> mViewVector;

        typedef std::map<osg::ref_ptr<osg::Object>, ViewData*> ViewerMap;
        ViewerMap mViewers;

        float mReuseDistance;
        float mExpiryDelay; // time in seconds for unused view to be removed

        unsigned int mWorldUpdateRevision;

        std::deque<ViewData*> mUsedViews;
        std::deque<ViewData*> mUnusedViews;
    };

}

#endif
