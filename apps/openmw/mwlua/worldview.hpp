#ifndef MWLUA_WORLDVIEW_H
#define MWLUA_WORLDVIEW_H

#include "object.hpp"

namespace MWLua
{

    // Tracks all used game objects.
    class WorldView
    {
    public:
        void update();  // Should be called every frame.
        void clear();  // Should be called every time before starting or loading a new game.

        // Returns the number of seconds passed from the beginning of the game.
        double getGameTimeInSeconds() const { return mGameSeconds; }
        void setGameTimeInSeconds(double t) { mGameSeconds = t; }

        // Returns the number of game hours passed from the beginning of the game.
        // Note that the number of seconds in a game hour is not fixed.
        double getGameTimeInHours() const;

        ObjectIdList getActorsInScene() const { return mActorsInScene.mList; }
        ObjectIdList getItemsInScene() const { return mItemsInScene.mList; }

        ObjectRegistry* getObjectRegistry() { return &mObjectRegistry; }

        void objectUnloaded(const MWWorld::Ptr& ptr) { mObjectRegistry.deregisterPtr(ptr); }

        void objectAddedToScene(const MWWorld::Ptr& ptr);
        void objectRemovedFromScene(const MWWorld::Ptr& ptr);

    private:
        struct ObjectGroup
        {
            void updateList();
            void clear();

            bool mChanged = false;
            ObjectIdList mList = std::make_shared<std::vector<ObjectId>>();
            std::set<ObjectId> mSet;
        };

        void addToGroup(ObjectGroup& group, const MWWorld::Ptr& ptr);
        void removeFromGroup(ObjectGroup& group, const MWWorld::Ptr& ptr);

        ObjectRegistry mObjectRegistry;
        ObjectGroup mActorsInScene;
        ObjectGroup mItemsInScene;

        double mGameSeconds = 0;
    };

}

#endif // MWLUA_WORLDVIEW_H
