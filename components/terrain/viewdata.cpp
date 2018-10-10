#include "viewdata.hpp"

namespace Terrain
{

ViewData::ViewData()
    : mNumEntries(0)
    , mFrameLastUsed(0)
    , mChanged(false)
    , mHasEyePoint(false)
{

}

ViewData::~ViewData()
{

}

void ViewData::add(QuadTreeNode *node, bool visible)
{
    unsigned int index = mNumEntries++;

    if (index+1 > mEntries.size())
        mEntries.resize(index+1);

    Entry& entry = mEntries[index];
    if (entry.set(node, visible))
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

bool ViewData::hasEyePoint() const
{
    return mHasEyePoint;
}

void ViewData::setEyePoint(const osg::Vec3f &eye)
{
    mEyePoint = eye;
    mHasEyePoint = true;
}

const osg::Vec3f& ViewData::getEyePoint() const
{
    return mEyePoint;
}

void ViewData::reset(unsigned int frame)
{
    // clear any unused entries
    for (unsigned int i=mNumEntries; i<mEntries.size(); ++i)
        mEntries[i].set(nullptr, false);

    // reset index for next frame
    mNumEntries = 0;
    mChanged = false;

    mFrameLastUsed = frame;
}

void ViewData::clear()
{
    for (unsigned int i=0; i<mEntries.size(); ++i)
        mEntries[i].set(nullptr, false);
    mNumEntries = 0;
    mFrameLastUsed = 0;
    mChanged = false;
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
    , mVisible(true)
    , mLodFlags(0)
{

}

bool ViewData::Entry::set(QuadTreeNode *node, bool visible)
{
    mVisible = visible;
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

ViewData *ViewDataMap::getViewData(osg::Object *viewer)
{
    Map::const_iterator found = mViews.find(viewer);
    if (found == mViews.end())
    {
        ViewData* vd = createOrReuseView();
        vd->setViewer(viewer);
        mViews[viewer] = vd;
        return vd;
    }
    else
        return found->second;
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

void ViewDataMap::clearUnusedViews(unsigned int frame)
{
    for (Map::iterator it = mViews.begin(); it != mViews.end(); )
    {
        ViewData* vd = it->second;
        if (vd->getFrameLastUsed() + 2 < frame)
        {
            vd->setViewer(nullptr);
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

void ViewDataMap::setDefaultViewer(osg::Object *viewer)
{
    mDefaultViewer = viewer;
}

ViewData* ViewDataMap::getDefaultView()
{
    return getViewData(mDefaultViewer);
}


}
