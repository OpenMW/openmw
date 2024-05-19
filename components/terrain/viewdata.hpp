#ifndef OPENMW_COMPONENTS_TERRAIN_VIEWDATA_H
#define OPENMW_COMPONENTS_TERRAIN_VIEWDATA_H

#include <deque>
#include <vector>

#include <osg/Node>

#include "view.hpp"

namespace Terrain
{

    class QuadTreeNode;

    struct ViewDataEntry
    {
        ViewDataEntry();

        bool set(QuadTreeNode* node);

        QuadTreeNode* mNode;

        unsigned int mLodFlags;
        osg::ref_ptr<osg::Node> mRenderingNode;
    };

    class ViewData : public View
    {
    public:
        ViewData();
        ~ViewData();

        void add(QuadTreeNode* node);

        void reset() override;

        bool suitableToUse(const osg::Vec4i& activeGrid) const;

        void clear();

        bool contains(const QuadTreeNode* node) const;

        void copyFrom(const ViewData& other);

        unsigned int getNumEntries() const { return mNumEntries; }
        ViewDataEntry& getEntry(unsigned int i) { return mEntries[i]; }

        double getLastUsageTimeStamp() const { return mLastUsageTimeStamp; }
        void setLastUsageTimeStamp(double timeStamp) { mLastUsageTimeStamp = timeStamp; }

        /// Indicates at least one mNode of mEntries has changed.
        /// @note Such changes may necessitate a revalidation of cached mRenderingNodes elsewhere depending
        /// on the parameters that affect the creation of mRenderingNode.
        bool hasChanged() const { return mChanged; }
        void resetChanged() { mChanged = false; }

        bool hasViewPoint() const { return mHasViewPoint; }

        void setViewPoint(const osg::Vec3f& viewPoint);
        const osg::Vec3f& getViewPoint() const { return mViewPoint; }

        void setActiveGrid(const osg::Vec4i& grid)
        {
            if (grid != mActiveGrid)
            {
                mActiveGrid = grid;
                mEntries.clear();
                mNumEntries = 0;
                mNodes.clear();
            }
        }

        unsigned int getWorldUpdateRevision() const { return mWorldUpdateRevision; }
        void setWorldUpdateRevision(int updateRevision) { mWorldUpdateRevision = updateRevision; }

        void buildNodeIndex();

        void removeNodeFromIndex(const QuadTreeNode* node);

    private:
        std::vector<ViewDataEntry> mEntries;
        std::vector<const QuadTreeNode*> mNodes;
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
            : mReuseDistance(
                150) // large value should be safe because the visibility of each node is still updated individually for
                     // each camera even if the base view was reused. this value also serves as a threshold for when a
                     // newly loaded LOD gets unloaded again so that if you hover around an LOD transition point the
                     // LODs won't keep loading and unloading all the time.
            , mExpiryDelay(1.f)
            , mWorldUpdateRevision(0)
        {
        }

        ViewData* getViewData(
            osg::Object* viewer, const osg::Vec3f& viewPoint, const osg::Vec4i& activeGrid, bool& needsUpdate);

        ViewData* createOrReuseView();
        ViewData* createIndependentView() const;

        void clearUnusedViews(double referenceTime);
        void rebuildViews();

        float getReuseDistance() const { return mReuseDistance; }

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
