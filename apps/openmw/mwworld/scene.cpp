#include "scene.hpp"

#include <limits>

#include <BulletCollision/CollisionDispatch/btCollisionObject.h>
#include <BulletCollision/CollisionShapes/btCompoundShape.h>

#include <components/debug/debuglog.hpp>
#include <components/loadinglistener/loadinglistener.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/settings/settings.hpp>
#include <components/resource/resourcesystem.hpp>
#include <components/resource/scenemanager.hpp>
#include <components/resource/bulletshape.hpp>
#include <components/sceneutil/unrefqueue.hpp>
#include <components/detournavigator/navigator.hpp>
#include <components/detournavigator/debug.hpp>
#include <components/misc/convert.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwmechanics/actorutil.hpp"

#include "../mwrender/renderingmanager.hpp"
#include "../mwrender/landmanager.hpp"

#include "../mwphysics/physicssystem.hpp"
#include "../mwphysics/actor.hpp"
#include "../mwphysics/object.hpp"
#include "../mwphysics/heightfield.hpp"

#include "player.hpp"
#include "localscripts.hpp"
#include "esmstore.hpp"
#include "class.hpp"
#include "cellvisitors.hpp"
#include "cellstore.hpp"
#include "cellpreloader.hpp"

namespace
{
    using MWWorld::RotationOrder;

    osg::Quat makeActorOsgQuat(const ESM::Position& position)
    {
        return osg::Quat(position.rot[2], osg::Vec3(0, 0, -1));
    }

    osg::Quat makeInversedOrderObjectOsgQuat(const ESM::Position& position)
    {
        const float xr = position.rot[0];
        const float yr = position.rot[1];
        const float zr = position.rot[2];

        return osg::Quat(xr, osg::Vec3(-1, 0, 0))
                * osg::Quat(yr, osg::Vec3(0, -1, 0))
                * osg::Quat(zr, osg::Vec3(0, 0, -1));
    }

    osg::Quat makeObjectOsgQuat(const ESM::Position& position)
    {
        const float xr = position.rot[0];
        const float yr = position.rot[1];
        const float zr = position.rot[2];

        return osg::Quat(zr, osg::Vec3(0, 0, -1))
            * osg::Quat(yr, osg::Vec3(0, -1, 0))
            * osg::Quat(xr, osg::Vec3(-1, 0, 0));
    }

    void setNodeRotation(const MWWorld::Ptr& ptr, MWRender::RenderingManager& rendering, RotationOrder order)
    {
        if (!ptr.getRefData().getBaseNode())
            return;

        rendering.rotateObject(ptr,
            ptr.getClass().isActor()
            ? makeActorOsgQuat(ptr.getRefData().getPosition())
            : (order == RotationOrder::inverse
                ? makeInversedOrderObjectOsgQuat(ptr.getRefData().getPosition())
                : makeObjectOsgQuat(ptr.getRefData().getPosition()))
        );
    }

    void addObject(const MWWorld::Ptr& ptr, MWPhysics::PhysicsSystem& physics,
                   MWRender::RenderingManager& rendering)
    {
        if (ptr.getRefData().getBaseNode() || physics.getActor(ptr))
        {
            Log(Debug::Warning) << "Warning: Tried to add " << ptr.getCellRef().getRefId() << " to the scene twice";
            return;
        }

        bool useAnim = ptr.getClass().useAnim();
        std::string model = ptr.getClass().getModel(ptr);
        if (useAnim)
            model = Misc::ResourceHelpers::correctActorModelPath(model, rendering.getResourceSystem()->getVFS());

        std::string id = ptr.getCellRef().getRefId();
        if (id == "prisonmarker" || id == "divinemarker" || id == "templemarker" || id == "northmarker")
            model = ""; // marker objects that have a hardcoded function in the game logic, should be hidden from the player

        ptr.getClass().insertObjectRendering(ptr, model, rendering);
        setNodeRotation(ptr, rendering, RotationOrder::direct);

        ptr.getClass().insertObject (ptr, model, physics);

        if (useAnim)
            MWBase::Environment::get().getMechanicsManager()->add(ptr);

        if (ptr.getClass().isActor())
            rendering.addWaterRippleEmitter(ptr);

        // Restore effect particles
        MWBase::Environment::get().getWorld()->applyLoopingParticles(ptr);
    }

