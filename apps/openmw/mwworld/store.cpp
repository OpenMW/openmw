#include "store.hpp"
#include "esmstore.hpp"

#include <components/esm/esmreader.hpp>

namespace MWWorld {

void Store<ESM::Cell>::handleMovedCellRefs(ESM::ESMReader& esm, ESM::Cell* cell)
{
    //Handling MovedCellRefs, there is no way to do it inside loadcell
    while (esm.isNextSub("MVRF")) {
        ESM::CellRef ref;
        ESM::MovedCellRef cMRef;
        cell->getNextMVRF(esm, cMRef);

        ESM::Cell *cellAlt = const_cast<ESM::Cell*>(searchOrCreate(cMRef.mTarget[0], cMRef.mTarget[1]));

        // Get regular moved reference data. Adapted from CellStore::loadRefs. Maybe we can optimize the following
        //  implementation when the oher implementation works as well.
        bool deleted = false;
        cell->getNextRef(esm, ref, deleted);

        // Add data required to make reference appear in the correct cell.
        // We should not need to test for duplicates, as this part of the code is pre-cell merge.
        cell->mMovedRefs.push_back(cMRef);
        // But there may be duplicates here!
        if (!deleted)
        {
            ESM::CellRefTracker::iterator iter = std::find(cellAlt->mLeasedRefs.begin(), cellAlt->mLeasedRefs.end(), ref.mRefNum);
            if (iter == cellAlt->mLeasedRefs.end())
              cellAlt->mLeasedRefs.push_back(ref);
            else
              *iter = ref;
        }
    }
}

void Store<ESM::Cell>::load(ESM::ESMReader &esm)
{
    // Don't automatically assume that a new cell must be spawned. Multiple plugins write to the same cell,
    //  and we merge all this data into one Cell object. However, we can't simply search for the cell id,
    //  as many exterior cells do not have a name. Instead, we need to search by (x,y) coordinates - and they
    //  are not available until both cells have been loaded at least partially!

    // All cells have a name record, even nameless exterior cells.
    ESM::Cell cell;
    cell.loadName(esm);
    std::string idLower = Misc::StringUtils::lowerCase(cell.mName);

    // Load the (x,y) coordinates of the cell, if it is an exterior cell,
    // so we can find the cell we need to merge with
    cell.loadData(esm);

    if(cell.mData.mFlags & ESM::Cell::Interior)
    {
        // Store interior cell by name, try to merge with existing parent data.
        ESM::Cell *oldcell = const_cast<ESM::Cell*>(search(idLower));
        if (oldcell) {
            // merge new cell into old cell
            // push the new references on the list of references to manage (saveContext = true)
            oldcell->mData = cell.mData;
            oldcell->mName = cell.mName; // merge name just to be sure (ID will be the same, but case could have been changed)
            oldcell->loadCell(esm, true);
        } else
        {
            // spawn a new cell
            cell.loadCell(esm, true);

        record.load(esm, isDeleted);
        Misc::StringUtils::lowerCaseInPlace(record.mId);

        std::pair<typename Static::iterator, bool> inserted = mStatic.insert(std::make_pair(record.mId, record));
        if (inserted.second)
            mShared.push_back(&inserted.first->second);
        else
            inserted.first->second = record;

        return RecordId(record.mId, isDeleted);
    }
    template<typename T>
    void Store<T>::setUp()
    {
    }

    template<typename T>
    typename Store<T>::iterator Store<T>::begin() const
    {
        return mShared.begin(); 
    }
    template<typename T>
    typename Store<T>::iterator Store<T>::end() const
    {
        return mShared.end();
    }

    template<typename T>
    size_t Store<T>::getSize() const
    {
        return mShared.size();
    }

    template<typename T>
    int Store<T>::getDynamicSize() const
    {
        return mDynamic.size();
    }
    template<typename T>
    void Store<T>::listIdentifier(std::vector<std::string> &list) const
    {
        list.reserve(list.size() + getSize());
        typename std::vector<T *>::const_iterator it = mShared.begin();
        for (; it != mShared.end(); ++it) {
            list.push_back((*it)->mId);
        }
    }
    else
    {
        // Store exterior cells by grid position, try to merge with existing parent data.
        ESM::Cell *oldcell = const_cast<ESM::Cell*>(search(cell.getGridX(), cell.getGridY()));
        if (oldcell) {
            // merge new cell into old cell
            oldcell->mData = cell.mData;
            oldcell->mName = cell.mName;
            oldcell->loadCell(esm, false);

            // handle moved ref (MVRF) subrecords
            handleMovedCellRefs (esm, &cell);

            // push the new references on the list of references to manage
            oldcell->postLoad(esm);

            // merge lists of leased references, use newer data in case of conflict
            for (ESM::MovedCellRefTracker::const_iterator it = cell.mMovedRefs.begin(); it != cell.mMovedRefs.end(); ++it) {
                // remove reference from current leased ref tracker and add it to new cell
                ESM::MovedCellRefTracker::iterator itold = std::find(oldcell->mMovedRefs.begin(), oldcell->mMovedRefs.end(), it->mRefNum);
                if (itold != oldcell->mMovedRefs.end()) {
                    ESM::MovedCellRef target0 = *itold;
                    ESM::Cell *wipecell = const_cast<ESM::Cell*>(search(target0.mTarget[0], target0.mTarget[1]));
                    ESM::CellRefTracker::iterator it_lease = std::find(wipecell->mLeasedRefs.begin(), wipecell->mLeasedRefs.end(), it->mRefNum);
                    wipecell->mLeasedRefs.erase(it_lease);
                    *itold = *it;
                }
                ++sharedIter;
            }
            mStatic.erase(it);
        }

        return true;
    }

    template<typename T>
    bool Store<T>::erase(const std::string &id)
    {
        std::string key = Misc::StringUtils::lowerCase(id);
        typename Dynamic::iterator it = mDynamic.find(key);
        if (it == mDynamic.end()) {
            return false;
        }
        mDynamic.erase(it);

        // have to reinit the whole shared part
        assert(mShared.size() >= mStatic.size());
        mShared.erase(mShared.begin() + mStatic.size(), mShared.end());
        for (it = mDynamic.begin(); it != mDynamic.end(); ++it) {
            mShared.push_back(&it->second);
        }
        return true;
    }
    template<typename T>
    bool Store<T>::erase(const T &item)
    {
        return erase(item.mId);
    }
    template<typename T>
    void Store<T>::write (ESM::ESMWriter& writer, Loading::Listener& progress) const
    {
        for (typename Dynamic::const_iterator iter (mDynamic.begin()); iter!=mDynamic.end();
             ++iter)
        {
            writer.startRecord (T::sRecordId);
            iter->second.save (writer);
            writer.endRecord (T::sRecordId);
        }
    }
    template<typename T>
    RecordId Store<T>::read(ESM::ESMReader& reader)
    {
        T record;
        bool isDeleted = false;

        record.load (reader, isDeleted);
        insert (record);

        return RecordId(record.mId, isDeleted);
    }

    // LandTexture
    //=========================================================================
    Store<ESM::LandTexture>::Store()
    {
        mStatic.push_back(LandTextureList());
        LandTextureList &ltexl = mStatic[0];
        // More than enough to hold Morrowind.esm. Extra lists for plugins will we
        //  added on-the-fly in a different method.
        ltexl.reserve(128);
    }
    const ESM::LandTexture *Store<ESM::LandTexture>::search(size_t index, size_t plugin) const
    {
        assert(plugin < mStatic.size());
        const LandTextureList &ltexl = mStatic[plugin];

        if (index >= ltexl.size())
            return NULL;
        return &ltexl[index];
    }
    const ESM::LandTexture *Store<ESM::LandTexture>::find(size_t index, size_t plugin) const
    {
        const ESM::LandTexture *ptr = search(index, plugin);
        if (ptr == 0) {
            std::ostringstream msg;
            msg << "Land texture with index " << index << " not found";
            throw std::runtime_error(msg.str());
        }
        return ptr;
    }
    size_t Store<ESM::LandTexture>::getSize() const
    {
        return mStatic.size();
    }
    size_t Store<ESM::LandTexture>::getSize(size_t plugin) const
    {
        assert(plugin < mStatic.size());
        return mStatic[plugin].size();
    }
    RecordId Store<ESM::LandTexture>::load(ESM::ESMReader &esm, size_t plugin)
    {
        ESM::LandTexture lt;
        bool isDeleted = false;

        lt.load(esm, isDeleted);

        assert(plugin < mStatic.size());

        LandTextureList &ltexl = mStatic[plugin];
        if(lt.mIndex + 1 > (int)ltexl.size())
            ltexl.resize(lt.mIndex+1);

        // Store it
        ltexl[lt.mIndex] = lt;

        return RecordId(lt.mId, isDeleted);
    }
    RecordId Store<ESM::LandTexture>::load(ESM::ESMReader &esm)
    {
        return load(esm, esm.getIndex());
    }
    Store<ESM::LandTexture>::iterator Store<ESM::LandTexture>::begin(size_t plugin) const
    {
        assert(plugin < mStatic.size());
        return mStatic[plugin].begin();
    }
    Store<ESM::LandTexture>::iterator Store<ESM::LandTexture>::end(size_t plugin) const
    {
        assert(plugin < mStatic.size());
        return mStatic[plugin].end();
    }
    void Store<ESM::LandTexture>::resize(size_t num)
    {
        if (mStatic.size() < num)
            mStatic.resize(num);
    }
    
    // Land
    //=========================================================================
    Store<ESM::Land>::~Store()
    {
        for (std::vector<ESM::Land *>::const_iterator it =
                         mStatic.begin(); it != mStatic.end(); ++it)
        {
            delete *it;
        }

    }
    size_t Store<ESM::Land>::getSize() const
    {
        return mStatic.size();
    }
    Store<ESM::Land>::iterator Store<ESM::Land>::begin() const
    {
        return iterator(mStatic.begin());
    }
    Store<ESM::Land>::iterator Store<ESM::Land>::end() const
    {
        return iterator(mStatic.end());
    }
    ESM::Land *Store<ESM::Land>::search(int x, int y) const
    {
        ESM::Land land;
        land.mX = x, land.mY = y;

        std::vector<ESM::Land *>::const_iterator it =
            std::lower_bound(mStatic.begin(), mStatic.end(), &land, Compare());

        if (it != mStatic.end() && (*it)->mX == x && (*it)->mY == y) {
            return const_cast<ESM::Land *>(*it);
        }
        return 0;
    }
    ESM::Land *Store<ESM::Land>::find(int x, int y) const
    {
        ESM::Land *ptr = search(x, y);
        if (ptr == 0) {
            std::ostringstream msg;
            msg << "Land at (" << x << ", " << y << ") not found";
            throw std::runtime_error(msg.str());
        }
        return ptr;
    }
    RecordId Store<ESM::Land>::load(ESM::ESMReader &esm)
    {
        ESM::Land *ptr = new ESM::Land();
        bool isDeleted = false;

        ptr->load(esm, isDeleted);

        // Same area defined in multiple plugins? -> last plugin wins
        // Can't use search() because we aren't sorted yet - is there any other way to speed this up?
        for (std::vector<ESM::Land*>::iterator it = mStatic.begin(); it != mStatic.end(); ++it)
        {
            if ((*it)->mX == ptr->mX && (*it)->mY == ptr->mY)
            {
                delete *it;
                mStatic.erase(it);
                break;
            }
        }

        mStatic.push_back(ptr);

        return RecordId("", isDeleted);
    }
    void Store<ESM::Land>::setUp()
    {
        std::sort(mStatic.begin(), mStatic.end(), Compare());
    }


    // Cell
    //=========================================================================

    const ESM::Cell *Store<ESM::Cell>::search(const ESM::Cell &cell) const
    {
        if (cell.isExterior()) {
            return search(cell.getGridX(), cell.getGridY());
        }
        return search(cell.mName);
    }
    void Store<ESM::Cell>::handleMovedCellRefs(ESM::ESMReader& esm, ESM::Cell* cell)
    {
        //Handling MovedCellRefs, there is no way to do it inside loadcell
        while (esm.isNextSub("MVRF")) {
            ESM::CellRef ref;
            ESM::MovedCellRef cMRef;
            cell->getNextMVRF(esm, cMRef);

            ESM::Cell *cellAlt = const_cast<ESM::Cell*>(searchOrCreate(cMRef.mTarget[0], cMRef.mTarget[1]));

            // Get regular moved reference data. Adapted from CellStore::loadRefs. Maybe we can optimize the following
            //  implementation when the oher implementation works as well.
            bool deleted = false;
            cell->getNextRef(esm, ref, deleted);

            // Add data required to make reference appear in the correct cell.
            // We should not need to test for duplicates, as this part of the code is pre-cell merge.
            cell->mMovedRefs.push_back(cMRef);
            // But there may be duplicates here!
            if (!deleted)
            {
                ESM::CellRefTracker::iterator iter = std::find(cellAlt->mLeasedRefs.begin(), cellAlt->mLeasedRefs.end(), ref.mRefNum);
                if (iter == cellAlt->mLeasedRefs.end())
                  cellAlt->mLeasedRefs.push_back(ref);
                else
                    oldcell->mMovedRefs.push_back(*it);
            }

            // We don't need to merge mLeasedRefs of cell / oldcell. This list is filled when another cell moves a
            // reference to this cell, so the list for the new cell should be empty. The list for oldcell,
            // however, could have leased refs in it and so should be kept.
        } else
        {
            // spawn a new cell
            cell.loadCell(esm, false);

            // handle moved ref (MVRF) subrecords
            handleMovedCellRefs (esm, &cell);

            // push the new references on the list of references to manage
            cell.postLoad(esm);

            mExt[std::make_pair(cell.mData.mX, cell.mData.mY)] = cell;
        }
    }
}

void Store<ESM::LandTexture>::load(ESM::ESMReader &esm)
{
    load(esm, esm.getIndex());
}

}
