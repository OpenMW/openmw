#include "cells.hpp"

#include <components/debug/debuglog.hpp>
#include <components/esm/esmreader.hpp>
#include <components/esm/esmwriter.hpp>
#include <components/esm/defs.hpp>
#include <components/esm/cellstate.hpp>
#include <components/loadinglistener/loadinglistener.hpp>
#include <components/settings/settings.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "esmstore.hpp"
#include "containerstore.hpp"
#include "cellstore.hpp"

MWWorld::CellStore *MWWorld::Cells::getCellStore (const ESM::Cell *cell)
{
    if (cell->mData.mFlags & ESM::Cell::Interior)
    {
        std::string lowerName(Misc::StringUtils::lowerCase(cell->mName));
        std::map<std::string, CellStore>::iterator result = mInteriors.find (lowerName);

        if (result==mInteriors.end())
        {
            result = mInteriors.insert (std::make_pair (lowerName, CellStore (cell, mStore, mReader))).first;
        }

        return &result->second;
    }
    else
    {
        std::map<std::pair<int, int>, CellStore>::iterator result =
            mExteriors.find (std::make_pair (cell->getGridX(), cell->getGridY()));

        if (result==mExteriors.end())
        {
            result = mExteriors.insert (std::make_pair (
                std::make_pair (cell->getGridX(), cell->getGridY()), CellStore (cell, mStore, mReader))).first;

        }

        return &result->second;
    }
}

void MWWorld::Cells::clear()
{
    mInteriors.clear();
    mExteriors.clear();
    std::fill(mIdCache.begin(), mIdCache.end(), std::make_pair("", (MWWorld::CellStore*)0));
    mIdCacheIndex = 0;
}

MWWorld::Ptr MWWorld::Cells::getPtrAndCache (const std::string& name, CellStore& cellStore)
{
    Ptr ptr = getPtr (name, cellStore);

    if (!ptr.isEmpty() && ptr.isInCell())
    {
        mIdCache[mIdCacheIndex].first = name;
        mIdCache[mIdCacheIndex].second = &cellStore;
        if (++mIdCacheIndex>=mIdCache.size())
            mIdCacheIndex = 0;
    }

    return ptr;
}

void MWWorld::Cells::writeCell (ESM::ESMWriter& writer, CellStore& cell) const
{
    if (cell.getState()!=CellStore::State_Loaded)
        cell.load ();

    ESM::CellState cellState;

    cell.saveState (cellState);

    writer.startRecord (ESM::REC_CSTA);
    cellState.mId.save (writer);
    cellState.save (writer);
    cell.writeFog(writer);
    cell.writeReferences (writer);
    writer.endRecord (ESM::REC_CSTA);
}

MWWorld::Cells::Cells (const MWWorld::ESMStore& store, std::vector<ESM::ESMReader>& reader)
: mStore (store), mReader (reader),
  mIdCache (Settings::Manager::getInt("pointers cache size", "Cells"), std::pair<std::string, CellStore *> ("", (CellStore*)0)),
  mIdCacheIndex (0)
{}

MWWorld::CellStore *MWWorld::Cells::getExterior (int x, int y)
{
    std::map<std::pair<int, int>, CellStore>::iterator result =
        mExteriors.find (std::make_pair (x, y));

    if (result==mExteriors.end())
    {
        const ESM::Cell *cell = mStore.get<ESM::Cell>().search(x, y);

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

            cell = MWBase::Environment::get().getWorld()->createRecord (record);
        }

        result = mExteriors.insert (std::make_pair (
            std::make_pair (x, y), CellStore (cell, mStore, mReader))).first;
    }

    if (result->second.getState()!=CellStore::State_Loaded)
    {
        result->second.load ();
    }

    return &result->second;
}

MWWorld::CellStore *MWWorld::Cells::getInterior (const std::string& name)
{
    std::string lowerName = Misc::StringUtils::lowerCase(name);
    std::map<std::string, CellStore>::iterator result = mInteriors.find (lowerName);

    if (result==mInteriors.end())
    {
        const ESM::Cell *cell = mStore.get<ESM::Cell>().find(lowerName);

        result = mInteriors.insert (std::make_pair (lowerName, CellStore (cell, mStore, mReader))).first;
    }

    if (result->second.getState()!=CellStore::State_Loaded)
    {
        result->second.load ();
    }

    return &result->second;
}

void MWWorld::Cells::rest (double hours)
{
    for (auto &interior : mInteriors)
    {
        interior.second.rest(hours);
    }

    for (auto &exterior : mExteriors)
    {
        exterior.second.rest(hours);
    }
}

void MWWorld::Cells::recharge (float duration)
{
    for (auto &interior : mInteriors)
    {
        interior.second.recharge(duration);
    }

    for (auto &exterior : mExteriors)
    {
        exterior.second.recharge(duration);
    }
}

MWWorld::CellStore *MWWorld::Cells::getCell (const ESM::CellId& id)
{
    if (id.mPaged)
        return getExterior (id.mIndex.mX, id.mIndex.mY);

    return getInterior (id.mWorldspace);
}

MWWorld::Ptr MWWorld::Cells::getPtr (const std::string& name, CellStore& cell,
    bool searchInContainers)
{
    if (cell.getState()==CellStore::State_Unloaded)
        cell.preload ();

    if (cell.getState()==CellStore::State_Preloaded)
    {
        if (cell.hasId (name))
        {
            cell.load ();
        }
        else
            return Ptr();
    }

    Ptr ptr = cell.search (name);

    if (!ptr.isEmpty() && MWWorld::CellStore::isAccessible(ptr.getRefData(), ptr.getCellRef()))
        return ptr;

    if (searchInContainers)
        return cell.searchInContainer (name);

    return Ptr();
}

