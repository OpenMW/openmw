#include "groundcoverstore.hpp"

#include <components/esm3/loadcell.hpp>
#include <components/esm3/loadstat.hpp>
#include <components/esm3/readerscache.hpp>
#include <components/esmloader/esmdata.hpp>
#include <components/esmloader/load.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/misc/strings/lower.hpp>
#include <components/resource/resourcesystem.hpp>

#include "store.hpp"

namespace MWWorld
{
    void GroundcoverStore::init(const Store<ESM::Static>& statics, const Files::Collections& fileCollections,
        const std::vector<std::string>& groundcoverFiles, ToUTF8::Utf8Encoder* encoder, Loading::Listener* listener)
    {
        ::EsmLoader::Query query;
        query.mLoadStatics = true;
        query.mLoadCells = true;

        ESM::ReadersCache readers;
        ::EsmLoader::EsmData content
            = ::EsmLoader::loadEsmData(query, groundcoverFiles, fileCollections, readers, encoder, listener);

        static constexpr std::string_view prefix = "grass/";
        for (const ESM::Static& stat : statics)
        {
            VFS::Path::Normalized model = VFS::Path::toNormalized(stat.mModel);
            if (!model.value().starts_with(prefix))
                continue;
            mMeshCache[stat.mId] = Misc::ResourceHelpers::correctMeshPath(model);
        }

        for (const ESM::Static& stat : content.mStatics)
        {
            VFS::Path::Normalized model = VFS::Path::toNormalized(stat.mModel);
            if (!model.value().starts_with(prefix))
                continue;
            mMeshCache[stat.mId] = Misc::ResourceHelpers::correctMeshPath(model);
        }

        for (ESM::Cell& cell : content.mCells)
        {
            if (!cell.isExterior())
                continue;
            auto cellIndex = std::make_pair(cell.getGridX(), cell.getGridY());
            mCellContexts[cellIndex] = std::move(cell.mContextList);
        }
    }

    void GroundcoverStore::initCell(ESM::Cell& cell, int cellX, int cellY) const
    {
        cell.blank();

        auto searchCell = mCellContexts.find(std::make_pair(cellX, cellY));
        if (searchCell != mCellContexts.end())
            cell.mContextList = searchCell->second;
    }
}
