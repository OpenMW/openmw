#include "foreachbulletobject.hpp"

#include <components/debug/debuglog.hpp>
#include <components/esm3/cellref.hpp>
#include <components/esm3/esmreader.hpp>
#include <components/esm3/loadcell.hpp>
#include <components/esm3/readerscache.hpp>
#include <components/esmloader/esmdata.hpp>
#include <components/esmloader/lessbyid.hpp>
#include <components/esmloader/record.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/misc/strings/lower.hpp>
#include <components/resource/bulletshapemanager.hpp>
#include <components/vfs/manager.hpp>

#include <osg/ref_ptr>

#include <algorithm>
#include <utility>
#include <vector>

namespace Resource
{
    namespace
    {
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
                const ESM::ReadersCache::BusyItem reader
                    = readers.get(static_cast<std::size_t>(cell.mContextList[i].index));
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
                        constexpr VFS::Path::NormalizedView prefix("meshes");
                        return bulletShapeManager.getShape(prefix / model);
                    }
                    catch (const std::exception& e)
                    {
                        Log(Debug::Warning) << "Failed to load cell ref \"" << cellRef.mRefId << "\" model \"" << model
                                            << "\": " << e.what();
                        return osg::ref_ptr<const Resource::BulletShape>();
                    }
                }();

                if (shape == nullptr)
                    continue;

                switch (cellRef.mType)
                {
                    case ESM::REC_ACTI:
                    case ESM::REC_CONT:
                    case ESM::REC_DOOR:
                    case ESM::REC_STAT:
                        f(BulletObject{ std::move(shape), cellRef.mPos, cellRef.mScale });
                        break;
                    default:
                        break;
                }
            }
        }
    }

    void forEachBulletObject(ESM::ReadersCache& readers, const VFS::Manager& vfs,
        Resource::BulletShapeManager& bulletShapeManager, const EsmLoader::EsmData& esmData,
        std::function<void(const ESM::Cell& cell, const BulletObject& object)> callback)
    {
        Log(Debug::Info) << "Processing " << esmData.mCells.size() << " cells...";

        for (std::size_t i = 0; i < esmData.mCells.size(); ++i)
        {
            const ESM::Cell& cell = esmData.mCells[i];
            const bool exterior = cell.isExterior();

            Log(Debug::Debug) << "Processing " << (exterior ? "exterior" : "interior") << " cell (" << (i + 1) << "/"
                              << esmData.mCells.size() << ") \"" << cell.getDescription() << "\"";

            std::size_t objects = 0;

            forEachObject(cell, esmData, vfs, bulletShapeManager, readers, [&](const BulletObject& object) {
                callback(cell, object);
                ++objects;
            });

            Log(Debug::Info) << "Processed " << (exterior ? "exterior" : "interior") << " cell (" << (i + 1) << "/"
                             << esmData.mCells.size() << ") " << cell.getDescription() << " with " << objects
                             << " objects";
        }
    }
}
