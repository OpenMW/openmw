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

        void add(QuadTreeNode* node, bool visible);

        void reset(unsigned int frame);

        void clear();

        bool contains(QuadTreeNode* node);

        struct Entry
        {
            Entry();

            bool set(QuadTreeNode* node, bool visible);

            QuadTreeNode* mNode;
            bool mVisible;

            unsigned int mLodFlags;
            osg::ref_ptr<osg::Node> mRenderingNode;
        };

        unsigned int getNumEntries() const;

        Entry& getEntry(unsigned int i);

        unsigned int getFrameLastUsed() const { return mFrameLastUsed; }

        /// @return Have any nodes changed since the last frame
        bool hasChanged() const;

        bool hasEyePoint() const;

        void setEyePoint(const osg::Vec3f& eye);
        const osg::Vec3f& getEyePoint() const;

    private:
        std::vector<Entry> mEntries;
        unsigned int mNumEntries;
        unsigned int mFrameLastUsed;
        bool mChanged;
        osg::Vec3f mEyePoint;
        bool mHasEyePoint;
    };

    class ViewDataMap : public osg::Referenced
    {
    public:
        ViewData* getViewData(osg::Object* viewer);

        ViewData* createOrReuseView();

        void clearUnusedViews(unsigned int frame);

        void clear();

        void setDefaultViewer(osg::Object* viewer);

        ViewData* getDefaultView();

    private:
        std::list<ViewData> mViewVector;

        typedef std::map<osg::ref_ptr<osg::Object>, ViewData*> Map;
        Map mViews;

        std::deque<ViewData*> mUnusedViews;

        osg::ref_ptr<osg::Object> mDefaultViewer;
    };

}

#endif
