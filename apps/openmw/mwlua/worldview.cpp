#include "worldview.hpp"

#include <components/esm/esmreader.hpp>
#include <components/esm/esmwriter.hpp>

#include "../mwclass/container.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/timestamp.hpp"

namespace MWLua
{

    void WorldView::update()
    {
        mObjectRegistry.update();
        mActivatorsInScene.updateList();
        mActorsInScene.updateList();
        mContainersInScene.updateList();
        mDoorsInScene.updateList();
        mItemsInScene.updateList();
    }

    void WorldView::clear()
    {
        mObjectRegistry.clear();
        mActivatorsInScene.clear();
        mActorsInScene.clear();
        mContainersInScene.clear();
        mDoorsInScene.clear();
        mItemsInScene.clear();
    }

    WorldView::ObjectGroup* WorldView::chooseGroup(const MWWorld::Ptr& ptr)
    {
        const MWWorld::Class& cls = ptr.getClass();
        if (cls.isActivator())
            return &mActivatorsInScene;
        if (cls.isActor())
            return &mActorsInScene;
        if (cls.isDoor())
            return &mDoorsInScene;
        if (typeid(cls) == typeid(MWClass::Container))
            return &mContainersInScene;
        if (cls.hasToolTip(ptr))
            return &mItemsInScene;
        return nullptr;
    }

    void WorldView::objectAddedToScene(const MWWorld::Ptr& ptr)
    {
        mObjectRegistry.registerPtr(ptr);
        ObjectGroup* group = chooseGroup(ptr);
        if (group)
            addToGroup(*group, ptr);
    }

    void WorldView::objectRemovedFromScene(const MWWorld::Ptr& ptr)
    {
        ObjectGroup* group = chooseGroup(ptr);
        if (group)
            removeFromGroup(*group, ptr);
    }

    double WorldView::getGameTimeInHours() const
    {
        MWBase::World* world = MWBase::Environment::get().getWorld();
        MWWorld::TimeStamp timeStamp = world->getTimeStamp();
        return static_cast<double>(timeStamp.getDay()) * 24 + timeStamp.getHour();
    }

    void WorldView::load(ESM::ESMReader& esm)
    {
        esm.getHNT(mGameSeconds, "LUAW");
        ObjectId lastAssignedId;
        lastAssignedId.load(esm, true);
        mObjectRegistry.setLastAssignedId(lastAssignedId);
    }

    void WorldView::save(ESM::ESMWriter& esm) const
    {
        esm.writeHNT("LUAW", mGameSeconds);
        mObjectRegistry.getLastAssignedId().save(esm, true);
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
