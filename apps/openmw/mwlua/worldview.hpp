#ifndef MWLUA_WORLDVIEW_H
#define MWLUA_WORLDVIEW_H

#include "object.hpp"

namespace ESM
{
    class ESMWriter;
    class ESMReader;
}

namespace MWLua
{

    // Tracks all used game objects.
    class WorldView
    {
    public:
        void update();  // Should be called every frame.
        void clear();  // Should be called every time before starting or loading a new game.

        // Whether the world is paused (i.e. game time is not changing and actors don't move).
        bool isPaused() const { return mPaused; }

        // Returns the number of seconds passed from the beginning of the game.
        double getGameTimeInSeconds() const { return mGameSeconds; }
        void setGameTimeInSeconds(double t) { mGameSeconds = t; }

        // Returns the number of game hours passed from the beginning of the game.
        // Note that the number of seconds in a game hour is not fixed.
        double getGameTimeInHours() const;

        ObjectIdList getActivatorsInScene() const { return mActivatorsInScene.mList; }
        ObjectIdList getActorsInScene() const { return mActorsInScene.mList; }
        ObjectIdList getContainersInScene() const { return mContainersInScene.mList; }
        ObjectIdList getDoorsInScene() const { return mDoorsInScene.mList; }
        ObjectIdList getItemsInScene() const { return mItemsInScene.mList; }

        ObjectRegistry* getObjectRegistry() { return &mObjectRegistry; }

        void objectUnloaded(const MWWorld::Ptr& ptr) { mObjectRegistry.deregisterPtr(ptr); }

        void objectAddedToScene(const MWWorld::Ptr& ptr);
        void objectRemovedFromScene(const MWWorld::Ptr& ptr);

        // Returns list of objects that meets the `query` criteria.
        // If onlyActive = true, then search only among the objects that are currently in the scene.
        // TODO: ObjectIdList selectObjects(const Queries::Query& query, bool onlyActive);

        MWWorld::CellStore* findCell(const std::string& name, osg::Vec3f position);
        MWWorld::CellStore* findNamedCell(const std::string& name);
        MWWorld::CellStore* findExteriorCell(int x, int y);

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

        ObjectRegistry mObjectRegistry;
        ObjectGroup mActivatorsInScene;
        ObjectGroup mActorsInScene;
        ObjectGroup mContainersInScene;
        ObjectGroup mDoorsInScene;
        ObjectGroup mItemsInScene;

        double mGameSeconds = 0;
        bool mPaused = false;
    };

}

#endif // MWLUA_WORLDVIEW_H
