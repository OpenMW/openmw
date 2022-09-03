#include "scene.hpp"

#include <limits>
#include <chrono>
#include <atomic>

#include <BulletCollision/CollisionDispatch/btCollisionObject.h>

#include <components/debug/debuglog.hpp>
#include <components/loadinglistener/loadinglistener.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/settings/settings.hpp>
#include <components/resource/resourcesystem.hpp>
#include <components/resource/scenemanager.hpp>
#include <components/sceneutil/positionattitudetransform.hpp>
#include <components/detournavigator/navigator.hpp>
#include <components/detournavigator/agentbounds.hpp>
#include <components/misc/convert.hpp>
#include <components/detournavigator/heightfieldshape.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/luamanager.hpp"

#include "../mwrender/renderingmanager.hpp"
#include "../mwrender/landmanager.hpp"
#include "../mwrender/postprocessor.hpp"

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
#include "worldimp.hpp"
#include "cellutils.hpp"

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

    osg::Quat makeInverseNodeRotation(const MWWorld::Ptr& ptr)
    {
        const auto pos = ptr.getRefData().getPosition();
        return ptr.getClass().isActor() ? makeActorOsgQuat(pos) : makeInversedOrderObjectOsgQuat(pos);
    }

    osg::Quat makeDirectNodeRotation(const MWWorld::Ptr& ptr)
    {
        const auto pos = ptr.getRefData().getPosition();
        return ptr.getClass().isActor() ? makeActorOsgQuat(pos) : Misc::Convert::makeOsgQuat(pos);
    }

    osg::Quat makeNodeRotation(const MWWorld::Ptr& ptr, RotationOrder order)
    {
        if (order == RotationOrder::inverse)
            return makeInverseNodeRotation(ptr);
        return makeDirectNodeRotation(ptr);
    }

    void setNodeRotation(const MWWorld::Ptr& ptr, MWRender::RenderingManager& rendering, const osg::Quat &rotation)
    {
        if (ptr.getRefData().getBaseNode())
            rendering.rotateObject(ptr, rotation);
    }

    std::string getModel(const MWWorld::Ptr &ptr, const VFS::Manager *vfs)
    {
        if (Misc::ResourceHelpers::isHiddenMarker(ptr.getCellRef().getRefId()))
            return {};
        bool useAnim = ptr.getClass().useAnim();
        std::string model = ptr.getClass().getModel(ptr);
        if (useAnim)
            model = Misc::ResourceHelpers::correctActorModelPath(model, vfs);
        return model;
    }

    void addObject(const MWWorld::Ptr& ptr, const MWWorld::World& world, const std::vector<ESM::RefNum>& pagedRefs,
        MWPhysics::PhysicsSystem& physics, MWRender::RenderingManager& rendering)
    {
        if (ptr.getRefData().getBaseNode() || physics.getActor(ptr))
        {
            Log(Debug::Warning) << "Warning: Tried to add " << ptr.getCellRef().getRefId() << " to the scene twice";
            return;
        }

        std::string model = getModel(ptr, rendering.getResourceSystem()->getVFS());
        const auto rotation = makeDirectNodeRotation(ptr);

        const ESM::RefNum& refnum = ptr.getCellRef().getRefNum();
        if (!refnum.hasContentFile() || !std::binary_search(pagedRefs.begin(), pagedRefs.end(), refnum))
            ptr.getClass().insertObjectRendering(ptr, model, rendering);
        else
            ptr.getRefData().setBaseNode(new SceneUtil::PositionAttitudeTransform); // FIXME remove this when physics code is fixed not to depend on basenode
        setNodeRotation(ptr, rendering, rotation);

        if (ptr.getClass().useAnim())
            MWBase::Environment::get().getMechanicsManager()->add(ptr);

        if (ptr.getClass().isActor())
            rendering.addWaterRippleEmitter(ptr);

        // Restore effect particles
        world.applyLoopingParticles(ptr);

        if (!model.empty())
            ptr.getClass().insertObject(ptr, model, rotation, physics);

        MWBase::Environment::get().getLuaManager()->objectAddedToScene(ptr);
    }

    void addObject(const MWWorld::Ptr& ptr, const MWWorld::World& world, const MWPhysics::PhysicsSystem& physics,
                   DetourNavigator::Navigator& navigator)
    {
        if (const auto object = physics.getObject(ptr))
        {
            const DetourNavigator::ObjectTransform objectTransform {ptr.getRefData().getPosition(), ptr.getCellRef().getScale()};

            if (ptr.getClass().isDoor() && !ptr.getCellRef().getTeleport())
            {
                btVector3 aabbMin;
                btVector3 aabbMax;
                object->getShapeInstance()->mCollisionShape->getAabb(btTransform::getIdentity(), aabbMin, aabbMax);

                const auto center = (aabbMax + aabbMin) * 0.5f;

                const auto distanceFromDoor = world.getMaxActivationDistance() * 0.5f;
                const auto toPoint = aabbMax.x() - aabbMin.x() < aabbMax.y() - aabbMin.y()
                        ? btVector3(distanceFromDoor, 0, 0)
                        : btVector3(0, distanceFromDoor, 0);

                const auto transform = object->getTransform();
                const btTransform closedDoorTransform(
                    Misc::Convert::makeBulletQuaternion(ptr.getCellRef().getPosition()),
                    transform.getOrigin()
                );

                const auto start = Misc::Convert::toOsg(closedDoorTransform(center + toPoint));
                const auto startPoint = physics.castRay(start, start - osg::Vec3f(0, 0, 1000), ptr, {},
                    MWPhysics::CollisionType_World | MWPhysics::CollisionType_HeightMap | MWPhysics::CollisionType_Water);
                const auto connectionStart = startPoint.mHit ? startPoint.mHitPos : start;

                const auto end = Misc::Convert::toOsg(closedDoorTransform(center - toPoint));
                const auto endPoint = physics.castRay(end, end - osg::Vec3f(0, 0, 1000), ptr, {},
                    MWPhysics::CollisionType_World | MWPhysics::CollisionType_HeightMap | MWPhysics::CollisionType_Water);
                const auto connectionEnd = endPoint.mHit ? endPoint.mHitPos : end;

                navigator.addObject(
                    DetourNavigator::ObjectId(object),
                    DetourNavigator::DoorShapes(object->getShapeInstance(), objectTransform, connectionStart, connectionEnd),
                    transform
                );
            }
            else if (object->getShapeInstance()->mVisualCollisionType == Resource::VisualCollisionType::None)
            {
                navigator.addObject(
                    DetourNavigator::ObjectId(object),
                    DetourNavigator::ObjectShapes(object->getShapeInstance(), objectTransform),
                    object->getTransform()
                );
            }
        }
        else if (physics.getActor(ptr))
        {
            navigator.addAgent(world.getPathfindingAgentBounds(ptr));
        }
    }

    struct InsertVisitor
    {
        MWWorld::CellStore& mCell;
        Loading::Listener* mLoadingListener;

        std::vector<MWWorld::Ptr> mToInsert;

        InsertVisitor (MWWorld::CellStore& cell, Loading::Listener* loadingListener);

        bool operator() (const MWWorld::Ptr& ptr);

        template <class AddObject>
        void insert(AddObject&& addObject);
    };

    InsertVisitor::InsertVisitor (MWWorld::CellStore& cell, Loading::Listener* loadingListener)
        : mCell(cell), mLoadingListener(loadingListener)
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
                    Log(Debug::Error) << "failed to render '" << ptr.getCellRef().getRefId() << "': " << e.what();
                }
            }

            if (mLoadingListener != nullptr)
                mLoadingListener->increaseProgress(1);
        }
    }

    int getCellPositionDistanceToOrigin(const std::pair<int, int>& cellPosition)
    {
        return std::abs(cellPosition.first) + std::abs(cellPosition.second);
    }

    bool isCellInCollection(int x, int y, MWWorld::Scene::CellStoreCollection& collection)
    {
        for (auto *cell : collection)
        {
            assert(cell->getCell()->isExterior());
            if (x == cell->getCell()->getGridX() && y == cell->getCell()->getGridY())
                return true;
        }
        return false;
    }

    bool removeFromSorted(const ESM::RefNum& refNum, std::vector<ESM::RefNum>& pagedRefs)
    {
        const auto it = std::lower_bound(pagedRefs.begin(), pagedRefs.end(), refNum);
        if (it == pagedRefs.end() || *it != refNum)
            return false;
        pagedRefs.erase(it);
        return true;
    }
}


