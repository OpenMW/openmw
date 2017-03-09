#ifndef OPENMW_COMPONENTS_TERRAIN_VIEWDATA_H
#define OPENMW_COMPONENTS_TERRAIN_VIEWDATA_H

#include <vector>
#include <deque>

#include <osg/Node>

namespace Terrain
{

    class QuadTreeNode;

    class ViewData
    {
    public:
        ViewData();

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

        bool getPersistent() const { return mPersistent; }

        osg::Object* getViewer() const { return mViewer.get(); }
        void setViewer(osg::Object* viewer) { mViewer = viewer; }

        unsigned int getFrameLastUsed() const { return mFrameLastUsed; }

        /// @return Have any nodes changed since the last frame
        bool hasChanged() const;

    private:
        std::vector<Entry> mEntries;
        unsigned int mNumEntries;
        unsigned int mFrameLastUsed;
        bool mChanged;
        bool mPersistent;
        osg::ref_ptr<osg::Object> mViewer;
    };

    class ViewDataMap : public osg::Referenced
    {
    public:
        ViewData* getViewData(osg::Object* viewer, bool ref);

        ViewData* createOrReuseView();

        void clearUnusedViews(unsigned int frame);

        void clear();

    private:
        std::list<ViewData> mViewVector;

        typedef std::map<osg::Object*, ViewData*> Map;
        Map mViews;

        std::deque<ViewData*> mUnusedViews;
    };

}

#endif
