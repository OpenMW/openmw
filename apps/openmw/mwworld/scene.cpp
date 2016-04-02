#include "scene.hpp"

#include <limits>
#include <iostream>

#include <components/loadinglistener/loadinglistener.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/settings/settings.hpp>
#include <components/resource/resourcesystem.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwrender/renderingmanager.hpp"

#include "../mwphysics/physicssystem.hpp"

#include "player.hpp"
#include "localscripts.hpp"
#include "esmstore.hpp"
#include "class.hpp"
#include "cellvisitors.hpp"
#include "cellstore.hpp"
#include "cellpreloader.hpp"

namespace
{

    void addObject(const MWWorld::Ptr& ptr, MWPhysics::PhysicsSystem& physics,
                   MWRender::RenderingManager& rendering)
    {
        std::string model = Misc::ResourceHelpers::correctActorModelPath(ptr.getClass().getModel(ptr), rendering.getResourceSystem()->getVFS());
        std::string id = ptr.getCellRef().getRefId();
        if (id == "prisonmarker" || id == "divinemarker" || id == "templemarker" || id == "northmarker")
            model = ""; // marker objects that have a hardcoded function in the game logic, should be hidden from the player
        ptr.getClass().insertObjectRendering(ptr, model, rendering);
        ptr.getClass().insertObject (ptr, model, physics);

        if (ptr.getClass().isActor())
            rendering.addWaterRippleEmitter(ptr);
    }

    void updateObjectRotation (const MWWorld::Ptr& ptr, MWPhysics::PhysicsSystem& physics,
                                    MWRender::RenderingManager& rendering, bool inverseRotationOrder)
    {
        if (ptr.getRefData().getBaseNode() != NULL)
        {
            osg::Quat worldRotQuat(ptr.getRefData().getPosition().rot[2], osg::Vec3(0,0,-1));
            if (!ptr.getClass().isActor())
            {
                float xr = ptr.getRefData().getPosition().rot[0];
                float yr = ptr.getRefData().getPosition().rot[1];
                if (!inverseRotationOrder)
                    worldRotQuat = worldRotQuat * osg::Quat(yr, osg::Vec3(0,-1,0)) *
                        osg::Quat(xr, osg::Vec3(-1,0,0));
                else
                    worldRotQuat = osg::Quat(xr, osg::Vec3(-1,0,0)) * osg::Quat(yr, osg::Vec3(0,-1,0)) * worldRotQuat;
            }

            rendering.rotateObject(ptr, worldRotQuat);
            physics.updateRotation(ptr);
        }
    }

    void updateObjectScale(const MWWorld::Ptr& ptr, MWPhysics::PhysicsSystem& physics,
                            MWRender::RenderingManager& rendering)
    {
        if (ptr.getRefData().getBaseNode() != NULL)
        {
            float scale = ptr.getCellRef().getScale();
            osg::Vec3f scaleVec (scale, scale, scale);
            ptr.getClass().adjustScale(ptr, scaleVec, true);
            rendering.scaleObject(ptr, scaleVec);

            physics.updateScale(ptr);
        }
    }

    struct InsertVisitor
    {
        MWWorld::CellStore& mCell;
        bool mRescale;
        Loading::Listener& mLoadingListener;
        MWPhysics::PhysicsSystem& mPhysics;
        MWRender::RenderingManager& mRendering;

        std::vector<MWWorld::Ptr> mToInsert;

        InsertVisitor (MWWorld::CellStore& cell, bool rescale, Loading::Listener& loadingListener,
            MWPhysics::PhysicsSystem& physics, MWRender::RenderingManager& rendering);

        bool operator() (const MWWorld::Ptr& ptr);
        void insert();
    };

    InsertVisitor::InsertVisitor (MWWorld::CellStore& cell, bool rescale,
        Loading::Listener& loadingListener, MWPhysics::PhysicsSystem& physics,
        MWRender::RenderingManager& rendering)
    : mCell (cell), mRescale (rescale), mLoadingListener (loadingListener),
      mPhysics (physics),
      mRendering (rendering)
    {}

    bool InsertVisitor::operator() (const MWWorld::Ptr& ptr)
    {
        // do not insert directly as we can't modify the cell from within the visitation
        // CreatureLevList::insertObjectRendering may spawn a new creature
        mToInsert.push_back(ptr);
        return true;
    }

