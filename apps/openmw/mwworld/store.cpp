#include "store.hpp"

#include <components/debug/debuglog.hpp>

#include <components/esm/esmreader.hpp>
#include <components/esm/esmwriter.hpp>

#include <components/loadinglistener/loadinglistener.hpp>
#include <components/misc/rng.hpp>

#include <stdexcept>

namespace
{
    template<typename T>
    class GetRecords
    {
        const std::string mFind;
        std::vector<const T*> *mRecords;

    public:
        GetRecords(const std::string &str, std::vector<const T*> *records)
          : mFind(Misc::StringUtils::lowerCase(str)), mRecords(records)
        { }

        void operator()(const T *item)
        {
            if(Misc::StringUtils::ciCompareLen(mFind, item->mId, mFind.size()) == 0)
                mRecords->push_back(item);
        }
    };

    struct Compare
    {
        bool operator()(const ESM::Land *x, const ESM::Land *y) {
            if (x->mX == y->mX) {
                return x->mY < y->mY;
            }
            return x->mX < y->mX;
        }
        bool operator()(const ESM::Land *x, const std::pair<int, int>& y) {
            if (x->mX == y.first) {
                return x->mY < y.second;
            }
            return x->mX < y.first;
        }
    };
}

namespace MWWorld
{
    RecordId::RecordId(const std::string &id, bool isDeleted)
        : mId(id), mIsDeleted(isDeleted)
    {}

    template<typename T> 
    IndexedStore<T>::IndexedStore()
    {
    }
    template<typename T>        
    typename IndexedStore<T>::iterator IndexedStore<T>::begin() const
    {
        return mStatic.begin();
    }
    template<typename T>    
    typename IndexedStore<T>::iterator IndexedStore<T>::end() const
    {
        return mStatic.end();
    }
    template<typename T>
    void IndexedStore<T>::load(ESM::ESMReader &esm)
    {
        T record;
        bool isDeleted = false;

        record.load(esm, isDeleted);

        // Try to overwrite existing record
        std::pair<typename Static::iterator, bool> ret = mStatic.insert(std::make_pair(record.mIndex, record));
        if (!ret.second)
            ret.first->second = record;
    }
    template<typename T>
    int IndexedStore<T>::getSize() const
    {
        return mStatic.size();
    }
    template<typename T>
    void IndexedStore<T>::setUp()
    {
    }
    template<typename T>
    const T *IndexedStore<T>::search(int index) const
    {
        typename Static::const_iterator it = mStatic.find(index);
        if (it != mStatic.end())
            return &(it->second);
        return nullptr;
    }
    template<typename T>
    const T *IndexedStore<T>::find(int index) const
    {
        const T *ptr = search(index);
        if (ptr == 0)
        {
            const std::string msg = T::getRecordType() + " with index " + std::to_string(index) + " not found";
            throw std::runtime_error(msg);
        }
        return ptr;
    }

    // Need to instantiate these before they're used
    template class IndexedStore<ESM::MagicEffect>;
    template class IndexedStore<ESM::Skill>;

    template<typename T>
    Store<T>::Store()
    {
    }

    template<typename T>
    Store<T>::Store(const Store<T>& orig)
        : mStatic(orig.mStatic)
    {
    }

    template<typename T>
    void Store<T>::clearDynamic()
    {
        // remove the dynamic part of mShared
        assert(mShared.size() >= mStatic.size());
        mShared.erase(mShared.begin() + mStatic.size(), mShared.end());
        mDynamic.clear();
    }

    template<typename T>
    const T *Store<T>::search(const std::string &id) const
    {
        std::string idLower = Misc::StringUtils::lowerCase(id);

        typename Dynamic::const_iterator dit = mDynamic.find(idLower);
        if (dit != mDynamic.end()) {
            return &dit->second;
        }

        typename std::map<std::string, T>::const_iterator it = mStatic.find(idLower);

        if (it != mStatic.end() && Misc::StringUtils::ciEqual(it->second.mId, id)) {
            return &(it->second);
        }

        return 0;
    }
    template<typename T>
    const T *Store<T>::searchStatic(const std::string &id) const
    {
        std::string idLower = Misc::StringUtils::lowerCase(id);
        typename std::map<std::string, T>::const_iterator it = mStatic.find(idLower);

        if (it != mStatic.end() && Misc::StringUtils::ciEqual(it->second.mId, id)) {
            return &(it->second);
        }

        return 0;
    }

