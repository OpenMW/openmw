#include "worldspacedata.hpp"

#include <components/bullethelpers/aabb.hpp>
#include <components/debug/debugging.hpp>
#include <components/debug/debuglog.hpp>
#include <components/detournavigator/debug.hpp>
#include <components/detournavigator/gettilespositions.hpp>
#include <components/detournavigator/objectid.hpp>
#include <components/detournavigator/recastmesh.hpp>
#include <components/detournavigator/settings.hpp>
#include <components/detournavigator/tilecachedrecastmeshmanager.hpp>
#include <components/esm/refid.hpp>
#include <components/esm3/cellref.hpp>
#include <components/esm3/esmreader.hpp>
#include <components/esm3/loadcell.hpp>
#include <components/esm3/loadland.hpp>
#include <components/esm3/readerscache.hpp>
#include <components/esmloader/esmdata.hpp>
#include <components/esmloader/lessbyid.hpp>
#include <components/esmloader/record.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/misc/strings/conversion.hpp>
#include <components/misc/strings/lower.hpp>
#include <components/navmeshtool/protocol.hpp>
#include <components/resource/bulletshapemanager.hpp>
#include <components/settings/settings.hpp>
#include <components/vfs/manager.hpp>

#include <LinearMath/btVector3.h>

#include <osg/Vec2i>
#include <osg/ref_ptr>