    void InsertVisitor::insert()
    {
        for (std::vector<MWWorld::Ptr>::iterator it = mToInsert.begin(); it != mToInsert.end(); ++it)
        {
            MWWorld::Ptr ptr = *it;
            if (mRescale)
            {
                if (ptr.getCellRef().getScale()<0.5)
                    ptr.getCellRef().setScale(0.5);
                else if (ptr.getCellRef().getScale()>2)
                    ptr.getCellRef().setScale(2);
            }

            if (!ptr.getRefData().isDeleted() && ptr.getRefData().isEnabled())
            {
                try
                {
                    addObject(ptr, mPhysics, mRendering);
                    updateObjectRotation(ptr, mPhysics, mRendering, false);
                }
                catch (const std::exception& e)
                {
                    std::string error ("error during rendering '" + ptr.getCellRef().getRefId() + "': ");
                    std::cerr << error + e.what() << std::endl;
                }
            }

            mLoadingListener.increaseProgress (1);
        }
    }

    struct AdjustPositionVisitor
    {
        bool operator() (const MWWorld::Ptr& ptr)
        {
            if (!ptr.getRefData().isDeleted() && ptr.getRefData().isEnabled())
                ptr.getClass().adjustPosition (ptr, false);
            return true;
        }
    };

}


namespace MWWorld
{

    void Scene::updateObjectRotation (const Ptr& ptr, bool inverseRotationOrder)
    {
        ::updateObjectRotation(ptr, *mPhysics, mRendering, inverseRotationOrder);
    }

    void Scene::updateObjectScale(const Ptr &ptr)
    {
        ::updateObjectScale(ptr, *mPhysics, mRendering);
    }

    void Scene::getGridCenter(int &cellX, int &cellY)
    {
        int maxX = std::numeric_limits<int>::min();
        int maxY = std::numeric_limits<int>::min();
        int minX = std::numeric_limits<int>::max();
        int minY = std::numeric_limits<int>::max();
        CellStoreCollection::iterator iter = mActiveCells.begin();
        while (iter!=mActiveCells.end())
        {
            assert ((*iter)->getCell()->isExterior());
            int x = (*iter)->getCell()->getGridX();
            int y = (*iter)->getCell()->getGridY();
            maxX = std::max(x, maxX);
            maxY = std::max(y, maxY);
            minX = std::min(x, minX);
            minY = std::min(y, minY);
            ++iter;
        }
        cellX = (minX + maxX) / 2;
        cellY = (minY + maxY) / 2;
    }

    void Scene::update (float duration, bool paused)
    {
        if (mPreloadEnabled)
        {
            mPreloadTimer += duration;
            if (mPreloadTimer > 0.25f)
            {
                preloadCells();
                mPreloadTimer = 0.f;
            }
        }

        mRendering.update (duration, paused);

        mPreloader->updateCache(mRendering.getReferenceTime());
    }

    void Scene::unloadCell (CellStoreCollection::iterator iter)
    {
        std::cout << "Unloading cell\n";
        ListAndResetObjectsVisitor visitor;

        (*iter)->forEach<ListAndResetObjectsVisitor>(visitor);
        for (std::vector<MWWorld::Ptr>::const_iterator iter2 (visitor.mObjects.begin());
            iter2!=visitor.mObjects.end(); ++iter2)
        {
            mPhysics->remove(*iter2);
        }

        if ((*iter)->getCell()->isExterior())
        {
            ESM::Land* land =
                MWBase::Environment::get().getWorld()->getStore().get<ESM::Land>().search(
                    (*iter)->getCell()->getGridX(),
                    (*iter)->getCell()->getGridY()
                );
            if (land && land->mDataTypes&ESM::Land::DATA_VHGT)
                mPhysics->removeHeightField ((*iter)->getCell()->getGridX(), (*iter)->getCell()->getGridY());
        }

        MWBase::Environment::get().getMechanicsManager()->drop (*iter);

        mRendering.removeCell(*iter);
        MWBase::Environment::get().getWindowManager()->removeCell(*iter);

        MWBase::Environment::get().getWorld()->getLocalScripts().clearCell (*iter);

        MWBase::Environment::get().getSoundManager()->stopSound (*iter);
        mActiveCells.erase(*iter);
    }

