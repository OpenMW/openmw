#ifndef GAME_MWWORLD_WORLDMODEL_H
#define GAME_MWWORLD_WORLDMODEL_H

#include <list>
#include <map>
#include <string>
#include <string_view>
#include <unordered_map>

#include <components/esm/exteriorcelllocation.hpp>
#include <components/misc/algorithm.hpp>

#include "cellstore.hpp"
#include "ptr.hpp"
#include "ptrregistry.hpp"

namespace ESM
{
    class ESMReader;
    class ESMWriter;
    class ReadersCache;
    struct Cell;
}

namespace ESM4
{
    struct Cell;
}

namespace Loading
{
    class Listener;
}

namespace MWWorld
{
    class ESMStore;

    /// \brief Cell container
    class WorldModel
    {
    public:
        explicit WorldModel(ESMStore& store, ESM::ReadersCache& reader);

        WorldModel(const WorldModel&) = delete;
        WorldModel& operator=(const WorldModel&) = delete;

        void clear();

        CellStore& getExterior(ESM::ExteriorCellLocation location, bool forceLoad = true) const;

        CellStore* findCell(ESM::RefId Id, bool forceLoad = true) const;

        CellStore& getCell(ESM::RefId Id, bool forceLoad = true) const;

        // Returns a special cell that is never active. Can be used for creating objects
        // without adding them to the scene.
        CellStore& getDraftCell() const;

        CellStore* findInterior(std::string_view name, bool forceLoad = true) const;

        CellStore& getInterior(std::string_view name, bool forceLoad = true) const;

        CellStore* findCell(std::string_view name, bool forceLoad = true) const;

        CellStore& getCell(std::string_view name, bool forceLoad = true) const;

        Ptr getPtrByRefId(const ESM::RefId& name);

        Ptr getPtr(ESM::RefNum refNum) const { return mPtrRegistry.getOrEmpty(refNum); }

        PtrRegistryView getPtrRegistryView() const { return PtrRegistryView(mPtrRegistry); }

        ESM::RefNum getLastGeneratedRefNum() const { return mPtrRegistry.getLastGenerated(); }

        void setLastGeneratedRefNum(ESM::RefNum v) { mPtrRegistry.setLastGenerated(v); }

        std::size_t getPtrRegistryRevision() const { return mPtrRegistry.getRevision(); }

        void registerPtr(const Ptr& ptr) { mPtrRegistry.insert(ptr); }

        void deregisterLiveCellRef(const LiveCellRefBase& ref) noexcept { mPtrRegistry.remove(ref); }

        void assignSaveFileRefNum(ESM::CellRef& ref) { mPtrRegistry.assign(ref); }

        template <typename Fn>
        void forEachLoadedCellStore(Fn&& fn)
        {
            for (auto& [_, store] : mCells)
                fn(store);
        }

        /// Get all Ptrs referencing \a name in exterior cells
        /// @note Due to the current implementation of getPtr this only supports one Ptr per cell.
        /// @note name must be lower case
        void getExteriorPtrs(const ESM::RefId& name, std::vector<MWWorld::Ptr>& out);

        std::vector<MWWorld::Ptr> getAll(const ESM::RefId& id);

        int countSavedGameRecords() const;

        void write(ESM::ESMWriter& writer, Loading::Listener& progress) const;

        bool readRecord(ESM::ESMReader& reader, uint32_t type);

    private:
        PtrRegistry mPtrRegistry; // defined before mCells because during destruction it should be the last

        MWWorld::ESMStore& mStore;
        ESM::ReadersCache& mReaders;
        mutable std::unordered_map<ESM::RefId, CellStore> mCells;
        mutable std::map<std::string, CellStore*, Misc::StringUtils::CiComp> mInteriors;
        mutable std::map<ESM::ExteriorCellLocation, CellStore*> mExteriors;
        ESM::Cell mDraftCell;
        std::vector<std::pair<ESM::RefId, CellStore*>> mIdCache;
        std::size_t mIdCacheIndex = 0;

        CellStore& getOrInsertCellStore(const ESM::Cell& cell);

        CellStore& insertCellStore(const ESM::Cell& cell);

        Ptr getPtrAndCache(const ESM::RefId& name, CellStore& cellStore);

        void writeCell(ESM::ESMWriter& writer, CellStore& cell) const;
    };
}

#endif
