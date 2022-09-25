#ifndef GAME_MWWORLD_WORLDMODEL_H
#define GAME_MWWORLD_WORLDMODEL_H

#include <list>
#include <map>
#include <string>

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
        mutable std::map<std::string, CellStore> mInteriors;
        mutable std::map<std::pair<int, int>, CellStore> mExteriors;
        IdCache mIdCache;
        std::size_t mIdCacheIndex;

        WorldModel(const WorldModel&);
        WorldModel& operator=(const WorldModel&);

        CellStore* getCellStore(const ESM::Cell* cell);

        Ptr getPtrAndCache(const ESM::RefId& name, CellStore& cellStore);

        Ptr getPtr(CellStore& cellStore, const ESM::RefId& id, const ESM::RefNum& refNum);

        void writeCell(ESM::ESMWriter& writer, CellStore& cell) const;

    public:
        void clear();

        explicit WorldModel(const MWWorld::ESMStore& store, ESM::ReadersCache& reader);

        CellStore* getExterior(int x, int y);

        CellStore* getInterior(std::string_view name);

        CellStore* getCell(const ESM::CellId& id);

        Ptr getPtr(const ESM::RefId& name, CellStore& cellStore, bool searchInContainers = false);
        ///< \param searchInContainers Only affect loaded cells.
        /// @note name must be lower case

        /// @note name must be lower case
        Ptr getPtr(const ESM::RefId& name);

        Ptr getPtr(const ESM::RefId& id, const ESM::RefNum& refNum);

        void rest(double hours);
        void recharge(float duration);

        /// Get all Ptrs referencing \a name in exterior cells
        /// @note Due to the current implementation of getPtr this only supports one Ptr per cell.
        /// @note name must be lower case
        void getExteriorPtrs(const ESM::RefId& name, std::vector<MWWorld::Ptr>& out);

        /// Get all Ptrs referencing \a name in interior cells
        /// @note Due to the current implementation of getPtr this only supports one Ptr per cell.
        /// @note name must be lower case
        void getInteriorPtrs(const ESM::RefId& name, std::vector<MWWorld::Ptr>& out);

        std::vector<MWWorld::Ptr> getAll(const ESM::RefId& id);

        int countSavedGameRecords() const;

        void write(ESM::ESMWriter& writer, Loading::Listener& progress) const;

        bool readRecord(ESM::ESMReader& reader, uint32_t type, const std::map<int, int>& contentFileMap);
    };
}

#endif
