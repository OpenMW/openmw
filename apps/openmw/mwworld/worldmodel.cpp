#include "worldmodel.hpp"

#include <components/debug/debuglog.hpp>
#include <components/esm/defs.hpp>
#include <components/esm3/cellid.hpp>
#include <components/esm3/cellref.hpp>
#include <components/esm3/cellstate.hpp>
#include <components/esm3/esmreader.hpp>
#include <components/esm3/esmwriter.hpp>
#include <components/esm3/loadregn.hpp>
#include <components/esm4/loadwrld.hpp>
#include <components/loadinglistener/loadinglistener.hpp>
#include <components/settings/values.hpp>

#include "cellstore.hpp"
#include "esmstore.hpp"

namespace
{
    template <class Visitor, class Key, class Comp>
    bool forEachInStore(
        const ESM::RefId& id, Visitor&& visitor, std::unordered_map<Key, MWWorld::CellStore, Comp>& cellStore)
    {
        for (auto& cell : cellStore)
        {
            if (cell.second.getState() == MWWorld::CellStore::State_Unloaded)
                cell.second.preload();
            if (cell.second.getState() == MWWorld::CellStore::State_Preloaded)
            {
                if (cell.second.hasId(id))
                {
                    cell.second.load();
                }
                else
                    continue;
            }
            bool cont = cell.second.forEach([&](MWWorld::Ptr ptr) {
                if (ptr.getCellRef().getRefId() == id)
                {
                    return visitor(ptr);
                }
                return true;
            });
            if (!cont)
                return false;
        }
        return true;
    }

    struct PtrCollector
    {
        std::vector<MWWorld::Ptr> mPtrs;

        bool operator()(MWWorld::Ptr ptr)
        {
            mPtrs.emplace_back(ptr);
            return true;
        }
    };
}

MWWorld::CellStore& MWWorld::WorldModel::getOrInsertCellStore(const ESM::Cell& cell)
{
    const auto it = mCells.find(cell.mId);
    if (it != mCells.end())
        return it->second;
    return insertCellStore(cell);
}

MWWorld::CellStore& MWWorld::WorldModel::insertCellStore(const ESM::Cell& cell)
{
    CellStore& cellStore = mCells.emplace(cell.mId, CellStore(Cell(cell), mStore, mReaders)).first->second;
    if (cell.mData.mFlags & ESM::Cell::Interior)
        mInteriors.emplace(cell.mName, &cellStore);
    else
        mExteriors.emplace(
            ESM::ExteriorCellLocation(cell.getGridX(), cell.getGridY(), ESM::Cell::sDefaultWorldspaceId), &cellStore);
    return cellStore;
}

void MWWorld::WorldModel::clear()
{
    mPtrIndex.clear();
    mPtrIndexUpdateCounter = 0;
    mLastGeneratedRefnum = ESM::RefNum{};
    mInteriors.clear();
    mExteriors.clear();
    mCells.clear();
    std::fill(mIdCache.begin(), mIdCache.end(), std::make_pair(ESM::RefId(), (MWWorld::CellStore*)nullptr));
    mIdCacheIndex = 0;
}

void MWWorld::WorldModel::registerPtr(const MWWorld::Ptr& ptr)
{
    mPtrIndex[ptr.getCellRef().getOrAssignRefNum(mLastGeneratedRefnum)] = ptr;
    mPtrIndexUpdateCounter++;
}

void MWWorld::WorldModel::deregisterPtr(const MWWorld::Ptr& ptr)
{
    mPtrIndex.erase(ptr.getCellRef().getRefNum());
    mPtrIndexUpdateCounter++;
}

MWWorld::Ptr MWWorld::WorldModel::getPtr(const ESM::RefNum& refNum) const
{
    auto it = mPtrIndex.find(refNum);
    if (it != mPtrIndex.end())
        return it->second;
    else
        return MWWorld::Ptr();
}

