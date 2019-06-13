#ifndef GAME_MWWORLD_SCENE_H
#define GAME_MWWORLD_SCENE_H

#include <osg/Vec4i>
#include <osg/Vec2i>

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
    struct Navigator;
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

    enum class RotationOrder
    {
        direct,
        inverse
    };

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

            void insertCell (CellStore &cell, Loading::Listener* loadingListener, bool test = false);
            osg::Vec2i mCurrentGridCenter;

            // Load and unload cells as necessary to create a cell grid with "X" and "Y" in the center
            void changeCellGrid (int playerCellX, int playerCellY, bool changeEvent = true);

            typedef std::pair<osg::Vec3f, osg::Vec4i> PositionCellGrid;

            void preloadCells(float dt);
            void preloadTeleportDoorDestinations(const osg::Vec3f& playerPos, const osg::Vec3f& predictedPos, std::vector<PositionCellGrid>& exteriorPositions);
            void preloadExteriorGrid(const osg::Vec3f& playerPos, const osg::Vec3f& predictedPos);
            void preloadFastTravelDestinations(const osg::Vec3f& playerPos, const osg::Vec3f& predictedPos, std::vector<PositionCellGrid>& exteriorPositions);

            osg::Vec4i gridCenterToBounds(const osg::Vec2i &centerCell) const;
            osg::Vec2i getNewGridCenter(const osg::Vec3f &pos, const osg::Vec2i *currentGridCenter = nullptr) const;

        public:

            Scene (MWRender::RenderingManager& rendering, MWPhysics::PhysicsSystem *physics,
                   DetourNavigator::Navigator& navigator);

            ~Scene();

            void preloadCell(MWWorld::CellStore* cell, bool preloadSurrounding=false);
            void preloadTerrain(const osg::Vec3f& pos);

            void unloadCell (CellStoreCollection::iterator iter, bool test = false);

            void loadCell (CellStore *cell, Loading::Listener* loadingListener, bool respawn, bool test = false);

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

            void updateObjectRotation(const Ptr& ptr, RotationOrder order);
            void updateObjectScale(const Ptr& ptr);

            bool isCellActive(const CellStore &cell);

            Ptr searchPtrViaActorId (int actorId);

            void preload(const std::string& mesh, bool useAnim=false);

            void testExteriorCells();
            void testInteriorCells();
    };
}

#endif