    void addObject(const MWWorld::Ptr& ptr, const MWPhysics::PhysicsSystem& physics, DetourNavigator::Navigator& navigator)
    {
        if (const auto object = physics.getObject(ptr))
        {
            if (ptr.getClass().isDoor() && !ptr.getCellRef().getTeleport())
            {
                const auto shape = object->getShapeInstance()->getCollisionShape();

                btVector3 aabbMin;
                btVector3 aabbMax;
                shape->getAabb(btTransform::getIdentity(), aabbMin, aabbMax);

                const auto center = (aabbMax + aabbMin) * 0.5f;

                const auto distanceFromDoor = MWBase::Environment::get().getWorld()->getMaxActivationDistance() * 0.5f;
                const auto toPoint = aabbMax.x() - aabbMin.x() < aabbMax.y() - aabbMin.y()
                        ? btVector3(distanceFromDoor, 0, 0)
                        : btVector3(0, distanceFromDoor, 0);

                const auto& transform = object->getCollisionObject()->getWorldTransform();
                const btTransform closedDoorTransform(
                    Misc::Convert::toBullet(makeObjectOsgQuat(ptr.getCellRef().getPosition())),
                    transform.getOrigin()
                );

                const auto start = Misc::Convert::makeOsgVec3f(closedDoorTransform(center + toPoint));
                const auto startPoint = physics.castRay(start, start - osg::Vec3f(0, 0, 1000), ptr, {},
                    MWPhysics::CollisionType_World | MWPhysics::CollisionType_HeightMap | MWPhysics::CollisionType_Water);
                const auto connectionStart = startPoint.mHit ? startPoint.mHitPos : start;

                const auto end = Misc::Convert::makeOsgVec3f(closedDoorTransform(center - toPoint));
                const auto endPoint = physics.castRay(end, end - osg::Vec3f(0, 0, 1000), ptr, {},
                    MWPhysics::CollisionType_World | MWPhysics::CollisionType_HeightMap | MWPhysics::CollisionType_Water);
                const auto connectionEnd = endPoint.mHit ? endPoint.mHitPos : end;

                navigator.addObject(
                    DetourNavigator::ObjectId(object),
                    DetourNavigator::DoorShapes(
                        *shape,
                        object->getShapeInstance()->getAvoidCollisionShape(),
                        connectionStart,
                        connectionEnd
                    ),
                    transform
                );
            }
            else
            {
                navigator.addObject(
                    DetourNavigator::ObjectId(object),
                    DetourNavigator::ObjectShapes {
                        *object->getShapeInstance()->getCollisionShape(),
                        object->getShapeInstance()->getAvoidCollisionShape()
                    },
                    object->getCollisionObject()->getWorldTransform()
                );
            }
        }
        else if (physics.getActor(ptr))
        {
            navigator.addAgent(MWBase::Environment::get().getWorld()->getPathfindingHalfExtents(ptr));
        }
    }

    void updateObjectRotation (const MWWorld::Ptr& ptr, MWPhysics::PhysicsSystem& physics,
                                    MWRender::RenderingManager& rendering, RotationOrder order)
    {
        setNodeRotation(ptr, rendering, order);
        physics.updateRotation(ptr);
    }

    void updateObjectScale(const MWWorld::Ptr& ptr, MWPhysics::PhysicsSystem& physics,
                            MWRender::RenderingManager& rendering)
    {
        if (ptr.getRefData().getBaseNode() != nullptr)
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
        Loading::Listener& mLoadingListener;
        bool mTest;

        std::vector<MWWorld::Ptr> mToInsert;

        InsertVisitor (MWWorld::CellStore& cell, Loading::Listener& loadingListener, bool test);

        bool operator() (const MWWorld::Ptr& ptr);

        template <class AddObject>
        void insert(AddObject&& addObject);
    };

    InsertVisitor::InsertVisitor (MWWorld::CellStore& cell, Loading::Listener& loadingListener, bool test)
    : mCell (cell), mLoadingListener (loadingListener), mTest(test)
    {}

    bool InsertVisitor::operator() (const MWWorld::Ptr& ptr)
    {
        // do not insert directly as we can't modify the cell from within the visitation
        // CreatureLevList::insertObjectRendering may spawn a new creature
        mToInsert.push_back(ptr);
        return true;
    }

    template <class AddObject>
    void InsertVisitor::insert(AddObject&& addObject)
    {
        for (MWWorld::Ptr& ptr : mToInsert)
        {
            if (!ptr.getRefData().isDeleted() && ptr.getRefData().isEnabled())
            {
                try
                {
                    addObject(ptr);
                }
                catch (const std::exception& e)
                {
                    std::string error ("failed to render '" + ptr.getCellRef().getRefId() + "': ");
                    Log(Debug::Error) << error + e.what();
                }
            }

            if (!mTest)
                mLoadingListener.increaseProgress (1);
        }
    }

