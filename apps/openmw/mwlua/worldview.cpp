#include "worldview.hpp"

#include <components/esm/esmreader.hpp>
#include <components/esm/esmwriter.hpp>
#include <components/esm/loadcell.hpp>

#include "../mwbase/windowmanager.hpp"

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
        mPaused = MWBase::Environment::get().getWindowManager()->isGuiMode();
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
        // It is important to check `isMarker` first.
        // For example "prisonmarker" has class "Door" despite that it is only an invisible marker.
        if (isMarker(ptr))
            return nullptr;
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

    // TODO: If Lua scripts will use several threads at the same time, then `find*Cell` functions should have critical sections.
    MWWorld::CellStore* WorldView::findCell(const std::string& name, osg::Vec3f position)
    {
        MWBase::World* world = MWBase::Environment::get().getWorld();
        bool exterior = name.empty() || world->getExterior(name);
        if (exterior)
        {
            int cellX, cellY;
            world->positionToIndex(position.x(), position.y(), cellX, cellY);
            return world->getExterior(cellX, cellY);
        }
        else
            return world->getInterior(name);
    }

    MWWorld::CellStore* WorldView::findNamedCell(const std::string& name)
    {
        MWBase::World* world = MWBase::Environment::get().getWorld();
        const ESM::Cell* esmCell = world->getExterior(name);
        if (esmCell)
            return world->getExterior(esmCell->getGridX(), esmCell->getGridY());
        else
            return world->getInterior(name);
    }

    MWWorld::CellStore* WorldView::findExteriorCell(int x, int y)
    {
        MWBase::World* world = MWBase::Environment::get().getWorld();
        return world->getExterior(x, y);
    }

}
