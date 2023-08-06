#ifndef MWLUA_WORLDVIEW_H
#define MWLUA_WORLDVIEW_H

#include <set>

#include "object.hpp"

namespace MWLua
{

    // WorldView is a kind of an extension to mwworld. It was created on initial stage of
    // OpenMW Lua development in order to minimize the risk of merge conflicts.
    // TODO: Move get*InScene functions to mwworld/scene
    // TODO: Remove WorldView.
    class WorldView
    {
    public:
        void update(); // Should be called every frame.
        void clear(); // Should be called every time before starting or loading a new game.

        ObjectIdList getActivatorsInScene() const { return mActivatorsInScene.mList; }
        ObjectIdList getActorsInScene() const { return mActorsInScene.mList; }
        ObjectIdList getContainersInScene() const { return mContainersInScene.mList; }
        ObjectIdList getDoorsInScene() const { return mDoorsInScene.mList; }
        ObjectIdList getItemsInScene() const { return mItemsInScene.mList; }
        ObjectIdList getPlayers() const { return mPlayers; }

        void objectAddedToScene(const MWWorld::Ptr& ptr);
        void objectRemovedFromScene(const MWWorld::Ptr& ptr);

        void setPlayer(const MWWorld::Ptr& player) { *mPlayers = { getId(player) }; }

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
        ObjectIdList mPlayers = std::make_shared<std::vector<ObjectId>>();
    };

}

#endif // MWLUA_WORLDVIEW_H
