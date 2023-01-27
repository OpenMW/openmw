#include "worldmodel.hpp"

#include <components/debug/debuglog.hpp>
#include <components/esm/defs.hpp>
#include <components/esm3/cellref.hpp>
#include <components/esm3/cellstate.hpp>
#include <components/esm3/esmreader.hpp>
#include <components/esm3/esmwriter.hpp>
#include <components/esm3/loadregn.hpp>
#include <components/loadinglistener/loadinglistener.hpp>
#include <components/settings/settings.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "cellstore.hpp"
#include "cellutils.hpp"
#include "esmstore.hpp"

namespace
{
    template <class Visitor, class Key, class Comp>
    bool forEachInStore(const ESM::RefId& id, Visitor&& visitor, std::map<Key, MWWorld::CellStore, Comp>& cellStore)
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

MWWorld::CellStore* MWWorld::WorldModel::getCellStore(const ESM::Cell* cell)
{
    if (cell->mData.mFlags & ESM::Cell::Interior)
    {
        auto result = mInteriors.find(cell->mName);

        if (result == mInteriors.end())
            result = mInteriors.emplace(cell->mName, CellStore(MWWorld::Cell(cell), mStore, mReaders)).first;

        return &result->second;
    }
    else
    {
        std::map<std::pair<int, int>, CellStore>::iterator result
            = mExteriors.find(std::make_pair(cell->getGridX(), cell->getGridY()));

        if (result == mExteriors.end())
            result = mExteriors
                         .emplace(std::make_pair(cell->getGridX(), cell->getGridY()),
                             CellStore(MWWorld::Cell(cell), mStore, mReaders))
                         .first;

        return &result->second;
    }
}

void MWWorld::WorldModel::clear()
{
    mPtrIndex.clear();
    mPtrIndexUpdateCounter = 0;
    mLastGeneratedRefnum = ESM::RefNum{};
    mInteriors.clear();
    mExteriors.clear();
    std::fill(mIdCache.begin(), mIdCache.end(), std::make_pair(ESM::RefId::sEmpty, (MWWorld::CellStore*)nullptr));
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
    Ptr ptr = getPtr(name, cellStore);

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
    cellState.mId.save(writer);
    cellState.save(writer);
    cell.writeFog(writer);
    cell.writeReferences(writer);
    writer.endRecord(ESM::REC_CSTA);
}

MWWorld::WorldModel::WorldModel(const MWWorld::ESMStore& store, ESM::ReadersCache& readers)
    : mStore(store)
    , mReaders(readers)
    , mIdCacheIndex(0)
    , mPtrIndexUpdateCounter(0)
{
    int cacheSize = std::clamp(Settings::Manager::getInt("pointers cache size", "Cells"), 40, 1000);
    mIdCache = IdCache(cacheSize, std::pair<ESM::RefId, CellStore*>(ESM::RefId::sEmpty, (CellStore*)nullptr));
}

MWWorld::CellStore* MWWorld::WorldModel::getExterior(int x, int y)
{
    std::map<std::pair<int, int>, CellStore>::iterator result = mExteriors.find(std::make_pair(x, y));

    if (result == mExteriors.end())
    {
        const ESM::Cell* cell = mStore.get<ESM::Cell>().search(x, y);

        if (!cell)
        {
            // Cell isn't predefined. Make one on the fly.
            ESM::Cell record;
            record.mCellId.mWorldspace = ESM::CellId::sDefaultWorldspace;
            record.mCellId.mPaged = true;
            record.mCellId.mIndex.mX = x;
            record.mCellId.mIndex.mY = y;

            record.mData.mFlags = ESM::Cell::HasWater;
            record.mData.mX = x;
            record.mData.mY = y;
            record.mWater = 0;
            record.mMapColor = 0;

            cell = MWBase::Environment::get().getWorld()->createRecord(record);
        }

        result = mExteriors.emplace(std::make_pair(x, y), CellStore(MWWorld::Cell(cell), mStore, mReaders)).first;
    }

    if (result->second.getState() != CellStore::State_Loaded)
    {
        result->second.load();
    }

    return &result->second;
}

MWWorld::CellStore* MWWorld::WorldModel::getInterior(std::string_view name)
{
    auto result = mInteriors.find(name);

    if (result == mInteriors.end())
    {
        const ESM4::Cell* cell4 = mStore.get<ESM4::Cell>().searchCellName(name);

        if (!cell4)
        {
            const ESM::Cell* cell = mStore.get<ESM::Cell>().find(name);
            result = mInteriors.emplace(name, CellStore(MWWorld::Cell(cell), mStore, mReaders)).first;
        }
        else
        {
            result = mInteriors.emplace(name, CellStore(MWWorld::Cell(cell4), mStore, mReaders)).first;
        }
    }

    if (result->second.getState() != CellStore::State_Loaded)
    {
        result->second.load();
    }

    return &result->second;
}

MWWorld::CellStore* MWWorld::WorldModel::getCell(const ESM::CellId& id)
{
    if (id.mPaged)
        return getExterior(id.mIndex.mX, id.mIndex.mY);

    return getInterior(id.mWorldspace);
}

const ESM::Cell* MWWorld::WorldModel::getESMCellByName(std::string_view name)
{
    const ESM::Cell* cell = mStore.get<ESM::Cell>().search(name); // first try interiors
    if (!cell) // try named exteriors
        cell = mStore.get<ESM::Cell>().searchExtByName(name);
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
    return cell;
}

ESM::CellVariant MWWorld::WorldModel::getCellByName(std::string_view name)
{
    const ESM::Cell* cellEsm3 = getESMCellByName(name);
    return ESM::CellVariant(cellEsm3);
    if (!cellEsm3)
    {
        const ESM4::Cell* cellESM4 = mStore.get<ESM4::Cell>().searchCellName(name);
        return ESM::CellVariant(cellESM4);
    }
}

MWWorld::CellStore* MWWorld::WorldModel::getCell(std::string_view name)
{
    const ESM::Cell* cell = getESMCellByName(name);
    if (cell->isExterior())
        return getExterior(cell->getGridX(), cell->getGridY());
    else
        return getInterior(name);
}

MWWorld::CellStore* MWWorld::WorldModel::getCellByPosition(
    const osg::Vec3f& pos, std::string_view cellNameInSameWorldSpace)
{
    if (cellNameInSameWorldSpace.empty() || getESMCellByName(cellNameInSameWorldSpace)->isExterior())
    {
        const osg::Vec2i cellIndex = positionToCellIndex(pos.x(), pos.y());
        return getExterior(cellIndex.x(), cellIndex.y());
    }
    else
        return getInterior(cellNameInSameWorldSpace);
}

MWWorld::Ptr MWWorld::WorldModel::getPtr(const ESM::RefId& name, CellStore& cell, bool searchInContainers)
{
    if (cell.getState() == CellStore::State_Unloaded)
        cell.preload();

    if (cell.getState() == CellStore::State_Preloaded)
    {
        if (cell.hasId(name))
        {
            cell.load();
        }
        else
            return Ptr();
    }

    Ptr ptr = cell.search(name);

    if (!ptr.isEmpty() && MWWorld::CellStore::isAccessible(ptr.getRefData(), ptr.getCellRef()))
        return ptr;

    if (searchInContainers)
        return cell.searchInContainer(name);

    return Ptr();
}

MWWorld::Ptr MWWorld::WorldModel::getPtr(const ESM::RefId& name)
{
    // First check the cache
    for (IdCache::iterator iter(mIdCache.begin()); iter != mIdCache.end(); ++iter)
        if (iter->first == name && iter->second)
        {
            Ptr ptr = getPtr(name, *iter->second);
            if (!ptr.isEmpty())
                return ptr;
        }

    // Then check cells that are already listed
    // Search in reverse, this is a workaround for an ambiguous chargen_plank reference in the vanilla game.
    // there is one at -22,16 and one at -2,-9, the latter should be used.
    for (std::map<std::pair<int, int>, CellStore>::reverse_iterator iter = mExteriors.rbegin();
         iter != mExteriors.rend(); ++iter)
    {
        Ptr ptr = getPtrAndCache(name, iter->second);
        if (!ptr.isEmpty())
            return ptr;
    }

    for (auto iter = mInteriors.begin(); iter != mInteriors.end(); ++iter)
    {
        Ptr ptr = getPtrAndCache(name, iter->second);
        if (!ptr.isEmpty())
            return ptr;
    }

    // Now try the other cells
    const MWWorld::Store<ESM::Cell>& cells = mStore.get<ESM::Cell>();
    MWWorld::Store<ESM::Cell>::iterator iter;

    for (iter = cells.extBegin(); iter != cells.extEnd(); ++iter)
    {
        CellStore* cellStore = getCellStore(&(*iter));

        Ptr ptr = getPtrAndCache(name, *cellStore);

        if (!ptr.isEmpty())
            return ptr;
    }

    for (iter = cells.intBegin(); iter != cells.intEnd(); ++iter)
    {
        CellStore* cellStore = getCellStore(&(*iter));

        Ptr ptr = getPtrAndCache(name, *cellStore);

        if (!ptr.isEmpty())
            return ptr;
    }

    // giving up
    return Ptr();
}

MWWorld::Ptr MWWorld::WorldModel::getPtr(const ESM::RefId& id, const ESM::RefNum& refNum)
{
    for (auto& pair : mInteriors)
    {
        Ptr ptr = getPtr(pair.second, id, refNum);
        if (!ptr.isEmpty())
            return ptr;
    }
    for (auto& pair : mExteriors)
    {
        Ptr ptr = getPtr(pair.second, id, refNum);
        if (!ptr.isEmpty())
            return ptr;
    }
    return Ptr();
}

MWWorld::Ptr MWWorld::WorldModel::getPtr(CellStore& cellStore, const ESM::RefId& id, const ESM::RefNum& refNum)
{
    if (cellStore.getState() == CellStore::State_Unloaded)
        cellStore.preload();
    if (cellStore.getState() == CellStore::State_Preloaded)
    {
        if (cellStore.hasId(id))
            cellStore.load();
        else
            return Ptr();
    }
    return cellStore.searchViaRefNum(refNum);
}

void MWWorld::WorldModel::getExteriorPtrs(const ESM::RefId& name, std::vector<MWWorld::Ptr>& out)
{
    const MWWorld::Store<ESM::Cell>& cells = mStore.get<ESM::Cell>();
    for (MWWorld::Store<ESM::Cell>::iterator iter = cells.extBegin(); iter != cells.extEnd(); ++iter)
    {
        CellStore* cellStore = getCellStore(&(*iter));

        Ptr ptr = getPtrAndCache(name, *cellStore);

        if (!ptr.isEmpty())
            out.push_back(ptr);
    }
}

void MWWorld::WorldModel::getInteriorPtrs(const ESM::RefId& name, std::vector<MWWorld::Ptr>& out)
{
    const MWWorld::Store<ESM::Cell>& cells = mStore.get<ESM::Cell>();
    for (MWWorld::Store<ESM::Cell>::iterator iter = cells.intBegin(); iter != cells.intEnd(); ++iter)
    {
        CellStore* cellStore = getCellStore(&(*iter));

        Ptr ptr = getPtrAndCache(name, *cellStore);

        if (!ptr.isEmpty())
            out.push_back(ptr);
    }
}

std::vector<MWWorld::Ptr> MWWorld::WorldModel::getAll(const ESM::RefId& id)
{
    PtrCollector visitor;
    if (forEachInStore(id, visitor, mInteriors))
        forEachInStore(id, visitor, mExteriors);
    return visitor.mPtrs;
}

int MWWorld::WorldModel::countSavedGameRecords() const
{
    int count = 0;

    for (auto iter(mInteriors.begin()); iter != mInteriors.end(); ++iter)
        if (iter->second.hasState())
            ++count;

    for (std::map<std::pair<int, int>, CellStore>::const_iterator iter(mExteriors.begin()); iter != mExteriors.end();
         ++iter)
        if (iter->second.hasState())
            ++count;

    return count;
}

void MWWorld::WorldModel::write(ESM::ESMWriter& writer, Loading::Listener& progress) const
{
    for (std::map<std::pair<int, int>, CellStore>::iterator iter(mExteriors.begin()); iter != mExteriors.end(); ++iter)
        if (iter->second.hasState())
        {
            writeCell(writer, iter->second);
            progress.increaseProgress();
        }

    for (auto iter(mInteriors.begin()); iter != mInteriors.end(); ++iter)
        if (iter->second.hasState())
        {
            writeCell(writer, iter->second);
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

    MWWorld::CellStore* getCellStore(const ESM::CellId& cellId) override
    {
        try
        {
            return mWorldModel.getCell(cellId);
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
        state.mId.load(reader);

        CellStore* cellStore = nullptr;

        try
        {
            cellStore = getCell(state.mId);
        }
        catch (...)
        {
            // silently drop cells that don't exist anymore
            Log(Debug::Warning) << "Warning: Dropping state for cell " << state.mId.mWorldspace
                                << " (cell no longer exists)";
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