    void Scene::loadCell (CellStore *cell, Loading::Listener* loadingListener, bool respawn)
    {
        std::pair<CellStoreCollection::iterator, bool> result = mActiveCells.insert(cell);

        if(result.second)
        {
            std::cout << "Loading cell " << cell->getCell()->getDescription() << std::endl;

            float verts = ESM::Land::LAND_SIZE;
            float worldsize = ESM::Land::REAL_SIZE;

            // Load terrain physics first...
            if (cell->getCell()->isExterior())
            {
                ESM::Land* land =
                    MWBase::Environment::get().getWorld()->getStore().get<ESM::Land>().search(
                        cell->getCell()->getGridX(),
                        cell->getCell()->getGridY()
                    );
                if (land && land->mDataTypes&ESM::Land::DATA_VHGT) {
                    // Actually only VHGT is needed here, but we'll need the rest for rendering anyway.
                    // Load everything now to reduce IO overhead.
                    const int flags = ESM::Land::DATA_VCLR|ESM::Land::DATA_VHGT|ESM::Land::DATA_VNML|ESM::Land::DATA_VTEX;

                    const ESM::Land::LandData *data = land->getLandData (flags);
                    mPhysics->addHeightField (data->mHeights, cell->getCell()->getGridX(), cell->getCell()->getGridY(),
                        worldsize / (verts-1), verts);
                }
                else
                {
                    static std::vector<float> defaultHeight;
                    defaultHeight.resize(verts*verts, ESM::Land::DEFAULT_HEIGHT);
                    mPhysics->addHeightField (&defaultHeight[0], cell->getCell()->getGridX(), cell->getCell()->getGridY(),
                            worldsize / (verts-1), verts);
                }
            }

            // register local scripts
            // do this before insertCell, to make sure we don't add scripts from levelled creature spawning twice
            MWBase::Environment::get().getWorld()->getLocalScripts().addCell (cell);

            if (respawn)
                cell->respawn();

            // ... then references. This is important for adjustPosition to work correctly.
            /// \todo rescale depending on the state of a new GMST
            insertCell (*cell, true, loadingListener);

            mRendering.addCell(cell);
            bool waterEnabled = cell->getCell()->hasWater() || cell->isExterior();
            float waterLevel = cell->getWaterLevel();
            mRendering.setWaterEnabled(waterEnabled);
            if (waterEnabled)
            {
                mPhysics->enableWater(waterLevel);
                mRendering.setWaterHeight(waterLevel);
            }
            else
                mPhysics->disableWater();

            if (!cell->isExterior() && !(cell->getCell()->mData.mFlags & ESM::Cell::QuasiEx))
                mRendering.configureAmbient(cell->getCell());
        }

        mPreloader->notifyLoaded(cell);
    }

    void Scene::changeToVoid()
    {
        CellStoreCollection::iterator active = mActiveCells.begin();
        while (active!=mActiveCells.end())
            unloadCell (active++);
        assert(mActiveCells.empty());
        mCurrentCell = NULL;
    }

    void Scene::playerMoved(const osg::Vec3f &pos)
    {
        if (!mCurrentCell || !mCurrentCell->isExterior())
            return;

        // figure out the center of the current cell grid (*not* necessarily mCurrentCell, which is the cell the player is in)
        int cellX, cellY;
        getGridCenter(cellX, cellY);
        float centerX, centerY;
        MWBase::Environment::get().getWorld()->indexToPosition(cellX, cellY, centerX, centerY, true);
        const float maxDistance = 8192/2 + mCellLoadingThreshold; // 1/2 cell size + threshold
        float distance = std::max(std::abs(centerX-pos.x()), std::abs(centerY-pos.y()));
        if (distance > maxDistance)
        {
            int newX, newY;
            MWBase::Environment::get().getWorld()->positionToIndex(pos.x(), pos.y(), newX, newY);
            changeCellGrid(newX, newY);
            //mRendering.updateTerrain();
        }
    }