MWWorld::Ptr MWWorld::Cells::getPtr (const std::string& name)
{
    // First check the cache
    for (std::vector<std::pair<std::string, CellStore *> >::iterator iter (mIdCache.begin());
        iter!=mIdCache.end(); ++iter)
        if (iter->first==name && iter->second)
        {
            Ptr ptr = getPtr (name, *iter->second);
            if (!ptr.isEmpty())
                return ptr;
        }

    // Then check cells that are already listed
    // Search in reverse, this is a workaround for an ambiguous chargen_plank reference in the vanilla game.
    // there is one at -22,16 and one at -2,-9, the latter should be used.
    for (std::map<std::pair<int, int>, CellStore>::reverse_iterator iter = mExteriors.rbegin();
        iter!=mExteriors.rend(); ++iter)
    {
        Ptr ptr = getPtrAndCache (name, iter->second);
        if (!ptr.isEmpty())
            return ptr;
    }

    for (std::map<std::string, CellStore>::iterator iter = mInteriors.begin();
        iter!=mInteriors.end(); ++iter)
    {
        Ptr ptr = getPtrAndCache (name, iter->second);
        if (!ptr.isEmpty())
            return ptr;
    }

    // Now try the other cells
    const MWWorld::Store<ESM::Cell> &cells = mStore.get<ESM::Cell>();
    MWWorld::Store<ESM::Cell>::iterator iter;

    for (iter = cells.extBegin(); iter != cells.extEnd(); ++iter)
    {
        CellStore *cellStore = getCellStore (&(*iter));

        Ptr ptr = getPtrAndCache (name, *cellStore);

        if (!ptr.isEmpty())
            return ptr;
    }

    for (iter = cells.intBegin(); iter != cells.intEnd(); ++iter)
    {
        CellStore *cellStore = getCellStore (&(*iter));

        Ptr ptr = getPtrAndCache (name, *cellStore);

        if (!ptr.isEmpty())
            return ptr;
    }

    // giving up
    return Ptr();
}

void MWWorld::Cells::getExteriorPtrs(const std::string &name, std::vector<MWWorld::Ptr> &out)
{
    const MWWorld::Store<ESM::Cell> &cells = mStore.get<ESM::Cell>();
    for (MWWorld::Store<ESM::Cell>::iterator iter = cells.extBegin(); iter != cells.extEnd(); ++iter)
    {
        CellStore *cellStore = getCellStore (&(*iter));

        Ptr ptr = getPtrAndCache (name, *cellStore);

        if (!ptr.isEmpty())
            out.push_back(ptr);
    }
}

void MWWorld::Cells::getInteriorPtrs(const std::string &name, std::vector<MWWorld::Ptr> &out)
{
    const MWWorld::Store<ESM::Cell> &cells = mStore.get<ESM::Cell>();
    for (MWWorld::Store<ESM::Cell>::iterator iter = cells.intBegin(); iter != cells.intEnd(); ++iter)
    {
        CellStore *cellStore = getCellStore (&(*iter));

        Ptr ptr = getPtrAndCache (name, *cellStore);

        if (!ptr.isEmpty())
            out.push_back(ptr);
    }
}

int MWWorld::Cells::countSavedGameRecords() const
{
    int count = 0;

    for (std::map<std::string, CellStore>::const_iterator iter (mInteriors.begin());
        iter!=mInteriors.end(); ++iter)
        if (iter->second.hasState())
            ++count;

    for (std::map<std::pair<int, int>, CellStore>::const_iterator iter (mExteriors.begin());
        iter!=mExteriors.end(); ++iter)
        if (iter->second.hasState())
            ++count;

    return count;
}

void MWWorld::Cells::write (ESM::ESMWriter& writer, Loading::Listener& progress) const
{
    for (std::map<std::pair<int, int>, CellStore>::iterator iter (mExteriors.begin());
        iter!=mExteriors.end(); ++iter)
        if (iter->second.hasState())
        {
            writeCell (writer, iter->second);
            progress.increaseProgress();
        }

    for (std::map<std::string, CellStore>::iterator iter (mInteriors.begin());
        iter!=mInteriors.end(); ++iter)
        if (iter->second.hasState())
        {
            writeCell (writer, iter->second);
            progress.increaseProgress();
        }
}

struct GetCellStoreCallback : public MWWorld::CellStore::GetCellStoreCallback
{
public:
    GetCellStoreCallback(MWWorld::Cells& cells)
        : mCells(cells)
    {
    }

    MWWorld::Cells& mCells;

    virtual MWWorld::CellStore* getCellStore(const ESM::CellId& cellId)
    {
        try
        {
            return mCells.getCell(cellId);
        }
        catch (...)
        {
            return nullptr;
        }
    }
};

bool MWWorld::Cells::readRecord (ESM::ESMReader& reader, uint32_t type,
    const std::map<int, int>& contentFileMap)
{
    if (type==ESM::REC_CSTA)
    {
        ESM::CellState state;
        state.mId.load (reader);

        CellStore *cellStore = 0;

        try
        {
            cellStore = getCell (state.mId);
        }
        catch (...)
        {
            // silently drop cells that don't exist anymore
            Log(Debug::Warning) << "Warning: Dropping state for cell " << state.mId.mWorldspace << " (cell no longer exists)";
            reader.skipRecord();
            return true;
        }

        state.load (reader);
        cellStore->loadState (state);

        if (state.mHasFogOfWar)
            cellStore->readFog(reader);

        if (cellStore->getState()!=CellStore::State_Loaded)
            cellStore->load ();

        GetCellStoreCallback callback(*this);

        cellStore->readReferences (reader, contentFileMap, &callback);

        return true;
    }

    return false;
}