#include <algorithm>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace NavMeshTool
{
    namespace
    {
        using DetourNavigator::CollisionShape;
        using DetourNavigator::HeightfieldPlane;
        using DetourNavigator::HeightfieldShape;
        using DetourNavigator::HeightfieldSurface;
        using DetourNavigator::ObjectId;
        using DetourNavigator::ObjectTransform;

        struct CellRef
        {
            ESM::RecNameInts mType;
            ESM::RefNum mRefNum;
            ESM::RefId mRefId;
            float mScale;
            ESM::Position mPos;

            CellRef(
                ESM::RecNameInts type, ESM::RefNum refNum, ESM::RefId&& refId, float scale, const ESM::Position& pos)
                : mType(type)
                , mRefNum(refNum)
                , mRefId(std::move(refId))
                , mScale(scale)
                , mPos(pos)
            {
            }
        };

        ESM::RecNameInts getType(const EsmLoader::EsmData& esmData, const ESM::RefId& refId)
        {
            const auto it = std::lower_bound(
                esmData.mRefIdTypes.begin(), esmData.mRefIdTypes.end(), refId, EsmLoader::LessById{});
            if (it == esmData.mRefIdTypes.end() || it->mId != refId)
                return {};
            return it->mType;
        }

        std::vector<CellRef> loadCellRefs(
            const ESM::Cell& cell, const EsmLoader::EsmData& esmData, ESM::ReadersCache& readers)
        {
            std::vector<EsmLoader::Record<CellRef>> cellRefs;

            for (std::size_t i = 0; i < cell.mContextList.size(); i++)
            {
                ESM::ReadersCache::BusyItem reader = readers.get(static_cast<std::size_t>(cell.mContextList[i].index));
                cell.restore(*reader, static_cast<int>(i));
                ESM::CellRef cellRef;
                bool deleted = false;
                while (ESM::Cell::getNextRef(*reader, cellRef, deleted))
                {
                    const ESM::RecNameInts type = getType(esmData, cellRef.mRefID);
                    if (type == ESM::RecNameInts{})
                        continue;
                    cellRefs.emplace_back(
                        deleted, type, cellRef.mRefNum, std::move(cellRef.mRefID), cellRef.mScale, cellRef.mPos);
                }
            }

            Log(Debug::Debug) << "Loaded " << cellRefs.size() << " cell refs";

            const auto getKey = [](const EsmLoader::Record<CellRef>& v) -> ESM::RefNum { return v.mValue.mRefNum; };
            std::vector<CellRef> result = prepareRecords(cellRefs, getKey);

            Log(Debug::Debug) << "Prepared " << result.size() << " unique cell refs";

            return result;
        }

        template <class F>
        void forEachObject(const ESM::Cell& cell, const EsmLoader::EsmData& esmData, const VFS::Manager& vfs,
            Resource::BulletShapeManager& bulletShapeManager, ESM::ReadersCache& readers, F&& f)
        {
            std::vector<CellRef> cellRefs = loadCellRefs(cell, esmData, readers);

            Log(Debug::Debug) << "Prepared " << cellRefs.size() << " unique cell refs";

            for (CellRef& cellRef : cellRefs)
            {
                VFS::Path::Normalized model(getModel(esmData, cellRef.mRefId, cellRef.mType));
                if (model.empty())
                    continue;

                if (cellRef.mType != ESM::REC_STAT)
                    model = Misc::ResourceHelpers::correctActorModelPath(model, &vfs);

                osg::ref_ptr<const Resource::BulletShape> shape = [&] {
                    try
                    {
                        return bulletShapeManager.getShape(Misc::ResourceHelpers::correctMeshPath(model));
                    }
                    catch (const std::exception& e)
                    {
                        Log(Debug::Warning) << "Failed to load cell ref \"" << cellRef.mRefId << "\" model \"" << model
                                            << "\": " << e.what();
                        return osg::ref_ptr<const Resource::BulletShape>();
                    }
                }();

                if (shape == nullptr || shape->mCollisionShape == nullptr)
                    continue;

                osg::ref_ptr<Resource::BulletShapeInstance> shapeInstance(
                    new Resource::BulletShapeInstance(std::move(shape)));

                switch (cellRef.mType)
                {
                    case ESM::REC_ACTI:
                    case ESM::REC_CONT:
                    case ESM::REC_DOOR:
                    case ESM::REC_STAT:
                        f(BulletObject(std::move(shapeInstance), cellRef.mPos, cellRef.mScale));
                        break;
                    default:
                        break;
                }
            }
        }

        struct GetXY
        {
            osg::Vec2i operator()(const ESM::Land& value) const { return osg::Vec2i(value.mX, value.mY); }
        };

        struct LessByXY
        {
            bool operator()(const ESM::Land& lhs, const ESM::Land& rhs) const { return GetXY{}(lhs) < GetXY{}(rhs); }

            bool operator()(const ESM::Land& lhs, const osg::Vec2i& rhs) const { return GetXY{}(lhs) < rhs; }

            bool operator()(const osg::Vec2i& lhs, const ESM::Land& rhs) const { return lhs < GetXY{}(rhs); }
        };

        btAABB getAabb(const osg::Vec2i& cellPosition, btScalar minHeight, btScalar maxHeight)
        {
            btAABB aabb;
            aabb.m_min = btVector3(static_cast<btScalar>(cellPosition.x() * ESM::Land::REAL_SIZE),
                static_cast<btScalar>(cellPosition.y() * ESM::Land::REAL_SIZE), minHeight);
            aabb.m_max = btVector3(static_cast<btScalar>((cellPosition.x() + 1) * ESM::Land::REAL_SIZE),
                static_cast<btScalar>((cellPosition.y() + 1) * ESM::Land::REAL_SIZE), maxHeight);
            return aabb;
        }

        void mergeOrAssign(const btAABB& aabb, btAABB& target, bool& initialized)
        {
            if (initialized)
                return target.merge(aabb);

            target.m_min = aabb.m_min;
            target.m_max = aabb.m_max;
            initialized = true;
        }

        std::tuple<HeightfieldShape, float, float> makeHeightfieldShape(const std::optional<ESM::Land>& land,
            const osg::Vec2i& cellPosition, std::vector<std::vector<float>>& heightfields,
            std::vector<std::unique_ptr<ESM::Land::LandData>>& landDatas)
        {
            if (!land.has_value() || osg::Vec2i(land->mX, land->mY) != cellPosition
                || (land->mDataTypes & ESM::Land::DATA_VHGT) == 0)
                return { HeightfieldPlane{ static_cast<float>(ESM::Land::DEFAULT_HEIGHT) },
                    static_cast<float>(ESM::Land::DEFAULT_HEIGHT), static_cast<float>(ESM::Land::DEFAULT_HEIGHT) };

            ESM::Land::LandData& landData = *landDatas.emplace_back(std::make_unique<ESM::Land::LandData>());
            land->loadData(ESM::Land::DATA_VHGT, landData);
            heightfields.push_back(std::vector<float>(std::begin(landData.mHeights), std::end(landData.mHeights)));
            HeightfieldSurface surface;
            surface.mHeights = heightfields.back().data();
            surface.mMinHeight = landData.mMinHeight;
            surface.mMaxHeight = landData.mMaxHeight;
            surface.mSize = static_cast<std::size_t>(ESM::Land::LAND_SIZE);
            return { surface, landData.mMinHeight, landData.mMaxHeight };
        }

        template <class T>
        void serializeToStderr(const T& value)
        {
            const std::vector<std::byte> data = serialize(value);
            Debug::getRawStderr().write(
                reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
        }

        std::string makeAddObjectErrorMessage(
            ObjectId objectId, DetourNavigator::AreaType areaType, const CollisionShape& shape)
        {
            std::ostringstream stream;
            stream << "Failed to add object to recast mesh objectId=" << objectId.value() << " areaType=" << areaType
                   << " fileName=" << shape.getInstance()->mFileName
                   << " fileHash=" << Misc::StringUtils::toHex(shape.getInstance()->mFileHash);
            return stream.str();
        }
    }

    WorldspaceData::WorldspaceData(ESM::RefId worldspace, const DetourNavigator::RecastSettings& settings)
        : mWorldspace(worldspace)
        , mTileCachedRecastMeshManager(std::make_unique<TileCachedRecastMeshManager>(settings))
    {
        mAabb.m_min = btVector3(0, 0, 0);
        mAabb.m_max = btVector3(0, 0, 0);
    }

    std::unordered_map<ESM::RefId, std::vector<std::size_t>> collectWorldspaceCells(
        const EsmLoader::EsmData& esmData, bool processInteriorCells, const std::regex& worldspaceFilter)
    {
        Log(Debug::Info) << "Collecting worldspaces from " << esmData.mCells.size() << " cells...";

        std::unordered_map<ESM::RefId, std::vector<std::size_t>> result;

        for (std::size_t i = 0; i < esmData.mCells.size(); ++i)
        {
            const ESM::Cell& cell = esmData.mCells[i];
            const bool exterior = cell.isExterior();

            if (!exterior && !processInteriorCells)
            {
                Log(Debug::Verbose) << "Skipped interior"
                                    << " cell (" << (i + 1) << "/" << esmData.mCells.size() << ") \""
                                    << cell.getDescription() << "\"";
                continue;
            }

            const ESM::RefId cellWorldspace = cell.isExterior() ? ESM::Cell::sDefaultWorldspaceId : cell.mId;

            if (!std::regex_match(cellWorldspace.toString(), worldspaceFilter))
            {
                Log(Debug::Verbose) << "Skipped filtered out"
                                    << " cell (" << (i + 1) << "/" << esmData.mCells.size() << ") \""
                                    << cell.getDescription() << "\" from " << cellWorldspace << " worldspace";
                continue;
            }

            result[cellWorldspace].push_back(i);

            Log(Debug::Info) << "Collected " << (exterior ? "exterior" : "interior") << " cell (" << (i + 1) << "/"
                             << esmData.mCells.size() << ") " << cell.getDescription();
        }

        Log(Debug::Info) << "Collected " << result.size() << " worldspaces";

        return result;
    }

    WorldspaceData gatherWorldspaceData(const DetourNavigator::Settings& settings, ESM::ReadersCache& readers,
        const VFS::Manager& vfs, Resource::BulletShapeManager& bulletShapeManager, const EsmLoader::EsmData& esmData,
        bool writeBinaryLog, ESM::RefId worldspace, std::span<const std::size_t> cells)
    {
        Log(Debug::Info) << "Processing " << cells.size() << " cells from worldspace " << worldspace << "...";

        if (writeBinaryLog)
            serializeToStderr(ExpectedCells{ static_cast<std::uint64_t>(cells.size()) });

        WorldspaceData data(worldspace, settings.mRecast);
        TileCachedRecastMeshManager& manager = *data.mTileCachedRecastMeshManager;

        const auto guard = manager.makeUpdateGuard();

        manager.setWorldspace(worldspace, guard.get());

        std::size_t objectsCounter = 0;

        for (std::size_t i = 0; i < cells.size(); ++i)
        {
            const ESM::Cell& cell = esmData.mCells[cells[i]];
            const bool exterior = cell.isExterior();

            Log(Debug::Debug) << "Processing " << (exterior ? "exterior" : "interior") << " cell (" << (i + 1) << "/"
                              << cells.size() << ") \"" << cell.getDescription() << "\"";

            const osg::Vec2i cellPosition(cell.mData.mX, cell.mData.mY);
            const std::size_t cellObjectsBegin = data.mObjects.size();

            if (exterior)
            {
                const auto it
                    = std::lower_bound(esmData.mLands.begin(), esmData.mLands.end(), cellPosition, LessByXY{});
                const auto [heightfieldShape, minHeight, maxHeight]
                    = makeHeightfieldShape(it == esmData.mLands.end() ? std::optional<ESM::Land>() : *it, cellPosition,
                        data.mHeightfields, data.mLandData);

                mergeOrAssign(getAabb(cellPosition, minHeight, maxHeight), data.mAabb, data.mAabbInitialized);

                manager.addHeightfield(cellPosition, ESM::Land::REAL_SIZE, heightfieldShape, guard.get());

                manager.addWater(cellPosition, ESM::Land::REAL_SIZE, -1, guard.get());
            }
            else
            {
                if ((cell.mData.mFlags & ESM::Cell::HasWater) != 0)
                    manager.addWater(cellPosition, std::numeric_limits<int>::max(), cell.mWater, guard.get());
            }

            forEachObject(cell, esmData, vfs, bulletShapeManager, readers, [&](BulletObject object) {
                if (object.getShapeInstance()->mVisualCollisionType != Resource::VisualCollisionType::None)
                    return;

                const btTransform& transform = object.getCollisionObject().getWorldTransform();
                const btAABB aabb = BulletHelpers::getAabb(*object.getCollisionObject().getCollisionShape(), transform);
                mergeOrAssign(aabb, data.mAabb, data.mAabbInitialized);
                if (const btCollisionShape* avoid = object.getShapeInstance()->mAvoidCollisionShape.get())
                    data.mAabb.merge(BulletHelpers::getAabb(*avoid, transform));

                const ObjectId objectId(++objectsCounter);
                const CollisionShape shape(object.getShapeInstance(), *object.getCollisionObject().getCollisionShape(),
                    object.getObjectTransform());

                if (!manager.addObject(objectId, shape, transform, DetourNavigator::AreaType_ground, guard.get()))
                    throw std::logic_error(
                        makeAddObjectErrorMessage(objectId, DetourNavigator::AreaType_ground, shape));

                if (const btCollisionShape* avoid = object.getShapeInstance()->mAvoidCollisionShape.get())
                {
                    const ObjectId avoidObjectId(++objectsCounter);
                    const CollisionShape avoidShape(object.getShapeInstance(), *avoid, object.getObjectTransform());
                    if (!manager.addObject(
                            avoidObjectId, avoidShape, transform, DetourNavigator::AreaType_null, guard.get()))
                        throw std::logic_error(
                            makeAddObjectErrorMessage(avoidObjectId, DetourNavigator::AreaType_null, avoidShape));
                }

                data.mObjects.emplace_back(std::move(object));
            });

            if (writeBinaryLog)
                serializeToStderr(ProcessedCells{ static_cast<std::uint64_t>(i + 1) });

            Log(Debug::Info) << "Processed " << (exterior ? "exterior" : "interior") << " cell (" << (i + 1) << "/"
                             << cells.size() << ") " << cell.getDescription() << " with "
                             << (data.mObjects.size() - cellObjectsBegin) << " objects";
        }

        Log(Debug::Info) << "Processed " << cells.size() << " cells, added " << data.mObjects.size() << " objects and "
                         << data.mHeightfields.size() << " height fields";

        return data;
    }
}