    template<typename T>
    bool Store<T>::isDynamic(const std::string &id) const
    {
        typename Dynamic::const_iterator dit = mDynamic.find(id);
        return (dit != mDynamic.end());
    }
    template<typename T>
    const T *Store<T>::searchRandom(const std::string &id) const
    {
        std::vector<const T*> results;
        std::for_each(mShared.begin(), mShared.end(), GetRecords<T>(id, &results));
        if(!results.empty())
            return results[Misc::Rng::rollDice(results.size())];
        return nullptr;
    }
    template<typename T>
    const T *Store<T>::find(const std::string &id) const
    {
        const T *ptr = search(id);
        if (ptr == 0)
        {
            const std::string msg = T::getRecordType() + " '" + id + "' not found";
            throw std::runtime_error(msg);
        }
        return ptr;
    }
    template<typename T>
    const T *Store<T>::findRandom(const std::string &id) const
    {
        const T *ptr = searchRandom(id);
        if(ptr == 0)
        {
            const std::string msg = T::getRecordType() + " starting with '" + id + "' not found";
            throw std::runtime_error(msg);
        }
        return ptr;
    }
    template<typename T>
    RecordId Store<T>::load(ESM::ESMReader &esm)
    {
        T record;
        bool isDeleted = false;

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
    template<typename T>
    T *Store<T>::insert(const T &item)
    {
        std::string id = Misc::StringUtils::lowerCase(item.mId);
        std::pair<typename Dynamic::iterator, bool> result =
            mDynamic.insert(std::pair<std::string, T>(id, item));
        T *ptr = &result.first->second;
        if (result.second) {
            mShared.push_back(ptr);
        } else {
            *ptr = item;
        }
        return ptr;
    }
    template<typename T>
    T *Store<T>::insertStatic(const T &item)
    {
        std::string id = Misc::StringUtils::lowerCase(item.mId);
        std::pair<typename Static::iterator, bool> result =
            mStatic.insert(std::pair<std::string, T>(id, item));
        T *ptr = &result.first->second;
        if (result.second) {
            mShared.push_back(ptr);
        } else {
            *ptr = item;
        }
        return ptr;
    }
    template<typename T>
    bool Store<T>::eraseStatic(const std::string &id)
    {
        std::string idLower = Misc::StringUtils::lowerCase(id);

        typename std::map<std::string, T>::iterator it = mStatic.find(idLower);

        if (it != mStatic.end() && Misc::StringUtils::ciEqual(it->second.mId, id)) {
            // delete from the static part of mShared
            typename std::vector<T *>::iterator sharedIter = mShared.begin();
            typename std::vector<T *>::iterator end = sharedIter + mStatic.size();

            while (sharedIter != mShared.end() && sharedIter != end) {
                if((*sharedIter)->mId == idLower) {
                    mShared.erase(sharedIter);
                    break;
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
            return nullptr;
        return &ltexl[index];
    }
    const ESM::LandTexture *Store<ESM::LandTexture>::find(size_t index, size_t plugin) const
    {
        const ESM::LandTexture *ptr = search(index, plugin);
        if (ptr == 0)
        {
            const std::string msg = "Land texture with index " + std::to_string(index) + " not found";
            throw std::runtime_error(msg);
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

        // Replace texture for records with given ID and index from all plugins.
        for (unsigned int i=0; i<mStatic.size(); i++)
        {
            ESM::LandTexture* tex = const_cast<ESM::LandTexture*>(search(lt.mIndex, i));
            if (tex)
            {
                const std::string texId = Misc::StringUtils::lowerCase(tex->mId);
                const std::string ltId = Misc::StringUtils::lowerCase(lt.mId);
                if (texId == ltId)
                {
                    tex->mTexture = lt.mTexture;
                }
            }
        }

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
        for (const ESM::Land* staticLand : mStatic)
        {
            delete staticLand;
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
    const ESM::Land *Store<ESM::Land>::search(int x, int y) const
    {
        std::pair<int, int> comp(x,y);

        std::vector<ESM::Land *>::const_iterator it =
            std::lower_bound(mStatic.begin(), mStatic.end(), comp, Compare());

        if (it != mStatic.end() && (*it)->mX == x && (*it)->mY == y) {
            return *it;
        }
        return 0;
    }
    const ESM::Land *Store<ESM::Land>::find(int x, int y) const
    {
        const ESM::Land *ptr = search(x, y);
        if (ptr == 0)
        {
            const std::string msg = "Land at (" + std::to_string(x) + ", " + std::to_string(y) + ") not found";
            throw std::runtime_error(msg);
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
        // The land is static for given game session, there is no need to refresh it every load.
        if (mBuilt)
            return;

        std::sort(mStatic.begin(), mStatic.end(), Compare());
        mBuilt = true;
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
            ESM::CellRefTracker::iterator iter = std::find_if(cellAlt->mLeasedRefs.begin(), cellAlt->mLeasedRefs.end(), ESM::CellRefTrackerPredicate(ref.mRefNum));
            if (iter == cellAlt->mLeasedRefs.end())
                cellAlt->mLeasedRefs.push_back(std::make_pair(ref, deleted));
            else
                *iter = std::make_pair(ref, deleted);
        }
    }
    const ESM::Cell *Store<ESM::Cell>::search(const std::string &id) const
    {
        ESM::Cell cell;
        cell.mName = Misc::StringUtils::lowerCase(id);

        std::map<std::string, ESM::Cell>::const_iterator it = mInt.find(cell.mName);

        if (it != mInt.end() && Misc::StringUtils::ciEqual(it->second.mName, id)) {
            return &(it->second);
        }

        DynamicInt::const_iterator dit = mDynamicInt.find(cell.mName);
        if (dit != mDynamicInt.end()) {
            return &dit->second;
        }

        return 0;
    }
    const ESM::Cell *Store<ESM::Cell>::search(int x, int y) const
    {
        ESM::Cell cell;
        cell.mData.mX = x, cell.mData.mY = y;

        std::pair<int, int> key(x, y);
        DynamicExt::const_iterator it = mExt.find(key);
        if (it != mExt.end()) {
            return &(it->second);
        }

        DynamicExt::const_iterator dit = mDynamicExt.find(key);
        if (dit != mDynamicExt.end()) {
            return &dit->second;
        }

        return 0;
    }
    const ESM::Cell *Store<ESM::Cell>::searchStatic(int x, int y) const
    {
        ESM::Cell cell;
        cell.mData.mX = x, cell.mData.mY = y;

        std::pair<int, int> key(x, y);
        DynamicExt::const_iterator it = mExt.find(key);
        if (it != mExt.end()) {
            return &(it->second);
        }
        return 0;
    }
    const ESM::Cell *Store<ESM::Cell>::searchOrCreate(int x, int y)
    {
        std::pair<int, int> key(x, y);
        DynamicExt::const_iterator it = mExt.find(key);
        if (it != mExt.end()) {
            return &(it->second);
        }

        DynamicExt::const_iterator dit = mDynamicExt.find(key);
        if (dit != mDynamicExt.end()) {
            return &dit->second;
        }

        ESM::Cell newCell;
        newCell.mData.mX = x;
        newCell.mData.mY = y;
        newCell.mData.mFlags = ESM::Cell::HasWater;
        newCell.mAmbi.mAmbient = 0;
        newCell.mAmbi.mSunlight = 0;
        newCell.mAmbi.mFog = 0;
        newCell.mAmbi.mFogDensity = 0;
        return &mExt.insert(std::make_pair(key, newCell)).first->second;
    }
    const ESM::Cell *Store<ESM::Cell>::find(const std::string &id) const
    {
        const ESM::Cell *ptr = search(id);
        if (ptr == 0)
        {
            const std::string msg = "Cell '" + id + "' not found";
            throw std::runtime_error(msg);
        }
        return ptr;
    }
    const ESM::Cell *Store<ESM::Cell>::find(int x, int y) const
    {
        const ESM::Cell *ptr = search(x, y);
        if (ptr == 0)
        {
            const std::string msg = "Exterior at (" + std::to_string(x) + ", " + std::to_string(y) + ") not found";
            throw std::runtime_error(msg);
        }
        return ptr;
    }
    void Store<ESM::Cell>::clearDynamic()
    {
        setUp();
    }

    void Store<ESM::Cell>::setUp()
    {
        typedef DynamicExt::iterator ExtIterator;
        typedef std::map<std::string, ESM::Cell>::iterator IntIterator;

        mSharedInt.clear();
        mSharedInt.reserve(mInt.size());
        for (IntIterator it = mInt.begin(); it != mInt.end(); ++it) {
            mSharedInt.push_back(&(it->second));
        }

        mSharedExt.clear();
        mSharedExt.reserve(mExt.size());
        for (ExtIterator it = mExt.begin(); it != mExt.end(); ++it) {
            mSharedExt.push_back(&(it->second));
        }
    }
    RecordId Store<ESM::Cell>::load(ESM::ESMReader &esm)
    {
        // Don't automatically assume that a new cell must be spawned. Multiple plugins write to the same cell,
        //  and we merge all this data into one Cell object. However, we can't simply search for the cell id,
        //  as many exterior cells do not have a name. Instead, we need to search by (x,y) coordinates - and they
        //  are not available until both cells have been loaded at least partially!

        // All cells have a name record, even nameless exterior cells.
        ESM::Cell cell;
        bool isDeleted = false;

        // Load the (x,y) coordinates of the cell, if it is an exterior cell, 
        // so we can find the cell we need to merge with
        cell.loadNameAndData(esm, isDeleted);
        std::string idLower = Misc::StringUtils::lowerCase(cell.mName);

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

                mInt[idLower] = cell;
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
                    if (itold != oldcell->mMovedRefs.end())
                    {
                        if (it->mTarget[0] != itold->mTarget[0] || it->mTarget[1] != itold->mTarget[1])
                        {
                            ESM::Cell *wipecell = const_cast<ESM::Cell*>(search(itold->mTarget[0], itold->mTarget[1]));
                            ESM::CellRefTracker::iterator it_lease = std::find_if(wipecell->mLeasedRefs.begin(), wipecell->mLeasedRefs.end(), ESM::CellRefTrackerPredicate(it->mRefNum));
                            if (it_lease != wipecell->mLeasedRefs.end())
                                wipecell->mLeasedRefs.erase(it_lease);
                            else
                                Log(Debug::Error) << "Error: can't find " << it->mRefNum.mIndex << " " << it->mRefNum.mContentFile  << " in leasedRefs";
                        }
                        *itold = *it;
                    }
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

        return RecordId(cell.mName, isDeleted);
    }
    Store<ESM::Cell>::iterator Store<ESM::Cell>::intBegin() const
    {
        return iterator(mSharedInt.begin());
    }
    Store<ESM::Cell>::iterator Store<ESM::Cell>::intEnd() const
    {
        return iterator(mSharedInt.end());
    }
    Store<ESM::Cell>::iterator Store<ESM::Cell>::extBegin() const
    {
        return iterator(mSharedExt.begin());
    }
    Store<ESM::Cell>::iterator Store<ESM::Cell>::extEnd() const
    {
        return iterator(mSharedExt.end());
    }
    const ESM::Cell *Store<ESM::Cell>::searchExtByName(const std::string &id) const
    {
        const ESM::Cell *cell = nullptr;
        for (const ESM::Cell *sharedCell : mSharedExt)
        {
            if (Misc::StringUtils::ciEqual(sharedCell->mName, id))
            {
                if (cell == 0 ||
                    (sharedCell->mData.mX > cell->mData.mX) ||
                    (sharedCell->mData.mX == cell->mData.mX && sharedCell->mData.mY > cell->mData.mY))
                {
                    cell = sharedCell;
                }
            }
        }
        return cell;
    }
    const ESM::Cell *Store<ESM::Cell>::searchExtByRegion(const std::string &id) const
    {
        const ESM::Cell *cell = nullptr;
        for (const ESM::Cell *sharedCell : mSharedExt)
        {
            if (Misc::StringUtils::ciEqual(sharedCell->mRegion, id))
            {
                if (cell == nullptr ||
                    (sharedCell->mData.mX > cell->mData.mX) ||
                    (sharedCell->mData.mX == cell->mData.mX && sharedCell->mData.mY > cell->mData.mY))
                {
                    cell = sharedCell;
                }
            }
        }
        return cell;
    }
    size_t Store<ESM::Cell>::getSize() const
    {
        return mSharedInt.size() + mSharedExt.size();
    }
    size_t Store<ESM::Cell>::getExtSize() const
    {
        return mSharedExt.size();
    }
    size_t Store<ESM::Cell>::getIntSize() const
    {
        return mSharedInt.size();
    }
    void Store<ESM::Cell>::listIdentifier(std::vector<std::string> &list) const
    {
        list.reserve(list.size() + mSharedInt.size());

        for (const ESM::Cell *sharedCell : mSharedInt)
        {
            list.push_back(sharedCell->mName);
        }
    }
    ESM::Cell *Store<ESM::Cell>::insert(const ESM::Cell &cell)
    {
        if (search(cell) != 0)
        {
            const std::string cellType = (cell.isExterior()) ? "exterior" : "interior";
            throw std::runtime_error("Failed to create " + cellType + " cell");
        }
        ESM::Cell *ptr;
        if (cell.isExterior()) {
            std::pair<int, int> key(cell.getGridX(), cell.getGridY());

            // duplicate insertions are avoided by search(ESM::Cell &)
            std::pair<DynamicExt::iterator, bool> result =
                mDynamicExt.insert(std::make_pair(key, cell));

            ptr = &result.first->second;
            mSharedExt.push_back(ptr);
        } else {
            std::string key = Misc::StringUtils::lowerCase(cell.mName);

            // duplicate insertions are avoided by search(ESM::Cell &)
            std::pair<DynamicInt::iterator, bool> result =
                mDynamicInt.insert(std::make_pair(key, cell));

            ptr = &result.first->second;
            mSharedInt.push_back(ptr);
        }
        return ptr;
    }
    bool Store<ESM::Cell>::erase(const ESM::Cell &cell)
    {
        if (cell.isExterior()) {
            return erase(cell.getGridX(), cell.getGridY());
        }
        return erase(cell.mName);
    }
    bool Store<ESM::Cell>::erase(const std::string &id)
    {
        std::string key = Misc::StringUtils::lowerCase(id);
        DynamicInt::iterator it = mDynamicInt.find(key);

        if (it == mDynamicInt.end()) {
            return false;
        }
        mDynamicInt.erase(it);
        mSharedInt.erase(
            mSharedInt.begin() + mSharedInt.size(),
            mSharedInt.end()
        );

        for (it = mDynamicInt.begin(); it != mDynamicInt.end(); ++it) {
            mSharedInt.push_back(&it->second);
        }

        return true;
    }
    bool Store<ESM::Cell>::erase(int x, int y)
    {
        std::pair<int, int> key(x, y);
        DynamicExt::iterator it = mDynamicExt.find(key);

        if (it == mDynamicExt.end()) {
            return false;
        }
        mDynamicExt.erase(it);
        mSharedExt.erase(
            mSharedExt.begin() + mSharedExt.size(),
            mSharedExt.end()
        );

        for (it = mDynamicExt.begin(); it != mDynamicExt.end(); ++it) {
            mSharedExt.push_back(&it->second);
        }

        return true;
    }

    
    // Pathgrid
    //=========================================================================

    Store<ESM::Pathgrid>::Store()
        : mCells(nullptr)
    {
    }

    void Store<ESM::Pathgrid>::setCells(Store<ESM::Cell>& cells)
    {
        mCells = &cells;
    }
    RecordId Store<ESM::Pathgrid>::load(ESM::ESMReader &esm)
    {
        ESM::Pathgrid pathgrid;
        bool isDeleted = false;

        pathgrid.load(esm, isDeleted);

        // Unfortunately the Pathgrid record model does not specify whether the pathgrid belongs to an interior or exterior cell.
        // For interior cells, mCell is the cell name, but for exterior cells it is either the cell name or if that doesn't exist, the cell's region name.
        // mX and mY will be (0,0) for interior cells, but there is also an exterior cell with the coordinates of (0,0), so that doesn't help.
        // Check whether mCell is an interior cell. This isn't perfect, will break if a Region with the same name as an interior cell is created.
        // A proper fix should be made for future versions of the file format.
        bool interior = mCells->search(pathgrid.mCell) != nullptr;

        // Try to overwrite existing record
        if (interior)
        {
            std::pair<Interior::iterator, bool> ret = mInt.insert(std::make_pair(pathgrid.mCell, pathgrid));
            if (!ret.second)
                ret.first->second = pathgrid;
        }
        else
        {
            std::pair<Exterior::iterator, bool> ret = mExt.insert(std::make_pair(std::make_pair(pathgrid.mData.mX, pathgrid.mData.mY), pathgrid));
            if (!ret.second)
                ret.first->second = pathgrid;
        }

        return RecordId("", isDeleted);
    }
    size_t Store<ESM::Pathgrid>::getSize() const
    {
        return mInt.size() + mExt.size();
    }
    void Store<ESM::Pathgrid>::setUp()
    {
    }
    const ESM::Pathgrid *Store<ESM::Pathgrid>::search(int x, int y) const
    {
        Exterior::const_iterator it = mExt.find(std::make_pair(x,y));
        if (it != mExt.end())
            return &(it->second);
        return nullptr;
    }
    const ESM::Pathgrid *Store<ESM::Pathgrid>::search(const std::string& name) const
    {
        Interior::const_iterator it = mInt.find(name);
        if (it != mInt.end())
            return &(it->second);
        return nullptr;
    }
    const ESM::Pathgrid *Store<ESM::Pathgrid>::find(int x, int y) const
    {
        const ESM::Pathgrid* pathgrid = search(x,y);
        if (!pathgrid)
        {
            const std::string msg = "Pathgrid in cell '" + std::to_string(x) + " " + std::to_string(y) + "' not found";
            throw std::runtime_error(msg);
        }
        return pathgrid;
    }
    const ESM::Pathgrid* Store<ESM::Pathgrid>::find(const std::string& name) const
    {
        const ESM::Pathgrid* pathgrid = search(name);
        if (!pathgrid)
        {
            const std::string msg = "Pathgrid in cell '" + name + "' not found";
            throw std::runtime_error(msg);
        }
        return pathgrid;
    }
    const ESM::Pathgrid *Store<ESM::Pathgrid>::search(const ESM::Cell &cell) const
    {
        if (!(cell.mData.mFlags & ESM::Cell::Interior))
            return search(cell.mData.mX, cell.mData.mY);
        else
            return search(cell.mName);
    }
    const ESM::Pathgrid *Store<ESM::Pathgrid>::find(const ESM::Cell &cell) const
    {
        if (!(cell.mData.mFlags & ESM::Cell::Interior))
            return find(cell.mData.mX, cell.mData.mY);
        else
            return find(cell.mName);
    }


    // Skill
    //=========================================================================
    
    Store<ESM::Skill>::Store()
    {
    }


    // Magic effect
    //=========================================================================

    Store<ESM::MagicEffect>::Store()
    {
    }


    // Attribute
    //=========================================================================

    Store<ESM::Attribute>::Store()
    {
        mStatic.reserve(ESM::Attribute::Length);
    }
    const ESM::Attribute *Store<ESM::Attribute>::search(size_t index) const
    {
        if (index >= mStatic.size()) {
            return 0;
        }
        return &mStatic.at(index);
    }

    const ESM::Attribute *Store<ESM::Attribute>::find(size_t index) const
    {
        const ESM::Attribute *ptr = search(index);
        if (ptr == 0)
        {
            const std::string msg = "Attribute with index " + std::to_string(index) + " not found";
            throw std::runtime_error(msg);
        }
        return ptr;
    }
    void Store<ESM::Attribute>::setUp()
    {
        for (int i = 0; i < ESM::Attribute::Length; ++i) 
        {
            ESM::Attribute newAttribute;
            newAttribute.mId = ESM::Attribute::sAttributeIds[i];
            newAttribute.mName = ESM::Attribute::sGmstAttributeIds[i];
            newAttribute.mDescription = ESM::Attribute::sGmstAttributeDescIds[i];
            mStatic.push_back(newAttribute);
        }
    }
    size_t Store<ESM::Attribute>::getSize() const
    {
        return mStatic.size();
    }
    Store<ESM::Attribute>::iterator Store<ESM::Attribute>::begin() const
    {
        return mStatic.begin();
    }
    Store<ESM::Attribute>::iterator Store<ESM::Attribute>::end() const
    {
        return mStatic.end();
    }

    
    // Dialogue
    //=========================================================================


    template<>
    void Store<ESM::Dialogue>::setUp()
    {
        // DialInfos marked as deleted are kept during the loading phase, so that the linked list
        // structure is kept intact for inserting further INFOs. Delete them now that loading is done.
        for (Static::iterator it = mStatic.begin(); it != mStatic.end(); ++it)
        {
            ESM::Dialogue& dial = it->second;
            dial.clearDeletedInfos();
        }

        mShared.clear();
        mShared.reserve(mStatic.size());
        std::map<std::string, ESM::Dialogue>::iterator it = mStatic.begin();
        for (; it != mStatic.end(); ++it) {
            mShared.push_back(&(it->second));
        }
    }

    template <>
    inline RecordId Store<ESM::Dialogue>::load(ESM::ESMReader &esm) {
        // The original letter case of a dialogue ID is saved, because it's printed
        ESM::Dialogue dialogue;
        bool isDeleted = false;

        dialogue.loadId(esm);

        std::string idLower = Misc::StringUtils::lowerCase(dialogue.mId);
        std::map<std::string, ESM::Dialogue>::iterator found = mStatic.find(idLower);
        if (found == mStatic.end())
        {
            dialogue.loadData(esm, isDeleted);
            mStatic.insert(std::make_pair(idLower, dialogue));
        }
        else
        {
            found->second.loadData(esm, isDeleted);
            dialogue = found->second;
        }

        return RecordId(dialogue.mId, isDeleted);
    }

    template<>
    bool Store<ESM::Dialogue>::eraseStatic(const std::string &id)
    {
        auto it = mStatic.find(Misc::StringUtils::lowerCase(id));

        if (it != mStatic.end() && Misc::StringUtils::ciEqual(it->second.mId, id)) {
            mStatic.erase(it);
        }

        return true;
    }

}

template class MWWorld::Store<ESM::Activator>;
template class MWWorld::Store<ESM::Apparatus>;
template class MWWorld::Store<ESM::Armor>;
//template class MWWorld::Store<ESM::Attribute>;
template class MWWorld::Store<ESM::BirthSign>;
template class MWWorld::Store<ESM::BodyPart>;
template class MWWorld::Store<ESM::Book>;
//template class MWWorld::Store<ESM::Cell>;
template class MWWorld::Store<ESM::Class>;
template class MWWorld::Store<ESM::Clothing>;
template class MWWorld::Store<ESM::Container>;
template class MWWorld::Store<ESM::Creature>;
template class MWWorld::Store<ESM::CreatureLevList>;
template class MWWorld::Store<ESM::Dialogue>;
template class MWWorld::Store<ESM::Door>;
template class MWWorld::Store<ESM::Enchantment>;
template class MWWorld::Store<ESM::Faction>;
template class MWWorld::Store<ESM::GameSetting>;
template class MWWorld::Store<ESM::Global>;
template class MWWorld::Store<ESM::Ingredient>;
template class MWWorld::Store<ESM::ItemLevList>;
//template class MWWorld::Store<ESM::Land>;
//template class MWWorld::Store<ESM::LandTexture>;
template class MWWorld::Store<ESM::Light>;
template class MWWorld::Store<ESM::Lockpick>;
//template class MWWorld::Store<ESM::MagicEffect>;
template class MWWorld::Store<ESM::Miscellaneous>;
template class MWWorld::Store<ESM::NPC>;
//template class MWWorld::Store<ESM::Pathgrid>;
template class MWWorld::Store<ESM::Potion>;
template class MWWorld::Store<ESM::Probe>;
template class MWWorld::Store<ESM::Race>;
template class MWWorld::Store<ESM::Region>;
template class MWWorld::Store<ESM::Repair>;
template class MWWorld::Store<ESM::Script>;
//template class MWWorld::Store<ESM::Skill>;
template class MWWorld::Store<ESM::Sound>;
template class MWWorld::Store<ESM::SoundGenerator>;
template class MWWorld::Store<ESM::Spell>;
template class MWWorld::Store<ESM::StartScript>;
template class MWWorld::Store<ESM::Static>;
template class MWWorld::Store<ESM::Weapon>;