MWWorld::Ptr MWWorld::WorldModel::getPtrAndCache(const ESM::RefId& name, CellStore& cellStore)
{
    Ptr ptr = cellStore.getPtr(name);

    if (!ptr.isEmpty() && ptr.isInCell())
    {
        mIdCache[mIdCacheIndex].first = name;
        mIdCache[mIdCacheIndex].second = &cellStore;
        if (++mIdCacheIndex >= mIdCache.size())
            mIdCacheIndex = 0;
    }

    return ptr;
}

void MWWorld::WorldModel::writeCell(ESM::ESMWriter& writer, CellStore& cell) const
{
    if (cell.getState() != CellStore::State_Loaded)
        cell.load();

    ESM::CellState cellState;

    cell.saveState(cellState);

    writer.startRecord(ESM::REC_CSTA);

    writer.writeCellId(cellState.mId);
    cellState.save(writer);
    cell.writeFog(writer);
    cell.writeReferences(writer);
    writer.endRecord(ESM::REC_CSTA);
}

MWWorld::WorldModel::WorldModel(MWWorld::ESMStore& store, ESM::ReadersCache& readers)
    : mStore(store)
    , mReaders(readers)
    , mIdCache(Settings::cells().mPointersCacheSize, { ESM::RefId(), nullptr })
{
}

MWWorld::CellStore& MWWorld::WorldModel::getExterior(ESM::ExteriorCellLocation cellIndex, bool forceLoad)
{
    std::map<ESM::ExteriorCellLocation, CellStore*>::iterator result;

    result = mExteriors.find(cellIndex);

    if (result == mExteriors.end())
    {
        if (!ESM::isEsm4Ext(cellIndex.mWorldspace))
        {
            const ESM::Cell* cell = mStore.get<ESM::Cell>().search(cellIndex.mX, cellIndex.mY);

            if (cell == nullptr)
            {
                // Cell isn't predefined. Make one on the fly.
                ESM::Cell record;
                record.mData.mFlags = ESM::Cell::HasWater;
                record.mData.mX = cellIndex.mX;
                record.mData.mY = cellIndex.mY;
                record.mWater = 0;
                record.mMapColor = 0;
                record.updateId();

                cell = mStore.insert(record);
            }

            CellStore* cellStore
                = &mCells.emplace(cell->mId, CellStore(MWWorld::Cell(*cell), mStore, mReaders)).first->second;
            result = mExteriors.emplace(cellIndex, cellStore).first;
        }
        else
        {
            const Store<ESM4::Cell>& cell4Store = mStore.get<ESM4::Cell>();
            bool exteriorExists = mStore.get<ESM4::World>().search(cellIndex.mWorldspace) != nullptr;
            const ESM4::Cell* cell = cell4Store.searchExterior(cellIndex);
            if (!exteriorExists)
                throw std::runtime_error("Exterior ESM4 world is not found: " + cellIndex.mWorldspace.toDebugString());
            if (cell == nullptr)
            {
                ESM4::Cell record;
                record.mParent = cellIndex.mWorldspace;
                record.mX = cellIndex.mX;
                record.mY = cellIndex.mY;
                record.mCellFlags = 0;
                cell = mStore.insert(record);
            }
            CellStore* cellStore
                = &mCells.emplace(cell->mId, CellStore(MWWorld::Cell(*cell), mStore, mReaders)).first->second;
            result = mExteriors.emplace(cellIndex, cellStore).first;
        }
    }
    if (forceLoad && result->second->getState() != CellStore::State_Loaded)
    {
        result->second->load();
    }

    return *result->second;
}

MWWorld::CellStore* MWWorld::WorldModel::getInteriorOrNull(std::string_view name)
{
    auto result = mInteriors.find(name);
    if (result == mInteriors.end())
    {
        CellStore* newCellStore = nullptr;
        if (const ESM::Cell* cell = mStore.get<ESM::Cell>().search(name))
            newCellStore = &mCells.emplace(cell->mId, CellStore(MWWorld::Cell(*cell), mStore, mReaders)).first->second;
        else if (const ESM4::Cell* cell4 = mStore.get<ESM4::Cell>().searchCellName(name))
            newCellStore
                = &mCells.emplace(cell4->mId, CellStore(MWWorld::Cell(*cell4), mStore, mReaders)).first->second;
        if (!newCellStore)
            return nullptr; // Cell not found
        result = mInteriors.emplace(name, newCellStore).first;
    }
    return result->second;
}

