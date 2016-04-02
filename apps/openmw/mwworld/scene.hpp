#ifndef GAME_MWWORLD_SCENE_H
#define GAME_MWWORLD_SCENE_H

#include "ptr.hpp"
#include "globals.hpp"

#include <set>
#include <memory>

namespace osg
{
    class Vec3f;
}

namespace ESM
{
    struct Position;
}

namespace Files
{
    class Collections;
}

namespace Loading
{
    class Listener;
}

namespace MWRender
{
    class SkyManager;
    class RenderingManager;
}

namespace MWPhysics
{
    class PhysicsSystem;
}

namespace MWWorld
{
    class Player;
    class CellStore;
    class CellPreloader;

    class Scene
    {
        public:

            typedef std::set<CellStore *> CellStoreCollection;

        private:

            CellStore* mCurrentCell; // the cell the player is in
            CellStoreCollection mActiveCells;
            bool mCellChanged;
            MWPhysics::PhysicsSystem *mPhysics;
            MWRender::RenderingManager& mRendering;
            std::auto_ptr<CellPreloader> mPreloader;
            float mPreloadTimer;
            int mHalfGridSize;
            float mCellLoadingThreshold;
            float mPreloadDistance;
            bool mPreloadEnabled;

            bool mPreloadExteriorGrid;
            bool mPreloadDoors;
            bool mPreloadFastTravel;

            void insertCell (CellStore &cell, bool rescale, Loading::Listener* loadingListener);

            // Load and unload cells as necessary to create a cell grid with "X" and "Y" in the center
            void changeCellGrid (int X, int Y, bool changeEvent = true);

            void getGridCenter(int& cellX, int& cellY);

            void preloadCells();
            void preloadTeleportDoorDestinations();
            void preloadExteriorGrid();
            void preloadFastTravelDestinations();

            void preloadCell(MWWorld::CellStore* cell, bool preloadSurrounding=false);

        public:

            Scene (MWRender::RenderingManager& rendering, MWPhysics::PhysicsSystem *physics);

            ~Scene();

            void unloadCell (CellStoreCollection::iterator iter);

            void loadCell (CellStore *cell, Loading::Listener* loadingListener, bool respawn);

            void playerMoved (const osg::Vec3f& pos);

            void changePlayerCell (CellStore* newCell, const ESM::Position& position, bool adjustPlayerPos);

            CellStore *getCurrentCell();

            const CellStoreCollection& getActiveCells () const;

            bool hasCellChanged() const;
            ///< Has the set of active cells changed, since the last frame?

            void changeToInteriorCell (const std::string& cellName, const ESM::Position& position, bool adjustPlayerPos, bool changeEvent=true);
            ///< Move to interior cell.
            /// @param changeEvent Set cellChanged flag?

            void changeToExteriorCell (const ESM::Position& position, bool adjustPlayerPos, bool changeEvent=true);
            ///< Move to exterior cell.
            /// @param changeEvent Set cellChanged flag?

            void changeToVoid();
            ///< Change into a void

            void markCellAsUnchanged();

            void update (float duration, bool paused);

            void addObjectToScene (const Ptr& ptr);
            ///< Add an object that already exists in the world model to the scene.

            void removeObjectFromScene (const Ptr& ptr);
            ///< Remove an object from the scene, but not from the world model.

            void updateObjectRotation (const Ptr& ptr, bool inverseRotationOrder);
            void updateObjectScale(const Ptr& ptr);

            bool isCellActive(const CellStore &cell);

            Ptr searchPtrViaActorId (int actorId);
    };
}

#endif