    void Scene::changeCellGrid (int X, int Y, bool changeEvent)
    {
        Loading::Listener* loadingListener = MWBase::Environment::get().getWindowManager()->getLoadingScreen();
        Loading::ScopedLoad load(loadingListener);

        //mRendering.enableTerrain(true);

        std::string loadingExteriorText = "#{sLoadingMessage3}";
        loadingListener->setLabel(loadingExteriorText);

        CellStoreCollection::iterator active = mActiveCells.begin();
        while (active!=mActiveCells.end())
        {
            if ((*active)->getCell()->isExterior())
            {
                if (std::abs (X-(*active)->getCell()->getGridX())<=mHalfGridSize &&
                    std::abs (Y-(*active)->getCell()->getGridY())<=mHalfGridSize)
                {
                    // keep cells within the new grid
                    ++active;
                    continue;
                }
            }
            unloadCell (active++);
        }

        int refsToLoad = 0;
        // get the number of refs to load
        for (int x=X-mHalfGridSize; x<=X+mHalfGridSize; ++x)
        {
            for (int y=Y-mHalfGridSize; y<=Y+mHalfGridSize; ++y)
            {
                CellStoreCollection::iterator iter = mActiveCells.begin();

                while (iter!=mActiveCells.end())
                {
                    assert ((*iter)->getCell()->isExterior());

                    if (x==(*iter)->getCell()->getGridX() &&
                        y==(*iter)->getCell()->getGridY())
                        break;

                    ++iter;
                }

                if (iter==mActiveCells.end())
                    refsToLoad += MWBase::Environment::get().getWorld()->getExterior(x, y)->count();
            }
        }

        loadingListener->setProgressRange(refsToLoad);

        // Load cells
        for (int x=X-mHalfGridSize; x<=X+mHalfGridSize; ++x)
        {
            for (int y=Y-mHalfGridSize; y<=Y+mHalfGridSize; ++y)
            {
                CellStoreCollection::iterator iter = mActiveCells.begin();

                while (iter!=mActiveCells.end())
                {
                    assert ((*iter)->getCell()->isExterior());

                    if (x==(*iter)->getCell()->getGridX() &&
                        y==(*iter)->getCell()->getGridY())
                        break;

                    ++iter;
                }

                if (iter==mActiveCells.end())
                {
                    CellStore *cell = MWBase::Environment::get().getWorld()->getExterior(x, y);

                    loadCell (cell, loadingListener, changeEvent);
                }
            }
        }

        CellStore* current = MWBase::Environment::get().getWorld()->getExterior(X,Y);
        MWBase::Environment::get().getWindowManager()->changeCell(current);

        if (changeEvent)
            mCellChanged = true;
    }

    void Scene::changePlayerCell(CellStore *cell, const ESM::Position &pos, bool adjustPlayerPos)
    {
        mCurrentCell = cell;

        MWBase::World *world = MWBase::Environment::get().getWorld();
        MWWorld::Ptr old = world->getPlayerPtr();
        world->getPlayer().setCell(cell);

        MWWorld::Ptr player = world->getPlayerPtr();
        mRendering.updatePlayerPtr(player);

        if (adjustPlayerPos) {
            world->moveObject(player, pos.pos[0], pos.pos[1], pos.pos[2]);

            float x = pos.rot[0];
            float y = pos.rot[1];
            float z = pos.rot[2];
            world->rotateObject(player, x, y, z);

            if (adjustPlayerPos)
                player.getClass().adjustPosition(player, true);
        }

        MWBase::MechanicsManager *mechMgr =
            MWBase::Environment::get().getMechanicsManager();

        mechMgr->updateCell(old, player);
        mechMgr->watchActor(player);

        MWBase::Environment::get().getWorld()->adjustSky();
    }