    struct PositionVisitor
    {
        bool operator() (const MWWorld::Ptr& ptr)
        {
            if (!ptr.getRefData().isDeleted() && ptr.getRefData().isEnabled())
                ptr.getClass().adjustPosition (ptr, false);
            return true;
        }
    };

    int getCellPositionDistanceToOrigin(const std::pair<int, int>& cellPosition)
    {
        return std::abs(cellPosition.first) + std::abs(cellPosition.second);
    }

}


namespace MWWorld
{

    void Scene::updateObjectRotation(const Ptr& ptr, RotationOrder order)
    {
        ::updateObjectRotation(ptr, *mPhysics, mRendering, order);
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
        mPreloadTimer += duration;
        if (mPreloadTimer > 0.1f)
        {
            preloadCells(0.1f);
            mPreloadTimer = 0.f;
        }

        mRendering.update (duration, paused);

        mPreloader->updateCache(mRendering.getReferenceTime());
    }

    void Scene::unloadCell (CellStoreCollection::iterator iter, bool test)
    {
        if (!test)
            Log(Debug::Info) << "Unloading cell " << (*iter)->getCell()->getDescription();

        const auto navigator = MWBase::Environment::get().getWorld()->getNavigator();
        ListAndResetObjectsVisitor visitor;

        (*iter)->forEach(visitor);
        const auto world = MWBase::Environment::get().getWorld();
        for (const auto& ptr : visitor.mObjects)
        {
            if (const auto object = mPhysics->getObject(ptr))
                navigator->removeObject(DetourNavigator::ObjectId(object));
            else if (mPhysics->getActor(ptr))
            {
                navigator->removeAgent(world->getPathfindingHalfExtents(ptr));
                mRendering.removeActorPath(ptr);
            }
            mPhysics->remove(ptr);
        }

        const auto cellX = (*iter)->getCell()->getGridX();
        const auto cellY = (*iter)->getCell()->getGridY();

        if ((*iter)->getCell()->isExterior())
        {
            const ESM::Land* land =
                MWBase::Environment::get().getWorld()->getStore().get<ESM::Land>().search(
                    (*iter)->getCell()->getGridX(),
                    (*iter)->getCell()->getGridY()
                );
            if (land && land->mDataTypes&ESM::Land::DATA_VHGT)
            {
                if (const auto heightField = mPhysics->getHeightField(cellX, cellY))
                    navigator->removeObject(DetourNavigator::ObjectId(heightField));
                mPhysics->removeHeightField(cellX, cellY);
            }
        }

        if ((*iter)->getCell()->hasWater())
            navigator->removeWater(osg::Vec2i(cellX, cellY));

        const auto player = world->getPlayerPtr();
        navigator->update(player.getRefData().getPosition().asVec3());

        MWBase::Environment::get().getMechanicsManager()->drop (*iter);

        mRendering.removeCell(*iter);
        MWBase::Environment::get().getWindowManager()->removeCell(*iter);

        MWBase::Environment::get().getWorld()->getLocalScripts().clearCell (*iter);

        MWBase::Environment::get().getSoundManager()->stopSound (*iter);
        mActiveCells.erase(*iter);
    }

