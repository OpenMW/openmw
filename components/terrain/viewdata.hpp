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

        void reset();

        void clear();

        bool contains(QuadTreeNode* node);

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

    private:
        std::vector<Entry> mEntries;
        unsigned int mNumEntries;
        double mLastUsageTimeStamp;
        bool mChanged;
        osg::Vec3f mViewPoint;
        bool mHasViewPoint;
    };

    class ViewDataMap : public osg::Referenced
    {
    public:
        ViewDataMap()
            : mReuseDistance(300) // large value should be safe because the visibility of each node is still updated individually for each camera even if the base view was reused.
                                  // this value also serves as a threshold for when a newly loaded LOD gets unloaded again so that if you hover around an LOD transition point the LODs won't keep loading and unloading all the time.
            , mExpiryDelay(1.f)
        {}

        ViewData* getViewData(osg::Object* viewer, const osg::Vec3f& viewPoint, bool& needsUpdate);

        ViewData* createOrReuseView();

        void clearUnusedViews(double referenceTime);

        void clear();

    private:
        std::list<ViewData> mViewVector;

        typedef std::map<osg::ref_ptr<osg::Object>, ViewData*> Map;
        Map mViews;

        float mReuseDistance;
        float mExpiryDelay; // time in seconds for unused view to be removed

        std::deque<ViewData*> mUnusedViews;
    };

}

#endif