MWWorld::CellStore& MWWorld::WorldModel::getInterior(std::string_view name, bool forceLoad)
{
    CellStore* res = getInteriorOrNull(name);
    if (res == nullptr)
        throw std::runtime_error("Interior not found: '" + std::string(name) + "'");
    if (forceLoad && res->getState() != CellStore::State_Loaded)
        res->load();
    return *res;
}

MWWorld::CellStore& MWWorld::WorldModel::getCell(const ESM::RefId& id, bool forceLoad)
{
    auto result = mCells.find(id);
    if (result != mCells.end())
        return result->second;

    if (const auto* exteriorId = id.getIf<ESM::ESM3ExteriorCellRefId>())
        return getExterior(
            ESM::ExteriorCellLocation(exteriorId->getX(), exteriorId->getY(), ESM::Cell::sDefaultWorldspaceId),
            forceLoad);

    const ESM4::Cell* cell4 = mStore.get<ESM4::Cell>().search(id);
    CellStore* newCellStore = nullptr;
    if (!cell4)
    {
        const ESM::Cell* cell = mStore.get<ESM::Cell>().find(id);
        newCellStore = &mCells.emplace(cell->mId, CellStore(MWWorld::Cell(*cell), mStore, mReaders)).first->second;
    }
    else
    {
        newCellStore = &mCells.emplace(cell4->mId, CellStore(MWWorld::Cell(*cell4), mStore, mReaders)).first->second;
    }
    if (newCellStore->getCell()->isExterior())
    {
        std::pair<int, int> coord
            = std::make_pair(newCellStore->getCell()->getGridX(), newCellStore->getCell()->getGridY());
        ESM::ExteriorCellLocation extIndex = { coord.first, coord.second, newCellStore->getCell()->getWorldSpace() };
        mExteriors.emplace(extIndex, newCellStore);
    }
    else
    {
        mInteriors.emplace(newCellStore->getCell()->getNameId(), newCellStore);
    }
    if (forceLoad && newCellStore->getState() != CellStore::State_Loaded)
    {
        newCellStore->load();
    }
    return *newCellStore;
}

MWWorld::CellStore& MWWorld::WorldModel::getCell(std::string_view name, bool forceLoad)
{
    if (CellStore* res = getInteriorOrNull(name)) // first try interiors
    {
        if (forceLoad && res->getState() != CellStore::State_Loaded)
            res->load();
        return *res;
    }

    // try named exteriors
    const ESM::Cell* cell = mStore.get<ESM::Cell>().searchExtByName(name);

    if (!cell)
    {
        // treat "Wilderness" like an empty string
        static const std::string& defaultName
            = mStore.get<ESM::GameSetting>().find("sDefaultCellname")->mValue.getString();
        if (Misc::StringUtils::ciEqual(name, defaultName))
            cell = mStore.get<ESM::Cell>().searchExtByName({});
    }
    if (!cell)
    {
        // now check for regions
        for (const ESM::Region& region : mStore.get<ESM::Region>())
        {
            if (Misc::StringUtils::ciEqual(name, region.mName))
            {
                cell = mStore.get<ESM::Cell>().searchExtByRegion(region.mId);
                break;
            }
        }
    }
    if (!cell)
        throw std::runtime_error(std::string("Can't find cell with name ") + std::string(name));

    return getExterior(
        ESM::ExteriorCellLocation(cell->getGridX(), cell->getGridY(), ESM::Cell::sDefaultWorldspaceId), forceLoad);
}