    void Scene::loadCell (CellStore *cell, Loading::Listener* loadingListener, bool respawn, bool test)
    {
        std::pair<CellStoreCollection::iterator, bool> result = mActiveCells.insert(cell);

        if(result.second)
        {
            if (test)
                Log(Debug::Info) << "Testing cell " << cell->getCell()->getDescription();
            else
                Log(Debug::Info) << "Loading cell " << cell->getCell()->getDescription();

            float verts = ESM::Land::LAND_SIZE;
            float worldsize = ESM::Land::REAL_SIZE;

            const auto navigator = MWBase::Environment::get().getWorld()->getNavigator();

            const int cellX = cell->getCell()->getGridX();
            const int cellY = cell->getCell()->getGridY();

            // Load terrain physics first...
            if (!test && cell->getCell()->isExterior())
            {
                osg::ref_ptr<const ESMTerrain::LandObject> land = mRendering.getLandManager()->getLand(cellX, cellY);
                const ESM::Land::LandData* data = land ? land->getData(ESM::Land::DATA_VHGT) : 0;
                if (data)
                {
                    mPhysics->addHeightField (data->mHeights, cellX, cellY, worldsize / (verts-1), verts, data->mMinHeight, data->mMaxHeight, land.get());
                }
                else
                {
                    static std::vector<float> defaultHeight;
                    defaultHeight.resize(verts*verts, ESM::Land::DEFAULT_HEIGHT);
                    mPhysics->addHeightField (&defaultHeight[0], cell->getCell()->getGridX(), cell->getCell()->getGridY(), worldsize / (verts-1), verts, ESM::Land::DEFAULT_HEIGHT, ESM::Land::DEFAULT_HEIGHT, land.get());
                }

                if (const auto heightField = mPhysics->getHeightField(cellX, cellY))
                    navigator->addObject(DetourNavigator::ObjectId(heightField), *heightField->getShape(),
                            heightField->getCollisionObject()->getWorldTransform());
            }

            // register local scripts
            // do this before insertCell, to make sure we don't add scripts from levelled creature spawning twice
            MWBase::Environment::get().getWorld()->getLocalScripts().addCell (cell);

            if (respawn)
                cell->respawn();

            // ... then references. This is important for adjustPosition to work correctly.
            insertCell (*cell, loadingListener, test);

            mRendering.addCell(cell);
            if (!test)
            {
                MWBase::Environment::get().getWindowManager()->addCell(cell);
                bool waterEnabled = cell->getCell()->hasWater() || cell->isExterior();
                float waterLevel = cell->getWaterLevel();
                mRendering.setWaterEnabled(waterEnabled);
                if (waterEnabled)
                {
                    mPhysics->enableWater(waterLevel);
                    mRendering.setWaterHeight(waterLevel);

                    if (cell->getCell()->isExterior())
                    {
                        if (const auto heightField = mPhysics->getHeightField(cellX, cellY))
                            navigator->addWater(osg::Vec2i(cellX, cellY), ESM::Land::REAL_SIZE,
                                cell->getWaterLevel(), heightField->getCollisionObject()->getWorldTransform());
                    }
                    else
                    {
                        navigator->addWater(osg::Vec2i(cellX, cellY), std::numeric_limits<int>::max(),
                            cell->getWaterLevel(), btTransform::getIdentity());
                    }
                }
                else
                    mPhysics->disableWater();

                const auto player = MWBase::Environment::get().getWorld()->getPlayerPtr();
                navigator->update(player.getRefData().getPosition().asVec3());

                if (!cell->isExterior() && !(cell->getCell()->mData.mFlags & ESM::Cell::QuasiEx))
                {

                    mRendering.configureAmbient(cell->getCell());
                }
            }
        }

        mPreloader->notifyLoaded(cell);
    }

    void Scene::clear()
    {
        CellStoreCollection::iterator active = mActiveCells.begin();
        while (active!=mActiveCells.end())
            unloadCell (active++);
        assert(mActiveCells.empty());
        mCurrentCell = nullptr;

        mPreloader->clear();
    }

    void Scene::playerMoved(const osg::Vec3f &pos)
    {
        const auto navigator = MWBase::Environment::get().getWorld()->getNavigator();
        const auto player = MWBase::Environment::get().getWorld()->getPlayerPtr();
        navigator->update(player.getRefData().getPosition().asVec3());

        if (!mCurrentCell || !mCurrentCell->isExterior())
            return;

        // figure out the center of the current cell grid (*not* necessarily mCurrentCell, which is the cell the player is in)
        int cellX, cellY;
        getGridCenter(cellX, cellY);
        float centerX, centerY;
        MWBase::Environment::get().getWorld()->indexToPosition(cellX, cellY, centerX, centerY, true);
        const float maxDistance = Constants::CellSizeInUnits / 2 + mCellLoadingThreshold; // 1/2 cell size + threshold
        float distance = std::max(std::abs(centerX-pos.x()), std::abs(centerY-pos.y()));
        if (distance > maxDistance)
        {
            int newX, newY;
            MWBase::Environment::get().getWorld()->positionToIndex(pos.x(), pos.y(), newX, newY);
            changeCellGrid(newX, newY);
        }
    }

