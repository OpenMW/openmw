#include "viewdata.hpp"

#include "quadtreenode.hpp"

namespace Terrain
{

ViewData::ViewData()
    : mNumEntries(0)
    , mLastUsageTimeStamp(0.0)
    , mChanged(false)
    , mHasViewPoint(false)
    , mWorldUpdateRevision(0)
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
    mWorldUpdateRevision = other.mWorldUpdateRevision;
}

void ViewData::add(QuadTreeNode *node)
{
    unsigned int index = mNumEntries++;

    if (index+1 > mEntries.size())
        mEntries.resize(index+1);

    ViewDataEntry& entry = mEntries[index];
    if (entry.set(node))
        mChanged = true;
}

void ViewData::setViewPoint(const osg::Vec3f &viewPoint)
{
    mViewPoint = viewPoint;
    mHasViewPoint = true;
}

// NOTE: As a performance optimisation, we cache mRenderingNodes from previous frames here.
// If this cache becomes invalid (e.g. through mWorldUpdateRevision), we need to use clear() instead of reset().
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

bool ViewData::suitableToUse(const osg::Vec4i &activeGrid) const
{
    return hasViewPoint() && activeGrid == mActiveGrid && getNumEntries();
}

bool ViewData::contains(QuadTreeNode *node) const
{
    for (unsigned int i=0; i<mNumEntries; ++i)
        if (mEntries[i].mNode == node)
            return true;
    return false;
}

ViewDataEntry::ViewDataEntry()
    : mNode(nullptr)
    , mLodFlags(0)
{
}

bool ViewDataEntry::set(QuadTreeNode *node)
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

ViewData *ViewDataMap::getViewData(osg::Object *viewer, const osg::Vec3f& viewPoint, const osg::Vec4i &activeGrid, bool& needsUpdate)
{
    ViewerMap::const_iterator found = mViewers.find(viewer);
    ViewData* vd = nullptr;
    if (found == mViewers.end())
    {
        vd = createOrReuseView();
        mViewers[viewer] = vd;
    }
    else
        vd = found->second;
    needsUpdate = false;

    if (!(vd->suitableToUse(activeGrid) && (vd->getViewPoint()-viewPoint).length2() < mReuseDistance*mReuseDistance && vd->getWorldUpdateRevision() >= mWorldUpdateRevision))
    {
        float shortestDist = viewer ? mReuseDistance*mReuseDistance : std::numeric_limits<float>::max();
        const ViewData* mostSuitableView = nullptr;
        for (const ViewData* other : mUsedViews)
        {
            if (other->suitableToUse(activeGrid) && other->getWorldUpdateRevision() >= mWorldUpdateRevision)
            {
                float dist = (viewPoint-other->getViewPoint()).length2();
                if (dist < shortestDist)
                {
                    shortestDist = dist;
                    mostSuitableView = other;
                }
            }
        }
        if (mostSuitableView && mostSuitableView != vd)
        {
            vd->copyFrom(*mostSuitableView);
            return vd;
        }
        else if (!mostSuitableView)
        {
            if (vd->getWorldUpdateRevision() != mWorldUpdateRevision)
            {
                vd->setWorldUpdateRevision(mWorldUpdateRevision);
                vd->clear();
            }
            vd->setViewPoint(viewPoint);
            vd->setActiveGrid(activeGrid);
            vd->setChanged(true);
            needsUpdate = true;
        }
    }
    return vd;
}

ViewData *ViewDataMap::createOrReuseView()
{
    ViewData* vd = nullptr;
    if (mUnusedViews.size())
    {
        vd = mUnusedViews.front();
        mUnusedViews.pop_front();
    }
    else
    {
        mViewVector.emplace_back();
        vd = &mViewVector.back();
    }
    mUsedViews.push_back(vd);
    vd->setWorldUpdateRevision(mWorldUpdateRevision);
    return vd;
}

ViewData *ViewDataMap::createIndependentView() const
{
    ViewData* vd = new ViewData;
    vd->setWorldUpdateRevision(mWorldUpdateRevision);
    return vd;
}

void ViewDataMap::clearUnusedViews(double referenceTime)
{
    for (ViewerMap::iterator it = mViewers.begin(); it != mViewers.end(); )
    {
        if (it->second->getLastUsageTimeStamp() + mExpiryDelay < referenceTime)
            mViewers.erase(it++);
        else
            ++it;
    }
    for (std::deque<ViewData*>::iterator it = mUsedViews.begin(); it != mUsedViews.end(); )
    {
        if ((*it)->getLastUsageTimeStamp() + mExpiryDelay < referenceTime)
        {
            (*it)->clear();
            mUnusedViews.push_back(*it);
            it = mUsedViews.erase(it);
        }
        else
            ++it;
    }
}

void ViewDataMap::rebuildViews()
{
    ++mWorldUpdateRevision;
}

}
