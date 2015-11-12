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
#include "cellfunctors.hpp"
#include "cellstore.hpp"

namespace
{

    void addObject(const MWWorld::Ptr& ptr, MWPhysics::PhysicsSystem& physics,
                   MWRender::RenderingManager& rendering)
    {
        std::string model = Misc::ResourceHelpers::correctActorModelPath(ptr.getClass().getModel(ptr), rendering.getResourceSystem()->getVFS());
        std::string id = ptr.getClass().getId(ptr);
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
        }
    }

    struct InsertFunctor
    {
        MWWorld::CellStore& mCell;
        bool mRescale;
        Loading::Listener& mLoadingListener;
        MWPhysics::PhysicsSystem& mPhysics;
        MWRender::RenderingManager& mRendering;

        InsertFunctor (MWWorld::CellStore& cell, bool rescale, Loading::Listener& loadingListener,
            MWPhysics::PhysicsSystem& physics, MWRender::RenderingManager& rendering);

        bool operator() (const MWWorld::Ptr& ptr);
    };

    InsertFunctor::InsertFunctor (MWWorld::CellStore& cell, bool rescale,
        Loading::Listener& loadingListener, MWPhysics::PhysicsSystem& physics,
        MWRender::RenderingManager& rendering)
    : mCell (cell), mRescale (rescale), mLoadingListener (loadingListener),
      mPhysics (physics),
      mRendering (rendering)
    {}

    bool InsertFunctor::operator() (const MWWorld::Ptr& ptr)
    {
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
                updateObjectScale(ptr, mPhysics, mRendering);
                ptr.getClass().adjustPosition (ptr, false);
            }
            catch (const std::exception& e)
            {
                std::string error ("error during rendering '" + ptr.getCellRef().getRefId() + "': ");
                std::cerr << error + e.what() << std::endl;
            }
        }

        mLoadingListener.increaseProgress (1);

        return true;
    }
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
        if (mNeedMapUpdate)
        {
            // Note: exterior cell maps must be updated, even if they were visited before, because the set of surrounding cells might be different
            // (and objects in a different cell can "bleed" into another cells map if they cross the border)
            std::set<MWWorld::CellStore*> cellsToUpdate;
            for (CellStoreCollection::iterator active = mActiveCells.begin(); active!=mActiveCells.end(); ++active)
            {
                cellsToUpdate.insert(*active);
            }
            MWBase::Environment::get().getWindowManager()->requestMap(cellsToUpdate);

            mNeedMapUpdate = false;

            if (mCurrentCell->isExterior())
            {
                int cellX, cellY;
                getGridCenter(cellX, cellY);
                MWBase::Environment::get().getWindowManager()->setActiveMap(cellX,cellY,false);
            }
        }

        mRendering.update (duration, paused);
    }

    void Scene::unloadCell (CellStoreCollection::iterator iter)
    {
        std::cout << "Unloading cell\n";
        ListAndResetObjects functor;

        (*iter)->forEach<ListAndResetObjects>(functor);
        for (std::vector<MWWorld::Ptr>::const_iterator iter2 (functor.mObjects.begin());
            iter2!=functor.mObjects.end(); ++iter2)
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

    void Scene::loadCell (CellStore *cell, Loading::Listener* loadingListener)
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
            }

            cell->respawn();

            // ... then references. This is important for adjustPosition to work correctly.
            /// \todo rescale depending on the state of a new GMST
            insertCell (*cell, true, loadingListener);

            mRendering.addCell(cell);
            bool waterEnabled = cell->getCell()->hasWater() || cell->isExterior();
            float waterLevel = cell->isExterior() ? -1.f : cell->getWaterLevel();
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

        // register local scripts
        // ??? Should this go into the above if block ???
        MWBase::Environment::get().getWorld()->getLocalScripts().addCell (cell);
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
        const float maxDistance = 8192/2 + 1024; // 1/2 cell size + threshold
        float distance = std::max(std::abs(centerX-pos.x()), std::abs(centerY-pos.y()));
        if (distance > maxDistance)
        {
            int newX, newY;
            MWBase::Environment::get().getWorld()->positionToIndex(pos.x(), pos.y(), newX, newY);
            changeCellGrid(newX, newY);
            //mRendering.updateTerrain();
        }
    }

    void Scene::changeCellGrid (int X, int Y)
    {
        Loading::Listener* loadingListener = MWBase::Environment::get().getWindowManager()->getLoadingScreen();
        Loading::ScopedLoad load(loadingListener);

        //mRendering.enableTerrain(true);

        std::string loadingExteriorText = "#{sLoadingMessage3}";
        loadingListener->setLabel(loadingExteriorText);

        const int halfGridSize = Settings::Manager::getInt("exterior cell load distance", "Cells");

        CellStoreCollection::iterator active = mActiveCells.begin();
        while (active!=mActiveCells.end())
        {
            if ((*active)->getCell()->isExterior())
            {
                if (std::abs (X-(*active)->getCell()->getGridX())<=halfGridSize &&
                    std::abs (Y-(*active)->getCell()->getGridY())<=halfGridSize)
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
        for (int x=X-halfGridSize; x<=X+halfGridSize; ++x)
        {
            for (int y=Y-halfGridSize; y<=Y+halfGridSize; ++y)
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
        for (int x=X-halfGridSize; x<=X+halfGridSize; ++x)
        {
            for (int y=Y-halfGridSize; y<=Y+halfGridSize; ++y)
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

                    loadCell (cell, loadingListener);
                }
            }
        }

        CellStore* current = MWBase::Environment::get().getWorld()->getExterior(X,Y);
        MWBase::Environment::get().getWindowManager()->changeCell(current);

        mCellChanged = true;

        // Delay the map update until scripts have been given a chance to run.
        // If we don't do this, objects that should be disabled will still appear on the map.
        mNeedMapUpdate = true;
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

            player.getClass().adjustPosition(player, true);
        }

        MWBase::MechanicsManager *mechMgr =
            MWBase::Environment::get().getMechanicsManager();

        mechMgr->updateCell(old, player);
        mechMgr->watchActor(player);

        MWBase::Environment::get().getWorld()->adjustSky();
    }

    Scene::Scene (MWRender::RenderingManager& rendering, MWPhysics::PhysicsSystem *physics)
    : mCurrentCell (0), mCellChanged (false), mPhysics(physics), mRendering(rendering), mNeedMapUpdate(false)
    {
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

    void Scene::changeToInteriorCell (const std::string& cellName, const ESM::Position& position)
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
        loadCell (cell, loadingListener);

        changePlayerCell(cell, position, true);

        // adjust fog
        mRendering.configureFog(mCurrentCell->getCell());

        // Sky system
        MWBase::Environment::get().getWorld()->adjustSky();

        mCellChanged = true; MWBase::Environment::get().getWindowManager()->fadeScreenIn(0.5);

        MWBase::Environment::get().getWindowManager()->changeCell(mCurrentCell);

        // Delay the map update until scripts have been given a chance to run.
        // If we don't do this, objects that should be disabled will still appear on the map.
        mNeedMapUpdate = true;
    }

    void Scene::changeToExteriorCell (const ESM::Position& position, bool adjustPlayerPos)
    {
        int x = 0;
        int y = 0;

        MWBase::Environment::get().getWorld()->positionToIndex (position.pos[0], position.pos[1], x, y);

        changeCellGrid(x, y);

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
        InsertFunctor functor (cell, rescale, *loadingListener, *mPhysics, mRendering);
        cell.forEach (functor);
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
}
