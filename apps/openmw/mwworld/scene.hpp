#ifndef GAME_MWWORLD_SCENE_H
#define GAME_MWWORLD_SCENE_H

#include "ptr.hpp"
#include "globals.hpp"

#include <set>
#include <memory>
#include <unordered_map>

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

namespace DetourNavigator
{
    class Navigator;
    class Water;
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
            DetourNavigator::Navigator& mNavigator;
            std::unique_ptr<CellPreloader> mPreloader;
            float mPreloadTimer;
            int mHalfGridSize;
            float mCellLoadingThreshold;
            float mPreloadDistance;
            bool mPreloadEnabled;

            bool mPreloadExteriorGrid;
            bool mPreloadDoors;
            bool mPreloadFastTravel;
            float mPredictionTime;

            osg::Vec3f mLastPlayerPos;

            void insertCell (CellStore &cell, bool rescale, Loading::Listener* loadingListener);

            // Load and unload cells as necessary to create a cell grid with "X" and "Y" in the center
            void changeCellGrid (int playerCellX, int playerCellY, bool changeEvent = true);

            void getGridCenter(int& cellX, int& cellY);

            void preloadCells(float dt);
            void preloadTeleportDoorDestinations(const osg::Vec3f& playerPos, const osg::Vec3f& predictedPos, std::vector<osg::Vec3f>& exteriorPositions);
            void preloadExteriorGrid(const osg::Vec3f& playerPos, const osg::Vec3f& predictedPos);
            void preloadFastTravelDestinations(const osg::Vec3f& playerPos, const osg::Vec3f& predictedPos, std::vector<osg::Vec3f>& exteriorPositions);

        public:

            Scene (MWRender::RenderingManager& rendering, MWPhysics::PhysicsSystem *physics,
                   DetourNavigator::Navigator& navigator);

            ~Scene();

            void preloadCell(MWWorld::CellStore* cell, bool preloadSurrounding=false);
            void preloadTerrain(const osg::Vec3f& pos);

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

            void clear();
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

            void preload(const std::string& mesh, bool useAnim=false);
    };
}

#endif