    Scene::Scene (MWRender::RenderingManager& rendering, MWPhysics::PhysicsSystem *physics)
    : mCurrentCell (0), mCellChanged (false), mPhysics(physics), mRendering(rendering)
    , mPreloadTimer(0.f)
    , mHalfGridSize(Settings::Manager::getInt("exterior cell load distance", "Cells"))
    , mCellLoadingThreshold(1024.f)
    , mPreloadDistance(Settings::Manager::getInt("preload distance", "Cells"))
    , mPreloadEnabled(Settings::Manager::getBool("preload enabled", "Cells"))
    , mPreloadExteriorGrid(Settings::Manager::getBool("preload exterior grid", "Cells"))
    , mPreloadDoors(Settings::Manager::getBool("preload doors", "Cells"))
    , mPreloadFastTravel(Settings::Manager::getBool("preload fast travel", "Cells"))
    {
        mPreloader.reset(new CellPreloader(rendering.getResourceSystem(), physics->getShapeManager(), rendering.getTerrain()));
        mPreloader->setWorkQueue(mRendering.getWorkQueue());

        mPhysics->setUnrefQueue(rendering.getUnrefQueue());

        rendering.getResourceSystem()->setExpiryDelay(Settings::Manager::getFloat("cache expiry delay", "Cells"));

        mPreloader->setExpiryDelay(Settings::Manager::getFloat("preload cell expiry delay", "Cells"));
        mPreloader->setMinCacheSize(Settings::Manager::getInt("preload cell cache min", "Cells"));
        mPreloader->setMaxCacheSize(Settings::Manager::getInt("preload cell cache max", "Cells"));
        mPreloader->setPreloadInstances(Settings::Manager::getBool("preload instances", "Cells"));
    }

    Scene::~Scene()
    {
    }

    bool Scene::hasCellChanged() const
    {
        return mCellChanged;
    }

    const Scene::CellStoreCollection& Scene::getActiveCells() const
    {
        return mActiveCells;
    }

    void Scene::changeToInteriorCell (const std::string& cellName, const ESM::Position& position, bool adjustPlayerPos, bool changeEvent)
    {
        CellStore *cell = MWBase::Environment::get().getWorld()->getInterior(cellName);
        bool loadcell = (mCurrentCell == NULL);
        if(!loadcell)
            loadcell = *mCurrentCell != *cell;

        MWBase::Environment::get().getWindowManager()->fadeScreenOut(0.5);

        Loading::Listener* loadingListener = MWBase::Environment::get().getWindowManager()->getLoadingScreen();
        std::string loadingInteriorText = "#{sLoadingMessage2}";
        loadingListener->setLabel(loadingInteriorText);
        Loading::ScopedLoad load(loadingListener);

        //mRendering.enableTerrain(false);

        if(!loadcell)
        {
            MWBase::World *world = MWBase::Environment::get().getWorld();
            world->moveObject(world->getPlayerPtr(), position.pos[0], position.pos[1], position.pos[2]);

            float x = position.rot[0];
            float y = position.rot[1];
            float z = position.rot[2];
            world->rotateObject(world->getPlayerPtr(), x, y, z);

            if (adjustPlayerPos)
                world->getPlayerPtr().getClass().adjustPosition(world->getPlayerPtr(), true);
            MWBase::Environment::get().getWindowManager()->fadeScreenIn(0.5);
            return;
        }

        std::cout << "Changing to interior\n";

        // unload
        int current = 0;
        CellStoreCollection::iterator active = mActiveCells.begin();
        while (active!=mActiveCells.end())
        {
            unloadCell (active++);
            ++current;
        }

        int refsToLoad = cell->count();
        loadingListener->setProgressRange(refsToLoad);

        // Load cell.
        loadCell (cell, loadingListener, changeEvent);

        changePlayerCell(cell, position, adjustPlayerPos);

        // adjust fog
        mRendering.configureFog(mCurrentCell->getCell());

        // Sky system
        MWBase::Environment::get().getWorld()->adjustSky();

        if (changeEvent)
            mCellChanged = true;

        MWBase::Environment::get().getWindowManager()->fadeScreenIn(0.5);

        MWBase::Environment::get().getWindowManager()->changeCell(mCurrentCell);
    }

    void Scene::changeToExteriorCell (const ESM::Position& position, bool adjustPlayerPos, bool changeEvent)
    {
        int x = 0;
        int y = 0;

        MWBase::Environment::get().getWorld()->positionToIndex (position.pos[0], position.pos[1], x, y);

        changeCellGrid(x, y, changeEvent);

        CellStore* current = MWBase::Environment::get().getWorld()->getExterior(x, y);
        changePlayerCell(current, position, adjustPlayerPos);

        //mRendering.updateTerrain();
    }

    CellStore* Scene::getCurrentCell ()
    {
        return mCurrentCell;
    }

    void Scene::markCellAsUnchanged()
    {
        mCellChanged = false;
    }

