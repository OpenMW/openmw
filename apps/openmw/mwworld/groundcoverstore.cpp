#include "groundcoverstore.hpp"

#include <components/esmloader/load.hpp>
#include <components/esmloader/esmdata.hpp>
#include <components/misc/stringops.hpp>
#include <components/esm3/readerscache.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/resource/resourcesystem.hpp>

#include <apps/openmw/mwbase/environment.hpp>

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
        const ::EsmLoader::EsmData content = ::EsmLoader::loadEsmData(query, groundcoverFiles, fileCollections,
                                                                      readers, encoder, listener);

        const VFS::Manager* const vfs = MWBase::Environment::get().getResourceSystem()->getVFS();

        static constexpr std::string_view prefix = "grass\\";
        for (const ESM::Static& stat : statics)
        {
            std::string id = Misc::StringUtils::lowerCase(stat.mId);
            std::string model = Misc::StringUtils::lowerCase(stat.mModel);
            std::replace(model.begin(), model.end(), '/', '\\');
            if (model.compare(0, prefix.size(), prefix) != 0)
                continue;
            mMeshCache[id] = Misc::ResourceHelpers::correctMeshPath(model, vfs);
        }

        for (const ESM::Static& stat : content.mStatics)
        {
            std::string id = Misc::StringUtils::lowerCase(stat.mId);
            std::string model = Misc::StringUtils::lowerCase(stat.mModel);
            std::replace(model.begin(), model.end(), '/', '\\');
            if (model.compare(0, prefix.size(), prefix) != 0)
                continue;
            mMeshCache[id] = Misc::ResourceHelpers::correctMeshPath(model, vfs);
        }

        for (const ESM::Cell& cell : content.mCells)
        {
            if (!cell.isExterior()) continue;
            auto cellIndex = std::make_pair(cell.getCellId().mIndex.mX, cell.getCellId().mIndex.mY);
            mCellContexts[cellIndex] = std::move(cell.mContextList);
        }
    }

    std::string GroundcoverStore::getGroundcoverModel(const std::string& id) const
    {
        std::string idLower = Misc::StringUtils::lowerCase(id);
        auto search = mMeshCache.find(idLower);
        if (search == mMeshCache.end()) return std::string();

        return search->second;
    }

    void GroundcoverStore::initCell(ESM::Cell& cell, int cellX, int cellY) const
    {
        cell.blank();

        auto searchCell = mCellContexts.find(std::make_pair(cellX, cellY));
        if (searchCell != mCellContexts.end())
            cell.mContextList = searchCell->second;
    }
}