namespace MWWorld
{

    void Scene::removeFromPagedRefs(const Ptr &ptr)
    {
        const ESM::RefNum& refnum = ptr.getCellRef().getRefNum();
        if (refnum.hasContentFile() && removeFromSorted(refnum, mPagedRefs))
        {
            if (!ptr.getRefData().getBaseNode()) return;
            ptr.getClass().insertObjectRendering(ptr, getModel(ptr, mRendering.getResourceSystem()->getVFS()), mRendering);
            setNodeRotation(ptr, mRendering, makeNodeRotation(ptr, RotationOrder::direct));
            reloadTerrain();
        }
    }

    void Scene::updateObjectPosition(const Ptr &ptr, const osg::Vec3f &pos, bool movePhysics)
    {
        mRendering.moveObject(ptr, pos);
        if (movePhysics)
        {
            mPhysics->updatePosition(ptr);
        }
    }

    void Scene::updateObjectRotation(const Ptr &ptr, RotationOrder order)
    {
        const auto rot = makeNodeRotation(ptr, order);
        setNodeRotation(ptr, mRendering, rot);
        mPhysics->updateRotation(ptr, rot);
    }

    void Scene::updateObjectScale(const Ptr &ptr)
    {
        float scale = ptr.getCellRef().getScale();
        osg::Vec3f scaleVec (scale, scale, scale);
        ptr.getClass().adjustScale(ptr, scaleVec, true);
        mRendering.scaleObject(ptr, scaleVec);
        mPhysics->updateScale(ptr);
    }