    void Scene::insertCell (CellStore &cell, bool rescale, Loading::Listener* loadingListener)
    {
        InsertVisitor insertVisitor (cell, rescale, *loadingListener, *mPhysics, mRendering);
        cell.forEach (insertVisitor);
        insertVisitor.insert();

        // do adjustPosition (snapping actors to ground) after objects are loaded, so we don't depend on the loading order
        AdjustPositionVisitor adjustPosVisitor;
        cell.forEach (adjustPosVisitor);
    }

    void Scene::addObjectToScene (const Ptr& ptr)
    {
        try
        {
            addObject(ptr, *mPhysics, mRendering);
            updateObjectRotation(ptr, false);
            MWBase::Environment::get().getWorld()->scaleObject(ptr, ptr.getCellRef().getScale());
        }
        catch (std::exception& e)
        {
            std::cerr << "error during rendering '" << ptr.getCellRef().getRefId() << "': " << e.what() << std::endl;
        }
    }

    void Scene::removeObjectFromScene (const Ptr& ptr)
    {
        MWBase::Environment::get().getMechanicsManager()->remove (ptr);
        MWBase::Environment::get().getSoundManager()->stopSound3D (ptr);
        mPhysics->remove(ptr);
        mRendering.removeObject (ptr);
        if (ptr.getClass().isActor())
            mRendering.removeWaterRippleEmitter(ptr);
    }

    bool Scene::isCellActive(const CellStore &cell)
    {
        CellStoreCollection::iterator active = mActiveCells.begin();
        while (active != mActiveCells.end()) {
            if (**active == cell) {
                return true;
            }
            ++active;
        }
        return false;
    }

    Ptr Scene::searchPtrViaActorId (int actorId)
    {
        for (CellStoreCollection::const_iterator iter (mActiveCells.begin());
            iter!=mActiveCells.end(); ++iter)
            if (Ptr ptr = (*iter)->searchViaActorId (actorId))
                return ptr;

        return Ptr();
    }

    void Scene::preloadCells()
    {
        if (mPreloadDoors)
            preloadTeleportDoorDestinations();
        if (mPreloadExteriorGrid)
            preloadExteriorGrid();
        if (mPreloadFastTravel)
            preloadFastTravelDestinations();
    }

    void Scene::preloadTeleportDoorDestinations()
    {
        std::vector<MWWorld::ConstPtr> teleportDoors;
        for (CellStoreCollection::const_iterator iter (mActiveCells.begin());
            iter!=mActiveCells.end(); ++iter)
        {
            const MWWorld::CellStore* cellStore = *iter;
            typedef MWWorld::CellRefList<ESM::Door>::List DoorList;
            const DoorList &doors = cellStore->getReadOnlyDoors().mList;
            for (DoorList::const_iterator doorIt = doors.begin(); doorIt != doors.end(); ++doorIt)
            {
                if (!doorIt->mRef.getTeleport()) {
                    continue;
                }
                teleportDoors.push_back(MWWorld::ConstPtr(&*doorIt, cellStore));
            }
        }

        const MWWorld::ConstPtr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
        for (std::vector<MWWorld::ConstPtr>::iterator it = teleportDoors.begin(); it != teleportDoors.end(); ++it)
        {
            const MWWorld::ConstPtr& door = *it;
            float sqrDistToPlayer = (player.getRefData().getPosition().asVec3() - door.getRefData().getPosition().asVec3()).length2();

            if (sqrDistToPlayer < mPreloadDistance*mPreloadDistance)
            {
                try
                {
                    if (!door.getCellRef().getDestCell().empty())
                        preloadCell(MWBase::Environment::get().getWorld()->getInterior(door.getCellRef().getDestCell()));
                    else
                    {
                        int x,y;
                        MWBase::Environment::get().getWorld()->positionToIndex (door.getCellRef().getDoorDest().pos[0], door.getCellRef().getDoorDest().pos[1], x, y);
                        preloadCell(MWBase::Environment::get().getWorld()->getExterior(x,y), true);
                    }
                }
                catch (std::exception& e)
                {
                    // ignore error for now, would spam the log too much
                }
            }
        }
    }

