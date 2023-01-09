#ifndef GAME_MWWORLD_WORLDMODEL_H
#define GAME_MWWORLD_WORLDMODEL_H

#include <list>
#include <map>
#include <string>
#include <unordered_map>
#include <variant>

#include <components/misc/algorithm.hpp>

#include "cellstore.hpp"
#include "ptr.hpp"

namespace ESM
{
    class ESMReader;
    class ESMWriter;
    class ReadersCache;
    struct CellId;
    struct Cell;
    struct RefNum;
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
        typedef std::vector<std::pair<ESM::RefId, CellStore*>> IdCache;
        const MWWorld::ESMStore& mStore;
        ESM::ReadersCache& mReaders;
        mutable std::map<std::string, CellStore, Misc::StringUtils::CiComp> mInteriors;
        mutable std::map<std::pair<int, int>, CellStore> mExteriors;
        IdCache mIdCache;
        std::size_t mIdCacheIndex = 0;
        std::unordered_map<ESM::RefNum, Ptr> mPtrIndex;
        std::size_t mPtrIndexUpdateCounter = 0;
        ESM::RefNum mLastGeneratedRefnum;

        WorldModel(const WorldModel&);
        WorldModel& operator=(const WorldModel&);

        const ESM::Cell* getESMCellByName(std::string_view name);
        ESM::CellVariant getCellByName(std::string_view name);

        CellStore* getCellStore(const ESM::Cell* cell);
        Ptr getPtrAndCache(const ESM::RefId& name, CellStore& cellStore);

        void writeCell(ESM::ESMWriter& writer, CellStore& cell) const;

    public:
        void clear();

        explicit WorldModel(const MWWorld::ESMStore& store, ESM::ReadersCache& reader);

        CellStore* getExterior(int x, int y);
        CellStore* getInterior(std::string_view name);
        CellStore* getCell(std::string_view name); // interior or named exterior
        CellStore* getCell(const ESM::CellId& Id);

        // If cellNameInSameWorldSpace is an interior - returns this interior.
        // Otherwise returns exterior cell for given position in the same world space.
        // At the moment multiple world spaces are not supported, so all exteriors are in one world space.
        CellStore* getCellByPosition(const osg::Vec3f& pos, std::string_view cellNameInSameWorldSpace);

        void registerPtr(const MWWorld::Ptr& ptr);
        void deregisterPtr(const MWWorld::Ptr& ptr);
        ESM::RefNum getLastGeneratedRefNum() const { return mLastGeneratedRefnum; }
        void setLastGeneratedRefNum(ESM::RefNum v) { mLastGeneratedRefnum = v; }
        size_t getPtrIndexUpdateCounter() const { return mPtrIndexUpdateCounter; }
        const std::unordered_map<ESM::RefNum, Ptr>& getAllPtrs() const { return mPtrIndex; }

        Ptr getPtr(const ESM::RefNum& refNum) const;

        Ptr getPtr(const ESM::RefId& name, CellStore& cellStore);
        Ptr getPtr(const ESM::RefId& name);

        template <typename Fn>
        void forEachLoadedCellStore(Fn&& fn)
        {
            for (auto& [_, store] : mInteriors)
                fn(store);
            for (auto& [_, store] : mExteriors)
                fn(store);
        }

        /// Get all Ptrs referencing \a name in exterior cells
        /// @note Due to the current implementation of getPtr this only supports one Ptr per cell.
        /// @note name must be lower case
        void getExteriorPtrs(const ESM::RefId& name, std::vector<MWWorld::Ptr>& out);

        std::vector<MWWorld::Ptr> getAll(const ESM::RefId& id);

        int countSavedGameRecords() const;

        void write(ESM::ESMWriter& writer, Loading::Listener& progress) const;

        bool readRecord(ESM::ESMReader& reader, uint32_t type, const std::map<int, int>& contentFileMap);
    };
}

#endif