    void Scene::changeCellGrid (int playerCellX, int playerCellY, bool changeEvent)
    {
        Loading::Listener* loadingListener = MWBase::Environment::get().getWindowManager()->getLoadingScreen();
        Loading::ScopedLoad load(loadingListener);

        int messagesCount = MWBase::Environment::get().getWindowManager()->getMessagesCount();
        std::string loadingExteriorText = "#{sLoadingMessage3}";
        loadingListener->setLabel(loadingExteriorText, false, messagesCount > 0);

        CellStoreCollection::iterator active = mActiveCells.begin();
        while (active!=mActiveCells.end())
        {
            if ((*active)->getCell()->isExterior())
            {
                if (std::abs (playerCellX-(*active)->getCell()->getGridX())<=mHalfGridSize &&
                    std::abs (playerCellY-(*active)->getCell()->getGridY())<=mHalfGridSize)
                {
                    // keep cells within the new grid
                    ++active;
                    continue;
                }
            }
            unloadCell (active++);
        }

        std::size_t refsToLoad = 0;
        std::vector<std::pair<int, int>> cellsPositionsToLoad;
        // get the number of refs to load
        for (int x = playerCellX - mHalfGridSize; x <= playerCellX + mHalfGridSize; ++x)
        {
            for (int y = playerCellY - mHalfGridSize; y <= playerCellY + mHalfGridSize; ++y)
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
                    refsToLoad += MWBase::Environment::get().getWorld()->getExterior(x, y)->count();
                    cellsPositionsToLoad.push_back(std::make_pair(x, y));
                }
            }
        }

        loadingListener->setProgressRange(refsToLoad);

        const auto getDistanceToPlayerCell = [&] (const std::pair<int, int>& cellPosition)
        {
            return std::abs(cellPosition.first - playerCellX) + std::abs(cellPosition.second - playerCellY);
        };

        const auto getCellPositionPriority = [&] (const std::pair<int, int>& cellPosition)
        {
            return std::make_pair(getDistanceToPlayerCell(cellPosition), getCellPositionDistanceToOrigin(cellPosition));
        };

        std::sort(cellsPositionsToLoad.begin(), cellsPositionsToLoad.end(),
            [&] (const std::pair<int, int>& lhs, const std::pair<int, int>& rhs) {
                return getCellPositionPriority(lhs) < getCellPositionPriority(rhs);
            });

        // Load cells
        for (const auto& cellPosition : cellsPositionsToLoad)
        {
            const auto x = cellPosition.first;
            const auto y = cellPosition.second;

            CellStoreCollection::iterator iter = mActiveCells.begin();

            while (iter != mActiveCells.end())
            {
                assert ((*iter)->getCell()->isExterior());

                if (x == (*iter)->getCell()->getGridX() &&
                    y == (*iter)->getCell()->getGridY())
                    break;

                ++iter;
            }

            if (iter == mActiveCells.end())
            {
                CellStore *cell = MWBase::Environment::get().getWorld()->getExterior(x, y);

                loadCell (cell, loadingListener, changeEvent);
            }
        }

        CellStore* current = MWBase::Environment::get().getWorld()->getExterior(playerCellX, playerCellY);
        MWBase::Environment::get().getWindowManager()->changeCell(current);

        if (changeEvent)
            mCellChanged = true;
    }

    void Scene::testExteriorCells()
    {
        // Note: temporary disable ICO to decrease memory usage
        mRendering.getResourceSystem()->getSceneManager()->setIncrementalCompileOperation(nullptr);

        mRendering.getResourceSystem()->setExpiryDelay(1.f);

        const MWWorld::Store<ESM::Cell> &cells = MWBase::Environment::get().getWorld()->getStore().get<ESM::Cell>();

        Loading::Listener* loadingListener = MWBase::Environment::get().getWindowManager()->getLoadingScreen();
        Loading::ScopedLoad load(loadingListener);
        loadingListener->setProgressRange(cells.getExtSize());

        MWWorld::Store<ESM::Cell>::iterator it = cells.extBegin();
        int i = 1;
        for (; it != cells.extEnd(); ++it)
        {
            loadingListener->setLabel("Testing exterior cells ("+std::to_string(i)+"/"+std::to_string(cells.getExtSize())+")...");

            CellStoreCollection::iterator iter = mActiveCells.begin();

            CellStore *cell = MWBase::Environment::get().getWorld()->getExterior(it->mData.mX, it->mData.mY);
            loadCell (cell, loadingListener, false, true);

            iter = mActiveCells.begin();
            while (iter != mActiveCells.end())
            {
                if (it->isExterior() && it->mData.mX == (*iter)->getCell()->getGridX() &&
                    it->mData.mY == (*iter)->getCell()->getGridY())
                {
                    unloadCell(iter, true);
                    break;
                }

                ++iter;
            }

            mRendering.getResourceSystem()->updateCache(mRendering.getReferenceTime());
            mRendering.getUnrefQueue()->flush(mRendering.getWorkQueue());

            loadingListener->increaseProgress (1);
            i++;
        }

        mRendering.getResourceSystem()->getSceneManager()->setIncrementalCompileOperation(mRendering.getIncrementalCompileOperation());
        mRendering.getResourceSystem()->setExpiryDelay(Settings::Manager::getFloat("cache expiry delay", "Cells"));
    }

    void Scene::testInteriorCells()
    {
        // Note: temporary disable ICO to decrease memory usage
        mRendering.getResourceSystem()->getSceneManager()->setIncrementalCompileOperation(nullptr);

        mRendering.getResourceSystem()->setExpiryDelay(1.f);

        const MWWorld::Store<ESM::Cell> &cells = MWBase::Environment::get().getWorld()->getStore().get<ESM::Cell>();

        Loading::Listener* loadingListener = MWBase::Environment::get().getWindowManager()->getLoadingScreen();
        Loading::ScopedLoad load(loadingListener);
        loadingListener->setProgressRange(cells.getIntSize());

        int i = 1;
        MWWorld::Store<ESM::Cell>::iterator it = cells.intBegin();
        for (; it != cells.intEnd(); ++it)
        {
            loadingListener->setLabel("Testing interior cells ("+std::to_string(i)+"/"+std::to_string(cells.getIntSize())+")...");

            CellStore *cell = MWBase::Environment::get().getWorld()->getInterior(it->mName);
            loadCell (cell, loadingListener, false, true);

            CellStoreCollection::iterator iter = mActiveCells.begin();
            while (iter != mActiveCells.end())
            {
                assert (!(*iter)->getCell()->isExterior());

                if (it->mName == (*iter)->getCell()->mName)
                {
                    unloadCell(iter, true);
                    break;
                }

                ++iter;
            }

            mRendering.getResourceSystem()->updateCache(mRendering.getReferenceTime());
            mRendering.getUnrefQueue()->flush(mRendering.getWorkQueue());

            loadingListener->increaseProgress (1);
            i++;
        }

        mRendering.getResourceSystem()->getSceneManager()->setIncrementalCompileOperation(mRendering.getIncrementalCompileOperation());
        mRendering.getResourceSystem()->setExpiryDelay(Settings::Manager::getFloat("cache expiry delay", "Cells"));
    }

    void Scene::changePlayerCell(CellStore *cell, const ESM::Position &pos, bool adjustPlayerPos)
    {
        mCurrentCell = cell;

        mRendering.enableTerrain(cell->isExterior());

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

        mPhysics->updatePtr(old, player);

        MWBase::Environment::get().getWorld()->adjustSky();

        mLastPlayerPos = pos.asVec3();
    }

    Scene::Scene (MWRender::RenderingManager& rendering, MWPhysics::PhysicsSystem *physics,
                  DetourNavigator::Navigator& navigator)
    : mCurrentCell (0), mCellChanged (false), mPhysics(physics), mRendering(rendering), mNavigator(navigator)
    , mPreloadTimer(0.f)
    , mHalfGridSize(Settings::Manager::getInt("exterior cell load distance", "Cells"))
    , mCellLoadingThreshold(1024.f)
    , mPreloadDistance(Settings::Manager::getInt("preload distance", "Cells"))
    , mPreloadEnabled(Settings::Manager::getBool("preload enabled", "Cells"))
    , mPreloadExteriorGrid(Settings::Manager::getBool("preload exterior grid", "Cells"))
    , mPreloadDoors(Settings::Manager::getBool("preload doors", "Cells"))
    , mPreloadFastTravel(Settings::Manager::getBool("preload fast travel", "Cells"))
    , mPredictionTime(Settings::Manager::getFloat("prediction time", "Cells"))
    {
        mPreloader.reset(new CellPreloader(rendering.getResourceSystem(), physics->getShapeManager(), rendering.getTerrain(), rendering.getLandManager()));
        mPreloader->setWorkQueue(mRendering.getWorkQueue());

        mPreloader->setUnrefQueue(rendering.getUnrefQueue());
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
        bool useFading = (mCurrentCell != nullptr);
        if (useFading)
            MWBase::Environment::get().getWindowManager()->fadeScreenOut(0.5);

        Loading::Listener* loadingListener = MWBase::Environment::get().getWindowManager()->getLoadingScreen();
        int messagesCount = MWBase::Environment::get().getWindowManager()->getMessagesCount();
        std::string loadingInteriorText = "#{sLoadingMessage2}";
        loadingListener->setLabel(loadingInteriorText, false, messagesCount > 0);
        Loading::ScopedLoad load(loadingListener);

        if(mCurrentCell != nullptr && *mCurrentCell == *cell)
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

        Log(Debug::Info) << "Changing to interior";

        // unload
        CellStoreCollection::iterator active = mActiveCells.begin();
        while (active!=mActiveCells.end())
            unloadCell (active++);

        loadingListener->setProgressRange(cell->count());

        // Load cell.
        loadCell (cell, loadingListener, changeEvent);

        changePlayerCell(cell, position, adjustPlayerPos);

        // adjust fog
        mRendering.configureFog(mCurrentCell->getCell());

        // Sky system
        MWBase::Environment::get().getWorld()->adjustSky();

        if (changeEvent)
            mCellChanged = true;

        if (useFading)
            MWBase::Environment::get().getWindowManager()->fadeScreenIn(0.5);

        MWBase::Environment::get().getWindowManager()->changeCell(mCurrentCell);
    }

    void Scene::changeToExteriorCell (const ESM::Position& position, bool adjustPlayerPos, bool changeEvent)
    {
        int x = 0;
        int y = 0;

        MWBase::Environment::get().getWorld()->positionToIndex (position.pos[0], position.pos[1], x, y);

        if (changeEvent)
            MWBase::Environment::get().getWindowManager()->fadeScreenOut(0.5);

        changeCellGrid(x, y, changeEvent);

        CellStore* current = MWBase::Environment::get().getWorld()->getExterior(x, y);
        changePlayerCell(current, position, adjustPlayerPos);

        if (changeEvent)
            MWBase::Environment::get().getWindowManager()->fadeScreenIn(0.5);
    }

    CellStore* Scene::getCurrentCell ()
    {
        return mCurrentCell;
    }

    void Scene::markCellAsUnchanged()
    {
        mCellChanged = false;
    }

    void Scene::insertCell (CellStore &cell, Loading::Listener* loadingListener, bool test)
    {
        InsertVisitor insertVisitor (cell, *loadingListener, test);
        cell.forEach (insertVisitor);
        insertVisitor.insert([&] (const MWWorld::Ptr& ptr) { addObject(ptr, *mPhysics, mRendering); });
        insertVisitor.insert([&] (const MWWorld::Ptr& ptr) { addObject(ptr, *mPhysics, mNavigator); });

        // do adjustPosition (snapping actors to ground) after objects are loaded, so we don't depend on the loading order
        PositionVisitor posVisitor;
        cell.forEach (posVisitor);
    }

    void Scene::addObjectToScene (const Ptr& ptr)
    {
        try
        {
            addObject(ptr, *mPhysics, mRendering);
            addObject(ptr, *mPhysics, mNavigator);
            MWBase::Environment::get().getWorld()->scaleObject(ptr, ptr.getCellRef().getScale());
            const auto navigator = MWBase::Environment::get().getWorld()->getNavigator();
            const auto player = MWBase::Environment::get().getWorld()->getPlayerPtr();
            navigator->update(player.getRefData().getPosition().asVec3());
        }
        catch (std::exception& e)
        {
            Log(Debug::Error) << "failed to render '" << ptr.getCellRef().getRefId() << "': " << e.what();
        }
    }

    void Scene::removeObjectFromScene (const Ptr& ptr)
    {
        MWBase::Environment::get().getMechanicsManager()->remove (ptr);
        MWBase::Environment::get().getSoundManager()->stopSound3D (ptr);
        const auto navigator = MWBase::Environment::get().getWorld()->getNavigator();
        if (const auto object = mPhysics->getObject(ptr))
        {
            navigator->removeObject(DetourNavigator::ObjectId(object));
            const auto player = MWBase::Environment::get().getWorld()->getPlayerPtr();
            navigator->update(player.getRefData().getPosition().asVec3());
        }
        else if (mPhysics->getActor(ptr))
        {
            navigator->removeAgent(MWBase::Environment::get().getWorld()->getPathfindingHalfExtents(ptr));
        }
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

    class PreloadMeshItem : public SceneUtil::WorkItem
    {
    public:
        PreloadMeshItem(const std::string& mesh, Resource::SceneManager* sceneManager)
            : mMesh(mesh), mSceneManager(sceneManager)
        {
        }

        virtual void doWork()
        {
            try
            {
                mSceneManager->getTemplate(mMesh);
            }
            catch (std::exception& e)
            {
            }
        }
    private:
        std::string mMesh;
        Resource::SceneManager* mSceneManager;
    };

    void Scene::preload(const std::string &mesh, bool useAnim)
    {
        std::string mesh_ = mesh;
        if (useAnim)
            mesh_ = Misc::ResourceHelpers::correctActorModelPath(mesh_, mRendering.getResourceSystem()->getVFS());

        if (!mRendering.getResourceSystem()->getSceneManager()->checkLoaded(mesh_, mRendering.getReferenceTime()))
            mRendering.getWorkQueue()->addWorkItem(new PreloadMeshItem(mesh_, mRendering.getResourceSystem()->getSceneManager()));
    }

    void Scene::preloadCells(float dt)
    {
        std::vector<osg::Vec3f> exteriorPositions;

        const MWWorld::ConstPtr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
        osg::Vec3f playerPos = player.getRefData().getPosition().asVec3();
        osg::Vec3f moved = playerPos - mLastPlayerPos;
        osg::Vec3f predictedPos = playerPos + moved / dt * mPredictionTime;

        if (mCurrentCell->isExterior())
            exteriorPositions.push_back(predictedPos);

        mLastPlayerPos = playerPos;

        if (mPreloadEnabled)
        {
            if (mPreloadDoors)
                preloadTeleportDoorDestinations(playerPos, predictedPos, exteriorPositions);
            if (mPreloadExteriorGrid)
                preloadExteriorGrid(playerPos, predictedPos);
            if (mPreloadFastTravel)
                preloadFastTravelDestinations(playerPos, predictedPos, exteriorPositions);
        }

        mPreloader->setTerrainPreloadPositions(exteriorPositions);
    }

    void Scene::preloadTeleportDoorDestinations(const osg::Vec3f& playerPos, const osg::Vec3f& predictedPos, std::vector<osg::Vec3f>& exteriorPositions)
    {
        std::vector<MWWorld::ConstPtr> teleportDoors;
        for (const MWWorld::CellStore* cellStore : mActiveCells)
        {
            typedef MWWorld::CellRefList<ESM::Door>::List DoorList;
            const DoorList &doors = cellStore->getReadOnlyDoors().mList;
            for (auto& door : doors)
            {
                if (!door.mRef.getTeleport())
                {
                    continue;
                }
                teleportDoors.push_back(MWWorld::ConstPtr(&door, cellStore));
            }
        }

        for (const MWWorld::ConstPtr& door : teleportDoors)
        {
            float sqrDistToPlayer = (playerPos - door.getRefData().getPosition().asVec3()).length2();
            sqrDistToPlayer = std::min(sqrDistToPlayer, (predictedPos - door.getRefData().getPosition().asVec3()).length2());

            if (sqrDistToPlayer < mPreloadDistance*mPreloadDistance)
            {
                try
                {
                    if (!door.getCellRef().getDestCell().empty())
                        preloadCell(MWBase::Environment::get().getWorld()->getInterior(door.getCellRef().getDestCell()));
                    else
                    {
                        osg::Vec3f pos = door.getCellRef().getDoorDest().asVec3();
                        int x,y;
                        MWBase::Environment::get().getWorld()->positionToIndex (pos.x(), pos.y(), x, y);
                        preloadCell(MWBase::Environment::get().getWorld()->getExterior(x,y), true);
                        exteriorPositions.push_back(pos);
                    }
                }
                catch (std::exception& e)
                {
                    // ignore error for now, would spam the log too much
                }
            }
        }
    }

    void Scene::preloadExteriorGrid(const osg::Vec3f& playerPos, const osg::Vec3f& predictedPos)
    {
        if (!MWBase::Environment::get().getWorld()->isCellExterior())
            return;

        int halfGridSizePlusOne = mHalfGridSize + 1;


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
                dist = std::min(dist,std::max(std::abs(thisCellCenterX - predictedPos.x()), std::abs(thisCellCenterY - predictedPos.y())));
                float loadDist = Constants::CellSizeInUnits / 2 + Constants::CellSizeInUnits - mCellLoadingThreshold + mPreloadDistance;

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

    void Scene::preloadTerrain(const osg::Vec3f &pos)
    {
        std::vector<osg::Vec3f> vec;
        vec.push_back(pos);
        mPreloader->setTerrainPreloadPositions(vec);
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

    void Scene::preloadFastTravelDestinations(const osg::Vec3f& playerPos, const osg::Vec3f& /*predictedPos*/, std::vector<osg::Vec3f>& exteriorPositions) // ignore predictedPos here since opening dialogue with travel service takes extra time
    {
        const MWWorld::ConstPtr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
        ListFastTravelDestinationsVisitor listVisitor(mPreloadDistance, player.getRefData().getPosition().asVec3());

        for (MWWorld::CellStore* cellStore : mActiveCells)
        {
            cellStore->forEachType<ESM::NPC>(listVisitor);
            cellStore->forEachType<ESM::Creature>(listVisitor);
        }

        for (ESM::Transport::Dest& dest : listVisitor.mList)
        {
            if (!dest.mCellName.empty())
                preloadCell(MWBase::Environment::get().getWorld()->getInterior(dest.mCellName));
            else
            {
                osg::Vec3f pos = dest.mPos.asVec3();
                int x,y;
                MWBase::Environment::get().getWorld()->positionToIndex( pos.x(), pos.y(), x, y);
                preloadCell(MWBase::Environment::get().getWorld()->getExterior(x,y), true);
                exteriorPositions.push_back(pos);
            }
        }
    }
}