    void Scene::preloadExteriorGrid()
    {
        if (!MWBase::Environment::get().getWorld()->isCellExterior())
            return;

        int halfGridSizePlusOne = mHalfGridSize + 1;

        const MWWorld::ConstPtr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
        osg::Vec3f playerPos = player.getRefData().getPosition().asVec3();

        int cellX,cellY;
        getGridCenter(cellX,cellY);

        float centerX, centerY;
        MWBase::Environment::get().getWorld()->indexToPosition(cellX, cellY, centerX, centerY, true);

        for (int dx = -halfGridSizePlusOne; dx <= halfGridSizePlusOne; ++dx)
        {
            for (int dy = -halfGridSizePlusOne; dy <= halfGridSizePlusOne; ++dy)
            {
                if (dy != halfGridSizePlusOne && dy != -halfGridSizePlusOne && dx != halfGridSizePlusOne && dx != -halfGridSizePlusOne)
                    continue; // only care about the outer (not yet loaded) part of the grid

                float thisCellCenterX, thisCellCenterY;
                MWBase::Environment::get().getWorld()->indexToPosition(cellX+dx, cellY+dy, thisCellCenterX, thisCellCenterY, true);

                float dist = std::max(std::abs(thisCellCenterX - playerPos.x()), std::abs(thisCellCenterY - playerPos.y()));
                float loadDist = 8192/2 + 8192 - mCellLoadingThreshold + mPreloadDistance;

                if (dist < loadDist)
                    preloadCell(MWBase::Environment::get().getWorld()->getExterior(cellX+dx, cellY+dy));
            }
        }
    }

    void Scene::preloadCell(CellStore *cell, bool preloadSurrounding)
    {
        if (preloadSurrounding && cell->isExterior())
        {
            int x = cell->getCell()->getGridX();
            int y = cell->getCell()->getGridY();
            unsigned int numpreloaded = 0;
            for (int dx = -mHalfGridSize; dx <= mHalfGridSize; ++dx)
            {
                for (int dy = -mHalfGridSize; dy <= mHalfGridSize; ++dy)
                {
                    mPreloader->preload(MWBase::Environment::get().getWorld()->getExterior(x+dx, y+dy), mRendering.getReferenceTime());
                    if (++numpreloaded >= mPreloader->getMaxCacheSize())
                        break;
                }
            }
        }
        else
            mPreloader->preload(cell, mRendering.getReferenceTime());
    }

    struct ListFastTravelDestinationsVisitor
    {
        ListFastTravelDestinationsVisitor(float preloadDist, const osg::Vec3f& playerPos)
            : mPreloadDist(preloadDist)
            , mPlayerPos(playerPos)
        {
        }

        bool operator()(const MWWorld::Ptr& ptr)
        {
            if ((ptr.getRefData().getPosition().asVec3() - mPlayerPos).length2() > mPreloadDist * mPreloadDist)
                return true;

            if (ptr.getClass().isNpc())
            {
                const std::vector<ESM::Transport::Dest>& transport = ptr.get<ESM::NPC>()->mBase->mTransport.mList;
                mList.insert(mList.begin(), transport.begin(), transport.end());
            }
            else
            {
                const std::vector<ESM::Transport::Dest>& transport = ptr.get<ESM::Creature>()->mBase->mTransport.mList;
                mList.insert(mList.begin(), transport.begin(), transport.end());
            }
            return true;
        }
        float mPreloadDist;
        osg::Vec3f mPlayerPos;
        std::vector<ESM::Transport::Dest> mList;
    };

    void Scene::preloadFastTravelDestinations()
    {
        const MWWorld::ConstPtr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
        ListFastTravelDestinationsVisitor listVisitor(mPreloadDistance, player.getRefData().getPosition().asVec3());

        for (CellStoreCollection::const_iterator iter (mActiveCells.begin()); iter!=mActiveCells.end(); ++iter)
        {
            MWWorld::CellStore* cellStore = *iter;
            cellStore->forEachType<ESM::NPC>(listVisitor);
            cellStore->forEachType<ESM::Creature>(listVisitor);
        }

        for (std::vector<ESM::Transport::Dest>::const_iterator it = listVisitor.mList.begin(); it != listVisitor.mList.end(); ++it)
        {
            if (!it->mCellName.empty())
                preloadCell(MWBase::Environment::get().getWorld()->getInterior(it->mCellName));
            else
            {
                int x,y;
                MWBase::Environment::get().getWorld()->positionToIndex( it->mPos.pos[0], it->mPos.pos[1], x, y);
                preloadCell(MWBase::Environment::get().getWorld()->getExterior(x,y), true);
            }
        }
    }
}
