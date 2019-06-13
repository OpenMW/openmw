#include "viewdata.hpp"

namespace Terrain
{

ViewData::ViewData()
    : mNumEntries(0)
    , mLastUsageTimeStamp(0.0)
    , mChanged(false)
    , mHasViewPoint(false)
{

}

ViewData::~ViewData()
{

}

void ViewData::copyFrom(const ViewData& other)
{
    mNumEntries = other.mNumEntries;
    mEntries = other.mEntries;
    mChanged = other.mChanged;
    mHasViewPoint = other.mHasViewPoint;
    mViewPoint = other.mViewPoint;
    mActiveGrid = other.mActiveGrid;
}

void ViewData::add(QuadTreeNode *node)
{
    unsigned int index = mNumEntries++;

    if (index+1 > mEntries.size())
        mEntries.resize(index+1);

    Entry& entry = mEntries[index];
    if (entry.set(node))
        mChanged = true;
}

unsigned int ViewData::getNumEntries() const
{
    return mNumEntries;
}

ViewData::Entry &ViewData::getEntry(unsigned int i)
{
    return mEntries[i];
}

bool ViewData::hasChanged() const
{
    return mChanged;
}

bool ViewData::hasViewPoint() const
{
    return mHasViewPoint;
}

void ViewData::setViewPoint(const osg::Vec3f &viewPoint)
{
    mViewPoint = viewPoint;
    mHasViewPoint = true;
}

const osg::Vec3f& ViewData::getViewPoint() const
{
    return mViewPoint;
}

void ViewData::reset()
{
    // clear any unused entries
    for (unsigned int i=mNumEntries; i<mEntries.size(); ++i)
        mEntries[i].set(nullptr);

    // reset index for next frame
    mNumEntries = 0;
    mChanged = false;
}

void ViewData::clear()
{
    for (unsigned int i=0; i<mEntries.size(); ++i)
        mEntries[i].set(nullptr);
    mNumEntries = 0;
    mLastUsageTimeStamp = 0;
    mChanged = false;
    mHasViewPoint = false;
}

bool ViewData::contains(QuadTreeNode *node)
{
    for (unsigned int i=0; i<mNumEntries; ++i)
        if (mEntries[i].mNode == node)
            return true;
    return false;
}

ViewData::Entry::Entry()
    : mNode(nullptr)
    , mLodFlags(0)
{

}

bool ViewData::Entry::set(QuadTreeNode *node)
{
    if (node == mNode)
        return false;
    else
    {
        mNode = node;
        // clear cached data
        mRenderingNode = nullptr;
        return true;
    }
}

bool suitable(ViewData* vd, const osg::Vec3f& viewPoint, float& maxDist, const osg::Vec4i& activeGrid)
{
    return vd->hasViewPoint() && (vd->getViewPoint() - viewPoint).length2() < maxDist*maxDist && vd->getActiveGrid() == activeGrid;
}

ViewData *ViewDataMap::getViewData(osg::Object *viewer, const osg::Vec3f& viewPoint, const osg::Vec4i &activeGrid, bool& needsUpdate)
{
    Map::const_iterator found = mViews.find(viewer);
    ViewData* vd = nullptr;
    if (found == mViews.end())
    {
        vd = createOrReuseView();
        mViews[viewer] = vd;
    }
    else
        vd = found->second;

    if (!suitable(vd, viewPoint, mReuseDistance, activeGrid))
    {
        for (Map::const_iterator other = mViews.begin(); other != mViews.end(); ++other)
        {
            if (suitable(other->second, viewPoint, mReuseDistance, activeGrid) && other->second->getNumEntries())
            {
                vd->copyFrom(*other->second);
                needsUpdate = false;
                return vd;
            }
        }
        vd->setViewPoint(viewPoint);
        vd->setActiveGrid(activeGrid);
        needsUpdate = true;
    }
    else
        needsUpdate = false;

    return vd;
}

ViewData *ViewDataMap::createOrReuseView()
{
    if (mUnusedViews.size())
    {
        ViewData* vd = mUnusedViews.front();
        mUnusedViews.pop_front();
        return vd;
    }
    else
    {
        mViewVector.push_back(ViewData());
        return &mViewVector.back();
    }
}

void ViewDataMap::clearUnusedViews(double referenceTime)
{
    for (Map::iterator it = mViews.begin(); it != mViews.end(); )
    {
        ViewData* vd = it->second;
        if (vd->getLastUsageTimeStamp() + mExpiryDelay < referenceTime)
        {
            vd->clear();
            mUnusedViews.push_back(vd);
            mViews.erase(it++);
        }
        else
            ++it;
    }
}

void ViewDataMap::clear()
{
    mViews.clear();
    mUnusedViews.clear();
    mViewVector.clear();
}

}
