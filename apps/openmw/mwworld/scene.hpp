#ifndef GAME_MWWORLD_SCENE_H
#define GAME_MWWORLD_SCENE_H

#include <osg/Vec2i>
#include <osg/Vec4i>
#include <osg/ref_ptr>

#include "positioncellgrid.hpp"
#include "ptr.hpp"

#include <memory>
#include <optional>
#include <set>
#include <vector>

#include <components/esm/exteriorcelllocation.hpp>
#include <components/misc/constants.hpp>

namespace osg
{
    class Vec3f;
    class Stats;
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
    class UpdateGuard;
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

namespace SceneUtil
{
    class WorkItem;
}

namespace MWWorld
{
    class Player;
    class CellStore;
    class CellPreloader;
    class World;

    enum class RotationOrder
    {
        direct,
        inverse
    };

    class Scene
    {
    public:
        using CellStoreCollection = std::set<CellStore*, std::less<>>;

    private:
        struct ChangeCellGridRequest
        {
            osg::Vec3f mPosition;
            ESM::ExteriorCellLocation mCellIndex;
            bool mChangeEvent;
        };

        CellStore* mCurrentCell; // the cell the player is in
        CellStoreCollection mActiveCells;
        bool mCellChanged;
        bool mCellLoaded = false;
        MWWorld::World& mWorld;
        MWPhysics::PhysicsSystem* mPhysics;
        MWRender::RenderingManager& mRendering;
        DetourNavigator::Navigator& mNavigator;
        std::unique_ptr<CellPreloader> mPreloader;
        float mCellLoadingThreshold;
        float mPreloadDistance;
        bool mPreloadEnabled;

        bool mPreloadExteriorGrid;
        bool mPreloadDoors;
        bool mPreloadFastTravel;
        float mPredictionTime;
        float mLowestPoint;

        int mHalfGridSize = Constants::CellGridRadius;

        osg::Vec3f mLastPlayerPos;

        std::vector<ESM::RefNum> mPagedRefs;

        std::vector<osg::ref_ptr<SceneUtil::WorkItem>> mWorkItems;

        std::optional<ChangeCellGridRequest> mChangeCellGridRequest;

        void insertCell(CellStore& cell, Loading::Listener* loadingListener,
            const DetourNavigator::UpdateGuard* navigatorUpdateGuard);

        osg::Vec2i mCurrentGridCenter;

        // Load and unload cells as necessary to create a cell grid with "X" and "Y" in the center
        void changeCellGrid(const osg::Vec3f& pos, ESM::ExteriorCellLocation playerCellIndex, bool changeEvent = true);

        void requestChangeCellGrid(const osg::Vec3f& position, const osg::Vec2i& cell, bool changeEvent = true);

        void preloadCells(float dt);
        void preloadTeleportDoorDestinations(const osg::Vec3f& playerPos, const osg::Vec3f& predictedPos);
        void preloadExteriorGrid(const osg::Vec3f& playerPos, const osg::Vec3f& predictedPos);
        void preloadFastTravelDestinations(
            const osg::Vec3f& playerPos, std::vector<PositionCellGrid>& exteriorPositions);
        void preloadCellWithSurroundings(MWWorld::CellStore& cell);
        void preloadCell(MWWorld::CellStore& cell);
        void preloadTerrain(const osg::Vec3f& pos, ESM::RefId worldspace, bool sync = false);

        osg::Vec4i gridCenterToBounds(const osg::Vec2i& centerCell) const;
        osg::Vec2i getNewGridCenter(const osg::Vec3f& pos, const osg::Vec2i* currentGridCenter = nullptr) const;

        void unloadCell(CellStore* cell, const DetourNavigator::UpdateGuard* navigatorUpdateGuard);
        void loadCell(CellStore& cell, Loading::Listener* loadingListener, bool respawn, const osg::Vec3f& position,
            const DetourNavigator::UpdateGuard* navigatorUpdateGuard);

    public:
        Scene(MWWorld::World& world, MWRender::RenderingManager& rendering, MWPhysics::PhysicsSystem* physics,
            DetourNavigator::Navigator& navigator);

        ~Scene();

        void reloadTerrain();

        void playerMoved(const osg::Vec3f& pos);

        void changePlayerCell(CellStore& newCell, const ESM::Position& position, bool adjustPlayerPos);

        CellStore* getCurrentCell();

        const CellStoreCollection& getActiveCells() const;

        bool hasCellChanged() const;
        ///< Has the set of active cells changed, since the last frame?

        bool hasCellLoaded() const { return mCellLoaded; }

        void resetCellLoaded() { mCellLoaded = false; }

        void changeToInteriorCell(
            std::string_view cellName, const ESM::Position& position, bool adjustPlayerPos, bool changeEvent = true);
        ///< Move to interior cell.
        /// @param changeEvent Set cellChanged flag?

        void changeToExteriorCell(
            const ESM::RefId& extCellId, const ESM::Position& position, bool adjustPlayerPos, bool changeEvent = true);
        ///< Move to exterior cell.
        /// @param changeEvent Set cellChanged flag?

        void clear();
        ///< Change into a void

        void markCellAsUnchanged();

        void update(float duration);

        void addObjectToScene(const Ptr& ptr);
        ///< Add an object that already exists in the world model to the scene.

        void removeObjectFromScene(const Ptr& ptr, bool keepActive = false);
        ///< Remove an object from the scene, but not from the world model.

        void addPostponedPhysicsObjects();

        void removeFromPagedRefs(const Ptr& ptr);

        bool isPagedRef(const Ptr& ptr) const;

        void updateObjectRotation(const Ptr& ptr, RotationOrder order);
        void updateObjectScale(const Ptr& ptr);

        bool isCellActive(const CellStore& cell);

        void preload(const std::string& mesh, bool useAnim = false);

        void testExteriorCells();
        void testInteriorCells();

        void reportStats(unsigned int frameNumber, osg::Stats& stats) const;
    };
}

#endif
