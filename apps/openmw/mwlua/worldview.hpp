#ifndef MWLUA_WORLDVIEW_H
#define MWLUA_WORLDVIEW_H

#include "object.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/globals.hpp"

#include <set>

namespace ESM
{
    class ESMWriter;
    class ESMReader;
}

namespace MWLua
{

    // WorldView is a kind of an extension to mwworld. It was created on initial stage of
    // OpenMW Lua development in order to minimize the risk of merge conflicts.
    // TODO: Move get*InScene functions to mwworld/scene
    // TODO: Move time-related stuff to mwworld; maybe create a new class TimeManager.
    // TODO: Remove WorldView.
    class WorldView
    {
    public:
        void update(); // Should be called every frame.
        void clear(); // Should be called every time before starting or loading a new game.

        // Whether the world is paused (i.e. game time is not changing and actors don't move).
        bool isPaused() const { return mPaused; }

        // The number of seconds passed from the beginning of the game.
        double getSimulationTime() const { return mSimulationTime; }
        void setSimulationTime(double t) { mSimulationTime = t; }

        // The game time (in game seconds) passed from the beginning of the game.
        // Note that game time generally goes faster than the simulation time.
        double getGameTime() const;
        double getGameTimeScale() const { return MWBase::Environment::get().getWorld()->getTimeScaleFactor(); }
        void setGameTimeScale(double s)
        {
            MWBase::Environment::get().getWorld()->setGlobalFloat(MWWorld::Globals::sTimeScale, s);
        }

        ObjectIdList getActivatorsInScene() const { return mActivatorsInScene.mList; }
        ObjectIdList getActorsInScene() const { return mActorsInScene.mList; }
        ObjectIdList getContainersInScene() const { return mContainersInScene.mList; }
        ObjectIdList getDoorsInScene() const { return mDoorsInScene.mList; }
        ObjectIdList getItemsInScene() const { return mItemsInScene.mList; }

        void objectAddedToScene(const MWWorld::Ptr& ptr);
        void objectRemovedFromScene(const MWWorld::Ptr& ptr);

        void load(ESM::ESMReader& esm);
        void save(ESM::ESMWriter& esm) const;

    private:
        struct ObjectGroup
        {
            void updateList();
            void clear();

            bool mChanged = false;
            ObjectIdList mList = std::make_shared<std::vector<ObjectId>>();
            std::set<ObjectId> mSet;
        };

        ObjectGroup* chooseGroup(const MWWorld::Ptr& ptr);
        void addToGroup(ObjectGroup& group, const MWWorld::Ptr& ptr);
        void removeFromGroup(ObjectGroup& group, const MWWorld::Ptr& ptr);

        ObjectGroup mActivatorsInScene;
        ObjectGroup mActorsInScene;
        ObjectGroup mContainersInScene;
        ObjectGroup mDoorsInScene;
        ObjectGroup mItemsInScene;

        double mSimulationTime = 0;
        bool mPaused = false;
    };

}

#endif // MWLUA_WORLDVIEW_H