    void Scene::update(float duration)
    {
        if (mChangeCellGridRequest.has_value())
        {
            changeCellGrid(mChangeCellGridRequest->mPosition, mChangeCellGridRequest->mCell.x(),
                           mChangeCellGridRequest->mCell.y(), mChangeCellGridRequest->mChangeEvent);
            mChangeCellGridRequest.reset();
        }

        mPreloader->updateCache(mRendering.getReferenceTime());
        preloadCells(duration);
    }

    void Scene::unloadCell(CellStore* cell)
    {
        if (mActiveCells.find(cell) == mActiveCells.end())
            return;

        Log(Debug::Info) << "Unloading cell " << cell->getCell()->getDescription();

        ListAndResetObjectsVisitor visitor;

        cell->forEach(visitor);
        for (const auto& ptr : visitor.mObjects)
        {
            if (const auto object = mPhysics->getObject(ptr))
            {
                if (object->getShapeInstance()->mVisualCollisionType == Resource::VisualCollisionType::None)
                    mNavigator.removeObject(DetourNavigator::ObjectId(object));
                mPhysics->remove(ptr);
                ptr.mRef->mData.mPhysicsPostponed = false;
            }
            else if (mPhysics->getActor(ptr))
            {
                mNavigator.removeAgent(mWorld.getPathfindingAgentBounds(ptr));
                mRendering.removeActorPath(ptr);
                mPhysics->remove(ptr);
            }
            MWBase::Environment::get().getLuaManager()->objectRemovedFromScene(ptr);
        }

        const auto cellX = cell->getCell()->getGridX();
        const auto cellY = cell->getCell()->getGridY();

        if (cell->getCell()->isExterior())
        {
            if (mPhysics->getHeightField(cellX, cellY) != nullptr)
                mNavigator.removeHeightfield(osg::Vec2i(cellX, cellY));

            mPhysics->removeHeightField(cellX, cellY);
        }

        if (cell->getCell()->hasWater())
            mNavigator.removeWater(osg::Vec2i(cellX, cellY));

        if (const auto pathgrid = mWorld.getStore().get<ESM::Pathgrid>().search(*cell->getCell()))
            mNavigator.removePathgrid(*pathgrid);

        MWBase::Environment::get().getMechanicsManager()->drop (cell);

        mRendering.removeCell(cell);
        MWBase::Environment::get().getWindowManager()->removeCell(cell);

        mWorld.getLocalScripts().clearCell (cell);

        MWBase::Environment::get().getSoundManager()->stopSound (cell);
        mActiveCells.erase(cell);
        // Clean up any effects that may have been spawned while unloading all cells
        if(mActiveCells.empty())
            mRendering.notifyWorldSpaceChanged();
    }

