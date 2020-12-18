#include "worldview.hpp"

#include "../mwworld/class.hpp"

namespace MWLua
{

    void WorldView::update()
    {
        mObjectRegistry.update();
        mActorsInScene.updateList();
        mItemsInScene.updateList();
    }

    void WorldView::clear()
    {
        mObjectRegistry.clear();
        mActorsInScene.clear();
        mItemsInScene.clear();
    }

    void WorldView::objectAddedToScene(const MWWorld::Ptr& ptr)
    {
        if (ptr.getClass().isActor())
            addToGroup(mActorsInScene, ptr);
        else
            addToGroup(mItemsInScene, ptr);
    }

    void WorldView::objectRemovedFromScene(const MWWorld::Ptr& ptr)
    {
        if (ptr.getClass().isActor())
            removeFromGroup(mActorsInScene, ptr);
        else
            removeFromGroup(mItemsInScene, ptr);
    }

    void WorldView::ObjectGroup::updateList()
    {
        if (mChanged)
        {
            mList->clear();
            for (const ObjectId& id : mSet)
                mList->push_back(id);
            mChanged = false;
        }
    }

    void WorldView::ObjectGroup::clear()
    {
        mChanged = false;
        mList->clear();
        mSet.clear();
    }

    void WorldView::addToGroup(ObjectGroup& group, const MWWorld::Ptr& ptr)
    {
        group.mSet.insert(getId(ptr));
        group.mChanged = true;
    }

    void WorldView::removeFromGroup(ObjectGroup& group, const MWWorld::Ptr& ptr)
    {
        group.mSet.erase(getId(ptr));
        group.mChanged = true;
    }

}