MWWorld::Ptr MWWorld::WorldModel::getPtr(const ESM::RefId& name)
{
    for (const auto& [cachedId, cellStore] : mIdCache)
    {
        if (cachedId != name || cellStore == nullptr)
            continue;
        Ptr ptr = cellStore->getPtr(name);
        if (!ptr.isEmpty())
            return ptr;
    }

    // Then check cells that are already listed
    // Search in reverse, this is a workaround for an ambiguous chargen_plank reference in the vanilla game.
    // there is one at -22,16 and one at -2,-9, the latter should be used.
    for (auto iter = mExteriors.rbegin(); iter != mExteriors.rend(); ++iter)
    {
        Ptr ptr = getPtrAndCache(name, *iter->second);
        if (!ptr.isEmpty())
            return ptr;
    }

    for (auto iter = mInteriors.begin(); iter != mInteriors.end(); ++iter)
    {
        Ptr ptr = getPtrAndCache(name, *iter->second);
        if (!ptr.isEmpty())
            return ptr;
    }

    // Now try the other cells
    const MWWorld::Store<ESM::Cell>& cells = mStore.get<ESM::Cell>();

    for (auto iter = cells.extBegin(); iter != cells.extEnd(); ++iter)
    {
        if (mCells.contains(iter->mId))
            continue;

        Ptr ptr = getPtrAndCache(name, insertCellStore(*iter));

        if (!ptr.isEmpty())
            return ptr;
    }

    for (auto iter = cells.intBegin(); iter != cells.intEnd(); ++iter)
    {
        if (mCells.contains(iter->mId))
            continue;

        Ptr ptr = getPtrAndCache(name, insertCellStore(*iter));

        if (!ptr.isEmpty())
            return ptr;
    }

    // giving up
    return Ptr();
}

void MWWorld::WorldModel::getExteriorPtrs(const ESM::RefId& name, std::vector<MWWorld::Ptr>& out)
{
    const MWWorld::Store<ESM::Cell>& cells = mStore.get<ESM::Cell>();
    for (MWWorld::Store<ESM::Cell>::iterator iter = cells.extBegin(); iter != cells.extEnd(); ++iter)
    {
        CellStore& cellStore = getOrInsertCellStore(*iter);

        Ptr ptr = getPtrAndCache(name, cellStore);

        if (!ptr.isEmpty())
            out.push_back(ptr);
    }
}

std::vector<MWWorld::Ptr> MWWorld::WorldModel::getAll(const ESM::RefId& id)
{
    PtrCollector visitor;
    forEachInStore(id, visitor, mCells);
    return visitor.mPtrs;
}

int MWWorld::WorldModel::countSavedGameRecords() const
{
    int count = 0;

    for (auto iter(mCells.begin()); iter != mCells.end(); ++iter)
        if (iter->second.hasState())
            ++count;

    return count;
}

void MWWorld::WorldModel::write(ESM::ESMWriter& writer, Loading::Listener& progress) const
{
    for (auto& [id, cellStore] : mCells)
        if (cellStore.hasState())
        {
            writeCell(writer, cellStore);
            progress.increaseProgress();
        }
}

struct GetCellStoreCallback : public MWWorld::CellStore::GetCellStoreCallback
{
public:
    GetCellStoreCallback(MWWorld::WorldModel& worldModel)
        : mWorldModel(worldModel)
    {
    }

    MWWorld::WorldModel& mWorldModel;

    MWWorld::CellStore* getCellStore(const ESM::RefId& cellId) override
    {
        try
        {
            return &mWorldModel.getCell(cellId);
        }
        catch (...)
        {
            return nullptr;
        }
    }
};

bool MWWorld::WorldModel::readRecord(ESM::ESMReader& reader, uint32_t type, const std::map<int, int>& contentFileMap)
{
    if (type == ESM::REC_CSTA)
    {
        ESM::CellState state;
        state.mId = reader.getCellId();

        CellStore* cellStore = nullptr;

        try
        {
            cellStore = &getCell(state.mId);
        }
        catch (...)
        {
            // silently drop cells that don't exist anymore
            Log(Debug::Warning) << "Warning: Dropping state for cell " << state.mId << " (cell no longer exists)";
            reader.skipRecord();
            return true;
        }

        state.load(reader);
        cellStore->loadState(state);

        if (state.mHasFogOfWar)
            cellStore->readFog(reader);

        if (cellStore->getState() != CellStore::State_Loaded)
            cellStore->load();

        GetCellStoreCallback callback(*this);

        cellStore->readReferences(reader, contentFileMap, &callback);

        return true;
    }

    return false;
}