    void Scene::loadCell(CellStore *cell, Loading::Listener* loadingListener, bool respawn, const osg::Vec3f& position)
    {
        using DetourNavigator::HeightfieldShape;

        assert(mActiveCells.find(cell) == mActiveCells.end());
        mActiveCells.insert(cell);

        Log(Debug::Info) << "Loading cell " << cell->getCell()->getDescription();

        const int cellX = cell->getCell()->getGridX();
        const int cellY = cell->getCell()->getGridY();

        mNavigator.setWorldspace(cell->getCell()->mCellId.mWorldspace);

        if (cell->getCell()->isExterior())
        {
            osg::ref_ptr<const ESMTerrain::LandObject> land = mRendering.getLandManager()->getLand(cellX, cellY);
            const ESM::Land::LandData* data = land ? land->getData(ESM::Land::DATA_VHGT) : nullptr;
            const int verts = ESM::Land::LAND_SIZE;
            const int worldsize = ESM::Land::REAL_SIZE;
            if (data)
            {
                mPhysics->addHeightField(data->mHeights, cellX, cellY, worldsize, verts, data->mMinHeight, data->mMaxHeight, land.get());
            }
            else
            {
                static std::vector<float> defaultHeight;
                defaultHeight.resize(verts*verts, ESM::Land::DEFAULT_HEIGHT);
                mPhysics->addHeightField(defaultHeight.data(), cellX, cellY, worldsize, verts, ESM::Land::DEFAULT_HEIGHT, ESM::Land::DEFAULT_HEIGHT, land.get());
            }
            if (const auto heightField = mPhysics->getHeightField(cellX, cellY))
            {
                const osg::Vec2i cellPosition(cellX, cellY);
                const btVector3& origin = heightField->getCollisionObject()->getWorldTransform().getOrigin();
                const osg::Vec3f shift(origin.x(), origin.y(), origin.z());
                const HeightfieldShape shape = [&] () -> HeightfieldShape
                {
                    if (data == nullptr)
                    {
                        return DetourNavigator::HeightfieldPlane {static_cast<float>(ESM::Land::DEFAULT_HEIGHT)};
                    }
                    else
                    {
                        DetourNavigator::HeightfieldSurface heights;
                        heights.mHeights = data->mHeights;
                        heights.mSize = static_cast<std::size_t>(ESM::Land::LAND_SIZE);
                        heights.mMinHeight = data->mMinHeight;
                        heights.mMaxHeight = data->mMaxHeight;
                        return heights;
                    }
                } ();
                mNavigator.addHeightfield(cellPosition, ESM::Land::REAL_SIZE, shape);
            }
        }

        if (const auto pathgrid = mWorld.getStore().get<ESM::Pathgrid>().search(*cell->getCell()))
            mNavigator.addPathgrid(*cell->getCell(), *pathgrid);

        // register local scripts
        // do this before insertCell, to make sure we don't add scripts from levelled creature spawning twice
        mWorld.getLocalScripts().addCell (cell);

        if (respawn)
            cell->respawn();

        insertCell(*cell, loadingListener);

        mRendering.addCell(cell);

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
                    mNavigator.addWater(osg::Vec2i(cellX, cellY), ESM::Land::REAL_SIZE, waterLevel);
            }
            else
            {
                mNavigator.addWater(osg::Vec2i(cellX, cellY), std::numeric_limits<int>::max(), waterLevel);
            }
        }
        else
            mPhysics->disableWater();

        mNavigator.update(position);

        if (!cell->isExterior() && !(cell->getCell()->mData.mFlags & ESM::Cell::QuasiEx))
            mRendering.configureAmbient(cell->getCell());

        mPreloader->notifyLoaded(cell);
    }

    void Scene::clear()
    {
        for (auto iter = mActiveCells.begin(); iter!=mActiveCells.end(); )
        {
            auto* cell = *iter++;
            unloadCell (cell);
        }
        assert(mActiveCells.empty());
        mCurrentCell = nullptr;

        mPreloader->clear();
    }

    osg::Vec4i Scene::gridCenterToBounds(const osg::Vec2i& centerCell) const
    {
        return osg::Vec4i(centerCell.x()-mHalfGridSize,centerCell.y()-mHalfGridSize,centerCell.x()+mHalfGridSize+1,centerCell.y()+mHalfGridSize+1);
    }

    osg::Vec2i Scene::getNewGridCenter(const osg::Vec3f &pos, const osg::Vec2i* currentGridCenter) const
    {
        if (currentGridCenter)
        {
            float centerX, centerY;
            mWorld.indexToPosition(currentGridCenter->x(), currentGridCenter->y(), centerX, centerY, true);
            float distance = std::max(std::abs(centerX-pos.x()), std::abs(centerY-pos.y()));
            const float maxDistance = Constants::CellSizeInUnits / 2 + mCellLoadingThreshold; // 1/2 cell size + threshold
            if (distance <= maxDistance)
                return *currentGridCenter;
        }
        return positionToCellIndex(pos.x(), pos.y());
    }

    void Scene::playerMoved(const osg::Vec3f &pos)
    {
        if (!mCurrentCell || !mCurrentCell->isExterior())
            return;

        osg::Vec2i newCell = getNewGridCenter(pos, &mCurrentGridCenter);
        if (newCell != mCurrentGridCenter)
            requestChangeCellGrid(pos, newCell);
    }

    void Scene::requestChangeCellGrid(const osg::Vec3f &position, const osg::Vec2i& cell, bool changeEvent)
    {
        mChangeCellGridRequest = ChangeCellGridRequest {position, cell, changeEvent};
    }

    void Scene::changeCellGrid (const osg::Vec3f &pos, int playerCellX, int playerCellY, bool changeEvent)
    {
        for (auto iter = mActiveCells.begin(); iter != mActiveCells.end(); )
        {
            auto* cell = *iter++;
            if (cell->getCell()->isExterior())
            {
                const auto dx = std::abs(playerCellX - cell->getCell()->getGridX());
                const auto dy = std::abs(playerCellY - cell->getCell()->getGridY());
                if (dx > mHalfGridSize || dy > mHalfGridSize)
                    unloadCell(cell);
            }
            else
                unloadCell (cell);
        }

        mNavigator.updateBounds(pos);

        mCurrentGridCenter = osg::Vec2i(playerCellX, playerCellY);
        osg::Vec4i newGrid = gridCenterToBounds(mCurrentGridCenter);
        mRendering.setActiveGrid(newGrid);

        if (mRendering.pagingUnlockCache())
            mPreloader->abortTerrainPreloadExcept(nullptr);
        if (!mPreloader->isTerrainLoaded(std::make_pair(pos, newGrid), mRendering.getReferenceTime()))
            preloadTerrain(pos, true);
        mPagedRefs.clear();
        mRendering.getPagedRefnums(newGrid, mPagedRefs);

        std::size_t refsToLoad = 0;
        const auto cellsToLoad = [&] (CellStoreCollection& collection, int range) -> std::vector<std::pair<int,int>>
        {
            std::vector<std::pair<int, int>> cellsPositionsToLoad;
            for (int x = playerCellX - range; x <= playerCellX + range; ++x)
            {
                for (int y = playerCellY - range; y <= playerCellY + range; ++y)
                {
                    if (!isCellInCollection(x, y, collection))
                    {
                        refsToLoad += mWorld.getExterior(x, y)->count();
                        cellsPositionsToLoad.emplace_back(x, y);
                    }
                }
            }
            return cellsPositionsToLoad;
        };

        addPostponedPhysicsObjects();

        auto cellsPositionsToLoad = cellsToLoad(mActiveCells,mHalfGridSize);

        Loading::Listener* loadingListener = MWBase::Environment::get().getWindowManager()->getLoadingScreen();
        Loading::ScopedLoad load(loadingListener);
        std::string loadingExteriorText = "#{sLoadingMessage3}";
        loadingListener->setLabel(loadingExteriorText);
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

        for (const auto& [x,y] : cellsPositionsToLoad)
        {
            if (!isCellInCollection(x, y, mActiveCells))
            {
                CellStore *cell = mWorld.getExterior(x, y);
                loadCell(cell, loadingListener, changeEvent, pos);
            }
        }

        CellStore* current = mWorld.getExterior(playerCellX, playerCellY);
        MWBase::Environment::get().getWindowManager()->changeCell(current);

        if (changeEvent)
            mCellChanged = true;

        mCellLoaded = true;
    }

    void Scene::addPostponedPhysicsObjects()
    {
        for(const auto& cell : mActiveCells)
        {
            cell->forEach([&](const MWWorld::Ptr& ptr)
            {
                if(ptr.mRef->mData.mPhysicsPostponed)
                {
                    ptr.mRef->mData.mPhysicsPostponed = false;
                    if (ptr.mRef->mData.isEnabled() && ptr.mRef->mData.getCount() > 0)
                    {
                        std::string model = getModel(ptr, MWBase::Environment::get().getResourceSystem()->getVFS());
                        if (!model.empty())
                        {
                            const auto rotation = makeNodeRotation(ptr, RotationOrder::direct);
                            ptr.getClass().insertObjectPhysics(ptr, model, rotation, *mPhysics);
                        }
                    }
                }
                return true;
            });
        }
    }

    void Scene::testExteriorCells()
    {
        // Note: temporary disable ICO to decrease memory usage
        mRendering.getResourceSystem()->getSceneManager()->setIncrementalCompileOperation(nullptr);

        mRendering.getResourceSystem()->setExpiryDelay(1.f);

        const MWWorld::Store<ESM::Cell> &cells = mWorld.getStore().get<ESM::Cell>();

        Loading::Listener* loadingListener = MWBase::Environment::get().getWindowManager()->getLoadingScreen();
        Loading::ScopedLoad load(loadingListener);
        loadingListener->setProgressRange(cells.getExtSize());

        MWWorld::Store<ESM::Cell>::iterator it = cells.extBegin();
        int i = 1;
        for (; it != cells.extEnd(); ++it)
        {
            loadingListener->setLabel("Testing exterior cells ("+std::to_string(i)+"/"+std::to_string(cells.getExtSize())+")...");

            CellStore *cell = mWorld.getExterior(it->mData.mX, it->mData.mY);
            loadCell(cell, nullptr, false, osg::Vec3f(it->mData.mX + 0.5f, it->mData.mY + 0.5f, 0) * Constants::CellSizeInUnits);

            auto iter = mActiveCells.begin();
            while (iter != mActiveCells.end())
            {
                if (it->isExterior() && it->mData.mX == (*iter)->getCell()->getGridX() &&
                    it->mData.mY == (*iter)->getCell()->getGridY())
                {
                    unloadCell(*iter);
                    break;
                }

                ++iter;
            }

            mRendering.getResourceSystem()->updateCache(mRendering.getReferenceTime());

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

        const MWWorld::Store<ESM::Cell> &cells = mWorld.getStore().get<ESM::Cell>();

        Loading::Listener* loadingListener = MWBase::Environment::get().getWindowManager()->getLoadingScreen();
        Loading::ScopedLoad load(loadingListener);
        loadingListener->setProgressRange(cells.getIntSize());

        int i = 1;
        MWWorld::Store<ESM::Cell>::iterator it = cells.intBegin();
        for (; it != cells.intEnd(); ++it)
        {
            loadingListener->setLabel("Testing interior cells ("+std::to_string(i)+"/"+std::to_string(cells.getIntSize())+")...");

            CellStore *cell = mWorld.getInterior(it->mName);
            ESM::Position position;
            mWorld.findInteriorPosition(it->mName, position);
            loadCell(cell, nullptr, false, position.asVec3());

            auto iter = mActiveCells.begin();
            while (iter != mActiveCells.end())
            {
                assert (!(*iter)->getCell()->isExterior());

                if (it->mName == (*iter)->getCell()->mName)
                {
                    unloadCell(*iter);
                    break;
                }

                ++iter;
            }

            mRendering.getResourceSystem()->updateCache(mRendering.getReferenceTime());

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

        MWWorld::Ptr old = mWorld.getPlayerPtr();
        mWorld.getPlayer().setCell(cell);

        MWWorld::Ptr player = mWorld.getPlayerPtr();
        mRendering.updatePlayerPtr(player);

        // The player is loaded before the scene and by default it is grounded, with the scene fully loaded,
        // we validate and correct this. Only run once, during initial cell load.
        if (old.mCell == cell)
            mPhysics->traceDown(player, player.getRefData().getPosition().asVec3(), 10.f);

        if (adjustPlayerPos)
        {
            mWorld.moveObject(player, pos.asVec3());
            mWorld.rotateObject(player, pos.asRotationVec3());

            player.getClass().adjustPosition(player, true);
        }

        MWBase::Environment::get().getMechanicsManager()->updateCell(old, player);
        MWBase::Environment::get().getWindowManager()->watchActor(player);

        mPhysics->updatePtr(old, player);

        mWorld.adjustSky();

        mLastPlayerPos = player.getRefData().getPosition().asVec3();
    }

    Scene::Scene(MWWorld::World& world, MWRender::RenderingManager& rendering, MWPhysics::PhysicsSystem *physics,
                  DetourNavigator::Navigator& navigator)
    : mCurrentCell (nullptr), mCellChanged (false)
    , mWorld(world), mPhysics(physics), mRendering(rendering), mNavigator(navigator)
    , mCellLoadingThreshold(1024.f)
    , mPreloadDistance(Settings::Manager::getInt("preload distance", "Cells"))
    , mPreloadEnabled(Settings::Manager::getBool("preload enabled", "Cells"))
    , mPreloadExteriorGrid(Settings::Manager::getBool("preload exterior grid", "Cells"))
    , mPreloadDoors(Settings::Manager::getBool("preload doors", "Cells"))
    , mPreloadFastTravel(Settings::Manager::getBool("preload fast travel", "Cells"))
    , mPredictionTime(Settings::Manager::getFloat("prediction time", "Cells"))
    {
        mPreloader = std::make_unique<CellPreloader>(rendering.getResourceSystem(), physics->getShapeManager(), rendering.getTerrain(), rendering.getLandManager());
        mPreloader->setWorkQueue(mRendering.getWorkQueue());

        rendering.getResourceSystem()->setExpiryDelay(Settings::Manager::getFloat("cache expiry delay", "Cells"));

        mPreloader->setExpiryDelay(Settings::Manager::getFloat("preload cell expiry delay", "Cells"));
        mPreloader->setMinCacheSize(Settings::Manager::getInt("preload cell cache min", "Cells"));
        mPreloader->setMaxCacheSize(Settings::Manager::getInt("preload cell cache max", "Cells"));
        mPreloader->setPreloadInstances(Settings::Manager::getBool("preload instances", "Cells"));
    }

    Scene::~Scene()
    {
        for (const osg::ref_ptr<SceneUtil::WorkItem>& v : mWorkItems)
            v->abort();

        for (const osg::ref_ptr<SceneUtil::WorkItem>& v : mWorkItems)
            v->waitTillDone();
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
        CellStore *cell = mWorld.getInterior(cellName);
        bool useFading = (mCurrentCell != nullptr);
        if (useFading)
            MWBase::Environment::get().getWindowManager()->fadeScreenOut(0.5);

        Loading::Listener* loadingListener = MWBase::Environment::get().getWindowManager()->getLoadingScreen();
        std::string loadingInteriorText = "#{sLoadingMessage2}";
        loadingListener->setLabel(loadingInteriorText);
        Loading::ScopedLoad load(loadingListener);

        if(mCurrentCell != nullptr && *mCurrentCell == *cell)
        {
            mWorld.moveObject(mWorld.getPlayerPtr(), position.asVec3());
            mWorld.rotateObject(mWorld.getPlayerPtr(), position.asRotationVec3());

            if (adjustPlayerPos)
                mWorld.getPlayerPtr().getClass().adjustPosition(mWorld.getPlayerPtr(), true);
            MWBase::Environment::get().getWindowManager()->fadeScreenIn(0.5);
            return;
        }

        Log(Debug::Info) << "Changing to interior";

        // unload
        for (auto iter = mActiveCells.begin(); iter!=mActiveCells.end(); )
        {
            auto* cellToUnload = *iter++;
            unloadCell(cellToUnload);
        }
        assert(mActiveCells.empty());

        loadingListener->setProgressRange(cell->count());

        mNavigator.updateBounds(position.asVec3());

        // Load cell.
        mPagedRefs.clear();
        loadCell(cell, loadingListener, changeEvent, position.asVec3());

        changePlayerCell(cell, position, adjustPlayerPos);

        // adjust fog
        mRendering.configureFog(mCurrentCell->getCell());

        // Sky system
        mWorld.adjustSky();

        if (changeEvent)
            mCellChanged = true;

        mCellLoaded = true;

        if (useFading)
            MWBase::Environment::get().getWindowManager()->fadeScreenIn(0.5);

        MWBase::Environment::get().getWindowManager()->changeCell(mCurrentCell);

        MWBase::Environment::get().getWorld()->getPostProcessor()->setExteriorFlag(cell->getCell()->mData.mFlags & ESM::Cell::QuasiEx);
    }

    void Scene::changeToExteriorCell (const ESM::Position& position, bool adjustPlayerPos, bool changeEvent)
    {
        const osg::Vec2i cellIndex = positionToCellIndex(position.pos[0], position.pos[1]);

        if (changeEvent)
            MWBase::Environment::get().getWindowManager()->fadeScreenOut(0.5);

        changeCellGrid(position.asVec3(), cellIndex.x(), cellIndex.y(), changeEvent);

        CellStore* current = mWorld.getExterior(cellIndex.x(), cellIndex.y());
        changePlayerCell(current, position, adjustPlayerPos);

        if (changeEvent)
            MWBase::Environment::get().getWindowManager()->fadeScreenIn(0.5);

        MWBase::Environment::get().getWorld()->getPostProcessor()->setExteriorFlag(true);
    }

    CellStore* Scene::getCurrentCell ()
    {
        return mCurrentCell;
    }

    void Scene::markCellAsUnchanged()
    {
        mCellChanged = false;
    }

    void Scene::insertCell (CellStore &cell, Loading::Listener* loadingListener)
    {
        InsertVisitor insertVisitor(cell, loadingListener);
        cell.forEach (insertVisitor);
        insertVisitor.insert([&] (const MWWorld::Ptr& ptr) { addObject(ptr, mWorld, mPagedRefs, *mPhysics, mRendering); });
        insertVisitor.insert([&] (const MWWorld::Ptr& ptr) { addObject(ptr, mWorld, *mPhysics, mNavigator); });
    }

    void Scene::addObjectToScene (const Ptr& ptr)
    {
        try
        {
            addObject(ptr, mWorld, mPagedRefs, *mPhysics, mRendering);
            addObject(ptr, mWorld, *mPhysics, mNavigator);
            mWorld.scaleObject(ptr, ptr.getCellRef().getScale());
        }
        catch (std::exception& e)
        {
            Log(Debug::Error) << "failed to render '" << ptr.getCellRef().getRefId() << "': " << e.what();
        }
    }

    void Scene::removeObjectFromScene (const Ptr& ptr, bool keepActive)
    {
        MWBase::Environment::get().getMechanicsManager()->remove (ptr, keepActive);
        // You'd expect the sounds attached to the object to be stopped here
        // because the object is nowhere to be heard, but in Morrowind, they're not.
        // They're still stopped when the cell is unloaded
        // or if the player moves away far from the object's position.
        // Todd Howard, Who art in Bethesda, hallowed be Thy name.
        MWBase::Environment::get().getLuaManager()->objectRemovedFromScene(ptr);
        if (const auto object = mPhysics->getObject(ptr))
        {
            if (object->getShapeInstance()->mVisualCollisionType == Resource::VisualCollisionType::None)
                mNavigator.removeObject(DetourNavigator::ObjectId(object));
        }
        else if (mPhysics->getActor(ptr))
        {
            mNavigator.removeAgent(mWorld.getPathfindingAgentBounds(ptr));
        }
        mPhysics->remove(ptr);
        mRendering.removeObject (ptr);
        if (ptr.getClass().isActor())
            mRendering.removeWaterRippleEmitter(ptr);
        ptr.getRefData().setBaseNode(nullptr);
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

        void doWork() override
        {
            if (mAborted)
                return;

            try
            {
                mSceneManager->getTemplate(mMesh);
            }
            catch (std::exception&)
            {
            }
        }

        void abort() override
        {
            mAborted = true;
        }

    private:
        std::string mMesh;
        Resource::SceneManager* mSceneManager;
        std::atomic_bool mAborted {false};
    };

    void Scene::preload(const std::string &mesh, bool useAnim)
    {
        std::string mesh_ = mesh;
        if (useAnim)
            mesh_ = Misc::ResourceHelpers::correctActorModelPath(mesh_, mRendering.getResourceSystem()->getVFS());

        if (!mRendering.getResourceSystem()->getSceneManager()->checkLoaded(mesh_, mRendering.getReferenceTime()))
        {
            osg::ref_ptr<PreloadMeshItem> item(new PreloadMeshItem(mesh_, mRendering.getResourceSystem()->getSceneManager()));
            mRendering.getWorkQueue()->addWorkItem(item);
            const auto isDone = [] (const osg::ref_ptr<SceneUtil::WorkItem>& v) { return v->isDone(); };
            mWorkItems.erase(std::remove_if(mWorkItems.begin(), mWorkItems.end(), isDone), mWorkItems.end());
            mWorkItems.emplace_back(std::move(item));
        }
    }

    void Scene::preloadCells(float dt)
    {
        if (dt<=1e-06) return;
        std::vector<PositionCellGrid> exteriorPositions;

        const MWWorld::ConstPtr player = mWorld.getPlayerPtr();
        osg::Vec3f playerPos = player.getRefData().getPosition().asVec3();
        osg::Vec3f moved = playerPos - mLastPlayerPos;
        osg::Vec3f predictedPos = playerPos + moved / dt * mPredictionTime;

        if (mCurrentCell->isExterior())
            exteriorPositions.emplace_back(predictedPos, gridCenterToBounds(getNewGridCenter(predictedPos, &mCurrentGridCenter)));

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

    void Scene::preloadTeleportDoorDestinations(const osg::Vec3f& playerPos, const osg::Vec3f& predictedPos, std::vector<PositionCellGrid>& exteriorPositions)
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
                teleportDoors.emplace_back(&door, cellStore);
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
                        preloadCell(mWorld.getInterior(door.getCellRef().getDestCell()));
                    else
                    {
                        osg::Vec3f pos = door.getCellRef().getDoorDest().asVec3();
                        const osg::Vec2i cellIndex = positionToCellIndex(pos.x(), pos.y());
                        preloadCell(mWorld.getExterior(cellIndex.x(), cellIndex.y()), true);
                        exteriorPositions.emplace_back(pos, gridCenterToBounds(getNewGridCenter(pos)));
                    }
                }
                catch (std::exception&)
                {
                    // ignore error for now, would spam the log too much
                }
            }
        }
    }

    void Scene::preloadExteriorGrid(const osg::Vec3f& playerPos, const osg::Vec3f& predictedPos)
    {
        if (!mWorld.isCellExterior())
            return;

        int halfGridSizePlusOne = mHalfGridSize + 1;


        int cellX,cellY;
        cellX = mCurrentGridCenter.x(); cellY = mCurrentGridCenter.y();

        float centerX, centerY;
        mWorld.indexToPosition(cellX, cellY, centerX, centerY, true);

        for (int dx = -halfGridSizePlusOne; dx <= halfGridSizePlusOne; ++dx)
        {
            for (int dy = -halfGridSizePlusOne; dy <= halfGridSizePlusOne; ++dy)
            {
                if (dy != halfGridSizePlusOne && dy != -halfGridSizePlusOne && dx != halfGridSizePlusOne && dx != -halfGridSizePlusOne)
                    continue; // only care about the outer (not yet loaded) part of the grid

                float thisCellCenterX, thisCellCenterY;
                mWorld.indexToPosition(cellX+dx, cellY+dy, thisCellCenterX, thisCellCenterY, true);

                float dist = std::max(std::abs(thisCellCenterX - playerPos.x()), std::abs(thisCellCenterY - playerPos.y()));
                dist = std::min(dist,std::max(std::abs(thisCellCenterX - predictedPos.x()), std::abs(thisCellCenterY - predictedPos.y())));
                float loadDist = Constants::CellSizeInUnits / 2 + Constants::CellSizeInUnits - mCellLoadingThreshold + mPreloadDistance;

                if (dist < loadDist)
                    preloadCell(mWorld.getExterior(cellX+dx, cellY+dy));
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
                    mPreloader->preload(mWorld.getExterior(x+dx, y+dy), mRendering.getReferenceTime());
                    if (++numpreloaded >= mPreloader->getMaxCacheSize())
                        break;
                }
            }
        }
        else
            mPreloader->preload(cell, mRendering.getReferenceTime());
    }

    void Scene::preloadTerrain(const osg::Vec3f &pos, bool sync)
    {
        std::vector<PositionCellGrid> vec;
        vec.emplace_back(pos, gridCenterToBounds(getNewGridCenter(pos)));
        mPreloader->abortTerrainPreloadExcept(vec.data());
        mPreloader->setTerrainPreloadPositions(vec);
        if (!sync) return;

        Loading::Listener* loadingListener = MWBase::Environment::get().getWindowManager()->getLoadingScreen();
        Loading::ScopedLoad load(loadingListener);

        loadingListener->setLabel("#{sLoadingMessage4}");

        while (!mPreloader->syncTerrainLoad(vec, mRendering.getReferenceTime(), *loadingListener)) {}
    }

    void Scene::reloadTerrain()
    {
        mPreloader->setTerrainPreloadPositions(std::vector<PositionCellGrid>());
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

    void Scene::preloadFastTravelDestinations(const osg::Vec3f& playerPos, const osg::Vec3f& /*predictedPos*/, std::vector<PositionCellGrid>& exteriorPositions) // ignore predictedPos here since opening dialogue with travel service takes extra time
    {
        const MWWorld::ConstPtr player = mWorld.getPlayerPtr();
        ListFastTravelDestinationsVisitor listVisitor(mPreloadDistance, player.getRefData().getPosition().asVec3());

        for (MWWorld::CellStore* cellStore : mActiveCells)
        {
            cellStore->forEachType<ESM::NPC>(listVisitor);
            cellStore->forEachType<ESM::Creature>(listVisitor);
        }

        for (ESM::Transport::Dest& dest : listVisitor.mList)
        {
            if (!dest.mCellName.empty())
                preloadCell(mWorld.getInterior(dest.mCellName));
            else
            {
                osg::Vec3f pos = dest.mPos.asVec3();
                const osg::Vec2i cellIndex = positionToCellIndex(pos.x(), pos.y());
                preloadCell(mWorld.getExterior(cellIndex.x(), cellIndex.y()), true);
                exteriorPositions.emplace_back(pos, gridCenterToBounds(getNewGridCenter(pos)));
            }
        }
    }
}
