#include "store.hpp"

#include <iterator>
#include <sstream>
#include <stdexcept>

#include <components/debug/debuglog.hpp>

#include <components/esm/records.hpp>
#include <components/esm3/esmreader.hpp>
#include <components/esm3/esmwriter.hpp>

#include <components/fallback/fallback.hpp>
#include <components/loadinglistener/loadinglistener.hpp>
#include <components/misc/rng.hpp>

#include "../mwworld/cell.hpp"

namespace
{
    // TODO: Switch to C++23 to get a working version of std::unordered_map::erase
    template <class T, class Id>
    bool eraseFromMap(T& map, const Id& value)
    {
        auto it = map.find(value);
        if (it != map.end())
        {
            map.erase(it);
            return true;
        }
        return false;
    }

    std::string_view getGMSTString(const MWWorld::Store<ESM::GameSetting>& settings, std::string_view id)
    {
        const ESM::GameSetting* setting = settings.search(id);
        if (setting && setting->mValue.getType() == ESM::VT_String)
            return setting->mValue.getString();
        return id;
    }

    float getGMSTFloat(const MWWorld::Store<ESM::GameSetting>& settings, std::string_view id)
    {
        const ESM::GameSetting* setting = settings.search(id);
        if (setting && (setting->mValue.getType() == ESM::VT_Float || setting->mValue.getType() == ESM::VT_Int))
            return setting->mValue.getFloat();
        return {};
    }
}

namespace MWWorld
{
    RecordId::RecordId(const ESM::RefId& id, bool isDeleted)
        : mId(id)
        , mIsDeleted(isDeleted)
    {
    }

    template <typename T>
    IndexedStore<T>::IndexedStore()
    {
    }
    template <typename T>
    typename IndexedStore<T>::iterator IndexedStore<T>::begin() const
    {
        return mStatic.begin();
    }
    template <typename T>
    typename IndexedStore<T>::iterator IndexedStore<T>::end() const
    {
        return mStatic.end();
    }
    template <typename T>
    void IndexedStore<T>::load(ESM::ESMReader& esm)
    {
        T record;
        bool isDeleted = false;

        record.load(esm, isDeleted);
        auto idx = record.mIndex;
        mStatic.insert_or_assign(idx, std::move(record));
    }
    template <typename T>
    size_t IndexedStore<T>::getSize() const
    {
        return mStatic.size();
    }
    template <typename T>
    void IndexedStore<T>::setUp()
    {
    }
    template <typename T>
    const T* IndexedStore<T>::search(int index) const
    {
        typename Static::const_iterator it = mStatic.find(index);
        if (it != mStatic.end())
            return &(it->second);
        return nullptr;
    }
    template <typename T>
    const T* IndexedStore<T>::find(int index) const
    {
        const T* ptr = search(index);
        if (ptr == nullptr)
        {
            std::stringstream msg;
            msg << T::getRecordType() << " with index " << index << " not found";
            throw std::runtime_error(msg.str());
        }
        return ptr;
    }

    // Need to instantiate these before they're used
    template class IndexedStore<ESM::MagicEffect>;

    template <class T, class Id>
    TypedDynamicStore<T, Id>::TypedDynamicStore()
    {
    }

    template <class T, class Id>
    TypedDynamicStore<T, Id>::TypedDynamicStore(const TypedDynamicStore<T, Id>& orig)
        : mStatic(orig.mStatic)
    {
    }

    template <class T, class Id>
    void TypedDynamicStore<T, Id>::clearDynamic()
    {
        // remove the dynamic part of mShared
        assert(mShared.size() >= mStatic.size());
        mShared.erase(mShared.begin() + mStatic.size(), mShared.end());
        mDynamic.clear();
    }

    template <class T, class Id>
    const T* TypedDynamicStore<T, Id>::search(const Id& id) const
    {
        typename Dynamic::const_iterator dit = mDynamic.find(id);
        if (dit != mDynamic.end())
            return &dit->second;

        typename Static::const_iterator it = mStatic.find(id);
        if (it != mStatic.end())
            return &(it->second);

        return nullptr;
    }
    template <class T, class Id>
    const T* TypedDynamicStore<T, Id>::searchStatic(const Id& id) const
    {
        typename Static::const_iterator it = mStatic.find(id);
        if (it != mStatic.end())
            return &(it->second);

        return nullptr;
    }

    template <class T, class Id>
    bool TypedDynamicStore<T, Id>::isDynamic(const Id& id) const
    {
        typename Dynamic::const_iterator dit = mDynamic.find(id);
        return (dit != mDynamic.end());
    }
    template <class T, class Id>
    const T* TypedDynamicStore<T, Id>::searchRandom(const std::string_view prefix, Misc::Rng::Generator& prng) const
    {
        if constexpr (std::is_same_v<Id, ESM::RefId>)
        {
            if (prefix.empty())
            {
                if (!mShared.empty())
                    return mShared[Misc::Rng::rollDice(mShared.size(), prng)];
            }
            else if constexpr (!std::is_same_v<decltype(T::mId), ESM::FormId>)
            {
                std::vector<const T*> results;
                std::copy_if(mShared.begin(), mShared.end(), std::back_inserter(results),
                    [prefix](const T* item) { return item->mId.startsWith(prefix); });
                if (!results.empty())
                    return results[Misc::Rng::rollDice(results.size(), prng)];
            }
            return nullptr;
        }
        else
            throw std::runtime_error("Store<T>::searchRandom is supported only if Id is ESM::RefId");
    }
    template <class T, class Id>
    const T* TypedDynamicStore<T, Id>::find(const Id& id) const
    {
        const T* ptr = search(id);
        if (ptr == nullptr)
        {
            std::stringstream msg;
            if constexpr (!ESM::isESM4Rec(T::sRecordId))
            {
                msg << T::getRecordType();
            }
            else
            {
                msg << "ESM::REC_" << getRecNameString(T::sRecordId).toStringView();
            }
            msg << " '" << id << "' not found";
            throw std::runtime_error(msg.str());
        }
        return ptr;
    }
    template <class T, class Id>
    RecordId TypedDynamicStore<T, Id>::load(ESM::ESMReader& esm)
    {
        if constexpr (!ESM::isESM4Rec(T::sRecordId))
        {
            T record;
            bool isDeleted = false;
            record.load(esm, isDeleted);

            std::pair<typename Static::iterator, bool> inserted = mStatic.insert_or_assign(record.mId, record);
            if (inserted.second)
                mShared.push_back(&inserted.first->second);

            if constexpr (std::is_same_v<Id, ESM::RefId>)
                return RecordId(record.mId, isDeleted);
            else
                return RecordId();
        }
        else
        {
            std::stringstream msg;
            msg << "Can not load record of type ESM::REC_" << getRecNameString(T::sRecordId).toStringView()
                << ": ESM::ESMReader can load only ESM3 records.";
            throw std::runtime_error(msg.str());
        }
    }

    template <class T, class Id>
    void TypedDynamicStore<T, Id>::setUp()
    {
    }

    template <class T, class Id>
    typename TypedDynamicStore<T, Id>::iterator TypedDynamicStore<T, Id>::begin() const
    {
        return mShared.begin();
    }
    template <class T, class Id>
    typename TypedDynamicStore<T, Id>::iterator TypedDynamicStore<T, Id>::end() const
    {
        return mShared.end();
    }

    template <class T, class Id>
    size_t TypedDynamicStore<T, Id>::getSize() const
    {
        return mShared.size();
    }

    template <class T, class Id>
    size_t TypedDynamicStore<T, Id>::getDynamicSize() const
    {
        return mDynamic.size();
    }
    template <class T, class Id>
    void TypedDynamicStore<T, Id>::listIdentifier(std::vector<Id>& list) const
    {
        list.reserve(list.size() + getSize());
        typename std::vector<T*>::const_iterator it = mShared.begin();
        for (; it != mShared.end(); ++it)
        {
            list.push_back((*it)->mId);
        }
    }

    template <class T, class Id>
    T* TypedDynamicStore<T, Id>::insert(const T& item, bool overrideOnly)
    {
        if constexpr (std::is_same_v<decltype(item.mId), ESM::RefId>)
            overrideOnly = overrideOnly && !item.mId.template is<ESM::GeneratedRefId>();
        if (overrideOnly)
        {
            auto it = mStatic.find(item.mId);
            if (it == mStatic.end())
                return nullptr;
        }
        std::pair<typename Dynamic::iterator, bool> result = mDynamic.insert_or_assign(item.mId, item);
        T* ptr = &result.first->second;
        if (result.second)
            mShared.push_back(ptr);
        return ptr;
    }
    template <class T, class Id>
    T* TypedDynamicStore<T, Id>::insertStatic(const T& item)
    {
        std::pair<typename Static::iterator, bool> result = mStatic.insert_or_assign(item.mId, item);
        T* ptr = &result.first->second;
        if (result.second)
            mShared.push_back(ptr);
        return ptr;
    }
    template <class T, class Id>
    bool TypedDynamicStore<T, Id>::eraseStatic(const Id& id)
    {
        typename Static::iterator it = mStatic.find(id);

        if (it != mStatic.end())
        {
            // delete from the static part of mShared
            typename std::vector<T*>::iterator sharedIter = mShared.begin();
            typename std::vector<T*>::iterator end = sharedIter + mStatic.size();

            while (sharedIter != mShared.end() && sharedIter != end)
            {
                if ((*sharedIter)->mId == id)
                {
                    mShared.erase(sharedIter);
                    break;
                }
                ++sharedIter;
            }
            mStatic.erase(it);
        }

        return true;
    }

    template <class T, class Id>
    bool TypedDynamicStore<T, Id>::erase(const Id& id)
    {
        if (!eraseFromMap(mDynamic, id))
            return false;

        // have to reinit the whole shared part
        assert(mShared.size() >= mStatic.size());
        mShared.erase(mShared.begin() + mStatic.size(), mShared.end());
        for (auto it = mDynamic.begin(); it != mDynamic.end(); ++it)
        {
            mShared.push_back(&it->second);
        }
        return true;
    }
    template <class T, class Id>
    bool TypedDynamicStore<T, Id>::erase(const T& item)
    {
        return erase(item.mId);
    }
    template <class T, class Id>
    void TypedDynamicStore<T, Id>::write(ESM::ESMWriter& writer, Loading::Listener& progress) const
    {
        for (typename Dynamic::const_iterator iter(mDynamic.begin()); iter != mDynamic.end(); ++iter)
        {
            if constexpr (!ESM::isESM4Rec(T::sRecordId))
            {
                writer.startRecord(T::sRecordId);
                iter->second.save(writer);
                writer.endRecord(T::sRecordId);
            }
        }
    }
    template <class T, class Id>
    RecordId TypedDynamicStore<T, Id>::read(ESM::ESMReader& reader, bool overrideOnly)
    {
        if constexpr (!ESM::isESM4Rec(T::sRecordId))
        {
            T record;
            bool isDeleted = false;
            record.load(reader, isDeleted);

            insert(record, overrideOnly);

            if constexpr (std::is_same_v<Id, ESM::RefId>)
                return RecordId(record.mId, isDeleted);
            else
                return RecordId();
        }
        else
        {
            std::stringstream msg;
            msg << "Can not load record of type ESM::REC_" << getRecNameString(T::sRecordId).toStringView()
                << ": ESM::ESMReader can load only ESM3 records.";
            throw std::runtime_error(msg.str());
        }
    }

    // LandTexture
    //=========================================================================
    Store<ESM::LandTexture>::Store() = default;

    const std::string* Store<ESM::LandTexture>::search(std::uint32_t index, int plugin) const
    {
        auto mapping = mMappings.find(PluginIndex{ plugin, index });
        if (mapping == mMappings.end())
            return nullptr;
        auto texture = mStatic.find(mapping->second);
        if (texture == mStatic.end())
            return nullptr;
        return &texture->second;
    }

    size_t Store<ESM::LandTexture>::getSize() const
    {
        return mStatic.size();
    }

    RecordId Store<ESM::LandTexture>::load(ESM::ESMReader& esm)
    {
        const int plugin = esm.getIndex();

        ESM::LandTexture lt;
        bool isDeleted = false;

        lt.load(esm, isDeleted);

        if (!isDeleted)
        {
            mStatic[lt.mId] = std::move(lt.mTexture);
            mMappings.emplace(PluginIndex{ plugin, lt.mIndex }, lt.mId);
        }

        return RecordId(lt.mId, isDeleted);
    }

    bool Store<ESM::LandTexture>::eraseStatic(const ESM::RefId& id)
    {
        mStatic.erase(id);
        return true;
    }

    // Land
    //=========================================================================
    Store<ESM::Land>::~Store() = default;
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
    const ESM::Land* Store<ESM::Land>::search(int x, int y) const
    {
        std::pair<int, int> comp(x, y);
        if (auto it = mStatic.find(comp); it != mStatic.end() && it->mX == x && it->mY == y)
            return &*it;
        return nullptr;
    }
    const ESM::Land* Store<ESM::Land>::find(int x, int y) const
    {
        const ESM::Land* ptr = search(x, y);
        if (ptr == nullptr)
        {
            const std::string msg = "Land at (" + std::to_string(x) + ", " + std::to_string(y) + ") not found";
            throw std::runtime_error(msg);
        }
        return ptr;
    }
    RecordId Store<ESM::Land>::load(ESM::ESMReader& esm)
    {
        ESM::Land land;
        bool isDeleted = false;

        land.load(esm, isDeleted);

        // Same area defined in multiple plugins? -> last plugin wins
        auto it = mStatic.lower_bound(land);
        if (it != mStatic.end() && (std::tie(it->mX, it->mY) == std::tie(land.mX, land.mY)))
        {
            auto nh = mStatic.extract(it);
            nh.value() = std::move(land);
            mStatic.insert(std::move(nh));
        }
        else
            mStatic.insert(it, std::move(land));

        return RecordId(ESM::RefId(), isDeleted);
    }
    void Store<ESM::Land>::setUp()
    {
        // The land is static for given game session, there is no need to refresh it every load.
        if (mBuilt)
            throw std::logic_error("Store<ESM::Land>::setUp() is called twice");

        mBuilt = true;
    }

    // Cell
    //=========================================================================

    const ESM::Cell* Store<ESM::Cell>::search(const ESM::RefId& cellId) const
    {
        auto foundCellIt = mCells.find(cellId);
        if (foundCellIt != mCells.end())
            return &foundCellIt->second;
        return nullptr;
    }

    const ESM::Cell* Store<ESM::Cell>::search(const ESM::Cell& cell) const
    {
        return search(cell.mId);
    }

    // this method *must* be called right after esm3.loadCell()
    void Store<ESM::Cell>::handleMovedCellRefs(ESM::ESMReader& esm, ESM::Cell* cell)
    {
        ESM::CellRef ref;
        ESM::MovedCellRef cMRef;
        bool deleted = false;
        bool moved = false;

        ESM::ESM_Context ctx = esm.getContext();

        // Handling MovedCellRefs, there is no way to do it inside loadcell
        // TODO: verify above comment
        //
        // Get regular moved reference data. Adapted from CellStore::loadRefs. Maybe we can optimize the following
        //  implementation when the oher implementation works as well.
        while (ESM::Cell::getNextRef(esm, ref, deleted, cMRef, moved, ESM::Cell::GetNextRefMode::LoadOnlyMoved))
        {
            if (!moved)
                continue;

            ESM::Cell* cellAlt = const_cast<ESM::Cell*>(searchOrCreate(cMRef.mTarget[0], cMRef.mTarget[1]));

            // Add data required to make reference appear in the correct cell.
            // We should not need to test for duplicates, as this part of the code is pre-cell merge.
            cell->mMovedRefs.push_back(cMRef);

            // But there may be duplicates here!
            ESM::CellRefTracker::iterator iter = std::find_if(
                cellAlt->mLeasedRefs.begin(), cellAlt->mLeasedRefs.end(), ESM::CellRefTrackerPredicate(ref.mRefNum));
            if (iter == cellAlt->mLeasedRefs.end())
                cellAlt->mLeasedRefs.emplace_back(std::move(ref), deleted);
            else
                *iter = std::make_pair(std::move(ref), deleted);

            cMRef.mRefNum.mIndex = 0;
        }

        esm.restoreContext(ctx);
    }
    const ESM::Cell* Store<ESM::Cell>::search(std::string_view name) const
    {
        DynamicInt::const_iterator it = mInt.find(name);
        if (it != mInt.end())
        {
            return it->second;
        }

        DynamicInt::const_iterator dit = mDynamicInt.find(name);
        if (dit != mDynamicInt.end())
        {
            return dit->second;
        }

        return nullptr;
    }
    const ESM::Cell* Store<ESM::Cell>::search(int x, int y) const
    {
        std::pair<int, int> key(x, y);
        DynamicExt::const_iterator it = mExt.find(key);
        if (it != mExt.end())
            return it->second;

        DynamicExt::const_iterator dit = mDynamicExt.find(key);
        if (dit != mDynamicExt.end())
            return dit->second;

        return nullptr;
    }
    const ESM::Cell* Store<ESM::Cell>::searchStatic(int x, int y) const
    {
        DynamicExt::const_iterator it = mExt.find(std::make_pair(x, y));
        if (it != mExt.end())
            return (it->second);
        return nullptr;
    }
    const ESM::Cell* Store<ESM::Cell>::searchOrCreate(int x, int y)
    {
        std::pair<int, int> key(x, y);
        DynamicExt::const_iterator it = mExt.find(key);
        if (it != mExt.end())
            return (it->second);

        DynamicExt::const_iterator dit = mDynamicExt.find(key);
        if (dit != mDynamicExt.end())
            return dit->second;

        ESM::Cell newCell;
        newCell.mData.mX = x;
        newCell.mData.mY = y;
        newCell.mData.mFlags = ESM::Cell::HasWater;
        newCell.mAmbi.mAmbient = 0;
        newCell.mAmbi.mSunlight = 0;
        newCell.mAmbi.mFog = 0;
        newCell.mAmbi.mFogDensity = 0;
        newCell.updateId();

        ESM::Cell* newCellInserted = &mCells.emplace(newCell.mId, newCell).first->second;
        mExt.emplace(key, newCellInserted);
        mSharedExt.emplace_back(newCellInserted);
        return newCellInserted;
    }
    const ESM::Cell* Store<ESM::Cell>::find(const ESM::RefId& id) const
    {
        const ESM::Cell* ptr = search(id);
        if (ptr == nullptr)
        {
            const std::string msg = "Cell " + id.toDebugString() + " not found";
            throw std::runtime_error(msg);
        }
        return ptr;
    }
    const ESM::Cell* Store<ESM::Cell>::find(std::string_view id) const
    {
        const ESM::Cell* ptr = search(id);
        if (ptr == nullptr)
        {
            const std::string msg = "Cell '" + std::string(id) + "' not found";
            throw std::runtime_error(msg);
        }
        return ptr;
    }
    const ESM::Cell* Store<ESM::Cell>::find(int x, int y) const
    {
        const ESM::Cell* ptr = search(x, y);
        if (ptr == nullptr)
        {
            const std::string msg = "Exterior at (" + std::to_string(x) + ", " + std::to_string(y) + ") not found";
            throw std::runtime_error(msg);
        }
        return ptr;
    }
    void Store<ESM::Cell>::clearDynamic()
    {
        for (const auto& [_, cell] : mDynamicExt)
            mCells.erase(cell->mId);
        mDynamicExt.clear();
        for (const auto& [_, cell] : mDynamicInt)
            mCells.erase(cell->mId);
        mDynamicInt.clear();

        mSharedInt.erase(mSharedInt.begin() + mInt.size(), mSharedInt.end());
        mSharedExt.erase(mSharedExt.begin() + mExt.size(), mSharedExt.end());
    }
    RecordId Store<ESM::Cell>::load(ESM::ESMReader& esm)
    {
        // Don't automatically assume that a new cell must be spawned. Multiple plugins write to the same cell,
        //  and we merge all this data into one Cell object. However, we can't simply search for the cell id,
        //  as many exterior cells do not have a name. Instead, we need to search by (x,y) coordinates - and they
        //  are not available until both cells have been loaded at least partially!

        // All cells have a name record, even nameless exterior cells.
        ESM::Cell* emplacedCell = nullptr;
        bool isDeleted = false;
        bool newCell = false;

        {
            ESM::Cell cellToLoad;
            cellToLoad.loadNameAndData(esm, isDeleted);
            auto [it, inserted] = mCells.insert(std::make_pair(cellToLoad.mId, cellToLoad));
            emplacedCell = &it->second;
            if (!inserted)
            {
                emplacedCell->mData = cellToLoad.mData;
                emplacedCell->mName = cellToLoad.mName;
            }
            newCell = inserted;
        }
        ESM::Cell& cell = *emplacedCell;
        // Load the (x,y) coordinates of the cell, if it is an exterior cell,
        // so we can find the cell we need to merge with
        if (cell.mData.mFlags & ESM::Cell::Interior)
        {
            cell.loadCell(esm, true);
            if (newCell)
            {
                mInt[cell.mName] = &cell;
                mSharedInt.push_back(&cell);
            }
        }
        else
        {
            cell.loadCell(esm, false);
            // handle moved ref (MVRF) subrecords
            ESM::MovedCellRefTracker newMovedRefs;
            std::swap(newMovedRefs, cell.mMovedRefs);
            handleMovedCellRefs(esm, &cell);
            std::swap(newMovedRefs, cell.mMovedRefs);
            // push the new references on the list of references to manage
            cell.postLoad(esm);
            if (newCell)
            {
                mExt[std::make_pair(cell.mData.mX, cell.mData.mY)] = &cell;
                mSharedExt.push_back(&cell);
            }
            else
            {
                // merge lists of leased references, use newer data in case of conflict
                for (const auto& movedRef : newMovedRefs)
                {
                    // remove reference from current leased ref tracker and add it to new cell
                    auto itOld = std::find(cell.mMovedRefs.begin(), cell.mMovedRefs.end(), movedRef.mRefNum);
                    if (itOld != cell.mMovedRefs.end())
                    {
                        if (movedRef.mTarget[0] != itOld->mTarget[0] || movedRef.mTarget[1] != itOld->mTarget[1])
                        {
                            ESM::Cell* wipecell = const_cast<ESM::Cell*>(search(itOld->mTarget[0], itOld->mTarget[1]));
                            auto itLease = std::find_if(wipecell->mLeasedRefs.begin(), wipecell->mLeasedRefs.end(),
                                ESM::CellRefTrackerPredicate(movedRef.mRefNum));
                            if (itLease != wipecell->mLeasedRefs.end())
                                wipecell->mLeasedRefs.erase(itLease);
                            else
                                Log(Debug::Error) << "Error: can't find " << movedRef.mRefNum.mIndex << " "
                                                  << movedRef.mRefNum.mContentFile << " in leasedRefs";
                        }
                        *itOld = movedRef;
                    }
                    else
                        cell.mMovedRefs.push_back(movedRef);
                }

                // We don't need to merge mLeasedRefs of cell / oldcell. This list is filled when another cell moves a
                // reference to this cell, so the list for the new cell should be empty. The list for oldcell,
                // however, could have leased refs in it and so should be kept.
            }
        }

        return RecordId(cell.mId, isDeleted);
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
    void Store<ESM::Cell>::listIdentifier(std::vector<ESM::RefId>& list) const
    {
        list.reserve(list.size() + mSharedInt.size());

        for (const ESM::Cell* sharedCell : mSharedInt)
        {
            list.push_back(ESM::RefId::stringRefId(sharedCell->mName));
        }
    }
    ESM::Cell* Store<ESM::Cell>::insert(const ESM::Cell& cell)
    {
        if (search(cell) != nullptr)
        {
            const std::string cellType = (cell.isExterior()) ? "exterior" : "interior";
            throw std::runtime_error("Failed to create " + cellType + " cell");
        }
        ESM::Cell* insertedCell = &mCells.emplace(cell.mId, cell).first->second;
        if (cell.isExterior())
        {
            std::pair<int, int> key(cell.getGridX(), cell.getGridY());

            // duplicate insertions are avoided by search(ESM::Cell &)
            DynamicExt::iterator result = mDynamicExt.emplace(key, insertedCell).first;
            mSharedExt.push_back(result->second);
            return result->second;
        }
        else
        {
            // duplicate insertions are avoided by search(ESM::Cell &)
            DynamicInt::iterator result = mDynamicInt.emplace(cell.mName, insertedCell).first;
            mSharedInt.push_back(result->second);
            return result->second;
        }
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
    RecordId Store<ESM::Pathgrid>::load(ESM::ESMReader& esm)
    {
        ESM::Pathgrid pathgrid;
        bool isDeleted = false;

        pathgrid.load(esm, isDeleted);

        // Unfortunately the Pathgrid record model does not specify whether the pathgrid belongs to an interior or
        // exterior cell. For interior cells, mCell is the cell name, but for exterior cells it is either the cell name
        // or if that doesn't exist, the cell's region name. mX and mY will be (0,0) for interior cells, but there is
        // also an exterior cell with the coordinates of (0,0), so that doesn't help. Check whether mCell is an interior
        // cell. This isn't perfect, will break if a Region with the same name as an interior cell is created. A proper
        // fix should be made for future versions of the file format.
        bool interior = pathgrid.mData.mX == 0 && pathgrid.mData.mY == 0
            && mCells->search(pathgrid.mCell.getRefIdString()) != nullptr;

        ESM::RefId cell
            = interior ? pathgrid.mCell : ESM::RefId::esm3ExteriorCell(pathgrid.mData.mX, pathgrid.mData.mY);
        // deal with mods that have empty pathgrid records (Issue #6209)
        // we assume that these records are empty on purpose (i.e. to remove old pathgrid on an updated cell)
        if (isDeleted || pathgrid.mPoints.empty() || pathgrid.mEdges.empty())
        {
            mStatic.erase(cell);

            return RecordId(ESM::RefId(), isDeleted);
        }

        // Try to overwrite existing record
        auto ret = mStatic.emplace(cell, pathgrid);
        if (!ret.second)
            ret.first->second = std::move(pathgrid);

        return RecordId(ESM::RefId(), isDeleted);
    }
    size_t Store<ESM::Pathgrid>::getSize() const
    {
        return mStatic.size();
    }
    void Store<ESM::Pathgrid>::setUp() {}
    const ESM::Pathgrid* Store<ESM::Pathgrid>::search(const ESM::RefId& name) const
    {
        auto it = mStatic.find(name);
        if (it != mStatic.end())
            return &(it->second);
        return nullptr;
    }
    const ESM::Pathgrid* Store<ESM::Pathgrid>::find(const ESM::RefId& name) const
    {
        const ESM::Pathgrid* pathgrid = search(name);
        if (pathgrid == nullptr)
            throw std::runtime_error("Pathgrid in cell " + name.toDebugString() + " is not found");
        return pathgrid;
    }
    const ESM::Pathgrid* Store<ESM::Pathgrid>::search(const ESM::Cell& cell) const
    {
        return search(cell.mId);
    }
    const ESM::Pathgrid* Store<ESM::Pathgrid>::search(const MWWorld::Cell& cellVariant) const
    {
        return ESM::visit(ESM::VisitOverload{
                              [&](const ESM::Cell& cell) { return search(cell); },
                              [&](const ESM4::Cell& cell) -> const ESM::Pathgrid* { return nullptr; },
                          },
            cellVariant);
    }
    const ESM::Pathgrid* Store<ESM::Pathgrid>::find(const ESM::Cell& cell) const
    {
        return find(cell.mId);
    }

    // Skill
    //=========================================================================

    void Store<ESM::Skill>::setUp(const MWWorld::Store<ESM::GameSetting>& settings)
    {
        constexpr std::string_view skillValues[ESM::Skill::Length][4] = {
            { "sSkillBlock", "icons\\k\\combat_block.dds", "fWerewolfBlock", {} },
            { "sSkillArmorer", "icons\\k\\combat_armor.dds", "fWerewolfArmorer", {} },
            { "sSkillMediumarmor", "icons\\k\\combat_mediumarmor.dds", "fWerewolfMediumarmor", {} },
            { "sSkillHeavyarmor", "icons\\k\\combat_heavyarmor.dds", "fWerewolfHeavyarmor", {} },
            { "sSkillBluntweapon", "icons\\k\\combat_blunt.dds", "fWerewolfBluntweapon", {} },
            { "sSkillLongblade", "icons\\k\\combat_longblade.dds", "fWerewolfLongblade", {} },
            { "sSkillAxe", "icons\\k\\combat_axe.dds", "fWerewolfAxe", {} },
            { "sSkillSpear", "icons\\k\\combat_spear.dds", "fWerewolfSpear", {} },
            { "sSkillAthletics", "icons\\k\\combat_athletics.dds", "fWerewolfAthletics", {} },
            { "sSkillEnchant", "icons\\k\\magic_enchant.dds", "fWerewolfEnchant", {} },
            { "sSkillDestruction", "icons\\k\\magic_destruction.dds", "fWerewolfDestruction", "destruction" },
            { "sSkillAlteration", "icons\\k\\magic_alteration.dds", "fWerewolfAlteration", "alteration" },
            { "sSkillIllusion", "icons\\k\\magic_illusion.dds", "fWerewolfIllusion", "illusion" },
            { "sSkillConjuration", "icons\\k\\magic_conjuration.dds", "fWerewolfConjuration", "conjuration" },
            { "sSkillMysticism", "icons\\k\\magic_mysticism.dds", "fWerewolfMysticism", "mysticism" },
            { "sSkillRestoration", "icons\\k\\magic_restoration.dds", "fWerewolfRestoration", "restoration" },
            { "sSkillAlchemy", "icons\\k\\magic_alchemy.dds", "fWerewolfAlchemy", {} },
            { "sSkillUnarmored", "icons\\k\\magic_unarmored.dds", "fWerewolfUnarmored", {} },
            { "sSkillSecurity", "icons\\k\\stealth_security.dds", "fWerewolfSecurity", {} },
            { "sSkillSneak", "icons\\k\\stealth_sneak.dds", "fWerewolfSneak", {} },
            { "sSkillAcrobatics", "icons\\k\\stealth_acrobatics.dds", "fWerewolfAcrobatics", {} },
            { "sSkillLightarmor", "icons\\k\\stealth_lightarmor.dds", "fWerewolfLightarmor", {} },
            { "sSkillShortblade", "icons\\k\\stealth_shortblade.dds", "fWerewolfShortblade", {} },
            { "sSkillMarksman", "icons\\k\\stealth_marksman.dds", "fWerewolfMarksman", {} },
            // "Mercantile"! >_<
            { "sSkillMercantile", "icons\\k\\stealth_mercantile.dds", "fWerewolfMerchantile", {} },
            { "sSkillSpeechcraft", "icons\\k\\stealth_speechcraft.dds", "fWerewolfSpeechcraft", {} },
            { "sSkillHandtohand", "icons\\k\\stealth_handtohand.dds", "fWerewolfHandtohand", {} },
        };
        for (ESM::Skill* skill : mShared)
        {
            int index = ESM::Skill::refIdToIndex(skill->mId);
            if (index >= 0)
            {
                const auto& values = skillValues[index];
                skill->mName = getGMSTString(settings, values[0]);
                skill->mIcon = values[1];
                skill->mWerewolfValue = getGMSTFloat(settings, values[2]);
                const auto& school = values[3];
                if (!school.empty())
                {
                    if (!skill->mSchool)
                        skill->mSchool = ESM::MagicSchool{};
                    const std::string id{ school };
                    skill->mSchool->mAreaSound = ESM::RefId::stringRefId(id + " area");
                    skill->mSchool->mBoltSound = ESM::RefId::stringRefId(id + " bolt");
                    skill->mSchool->mCastSound = ESM::RefId::stringRefId(id + " cast");
                    skill->mSchool->mFailureSound = ESM::RefId::stringRefId("Spell Failure " + id);
                    skill->mSchool->mHitSound = ESM::RefId::stringRefId(id + " hit");
                    const std::string name = "sSchool" + id;
                    skill->mSchool->mName = getGMSTString(settings, name);
                    skill->mSchool->mAutoCalcMax = int(getGMSTFloat(settings, "iAutoSpell" + id + "Max"));
                }
            }
        }
    }

    // Game Settings
    //=========================================================================

    const ESM::GameSetting* Store<ESM::GameSetting>::search(const ESM::RefId& id) const
    {
        return TypedDynamicStore::search(id);
    }

    const ESM::GameSetting* Store<ESM::GameSetting>::find(std::string_view id) const
    {
        return TypedDynamicStore::find(ESM::RefId::stringRefId(id));
    }

    const ESM::GameSetting* Store<ESM::GameSetting>::search(std::string_view id) const
    {
        return TypedDynamicStore::search(ESM::RefId::stringRefId(id));
    }

    void Store<ESM::GameSetting>::setUp()
    {
        auto addSetting = [&](const std::string& key, ESM::Variant value) {
            auto id = ESM::RefId::stringRefId(key);
            ESM::GameSetting setting;
            setting.blank();
            setting.mId = id;
            setting.mValue = std::move(value);
            auto [iter, inserted] = mStatic.insert_or_assign(id, std::move(setting));
            if (inserted)
                mShared.push_back(&iter->second);
        };
        for (auto& [key, value] : Fallback::Map::getIntFallbackMap())
            addSetting(key, ESM::Variant(value));
        for (auto& [key, value] : Fallback::Map::getFloatFallbackMap())
            addSetting(key, ESM::Variant(value));
        for (auto& [key, value] : Fallback::Map::getNonNumericFallbackMap())
            addSetting(key, ESM::Variant(value));
        TypedDynamicStore<ESM::GameSetting>::setUp();
    }

    // Magic effect
    //=========================================================================
    Store<ESM::MagicEffect>::Store() {}

    // Attribute
    //=========================================================================

    void Store<ESM::Attribute>::setUp(const MWWorld::Store<ESM::GameSetting>& settings)
    {
        insertStatic({ .mId = ESM::Attribute::Strength,
            .mName = std::string{ getGMSTString(settings, "sAttributeStrength") },
            .mDescription = std::string{ getGMSTString(settings, "sStrDesc") },
            .mIcon = "icons\\k\\attribute_strength.dds",
            .mWerewolfValue = getGMSTFloat(settings, "fWerewolfStrength") });
        insertStatic({ .mId = ESM::Attribute::Intelligence,
            .mName = std::string{ getGMSTString(settings, "sAttributeIntelligence") },
            .mDescription = std::string{ getGMSTString(settings, "sIntDesc") },
            .mIcon = "icons\\k\\attribute_int.dds",
            // Oh, Bethesda. It's "Intelligence".
            .mWerewolfValue = getGMSTFloat(settings, "fWerewolfIntellegence") });
        insertStatic({ .mId = ESM::Attribute::Willpower,
            .mName = std::string{ getGMSTString(settings, "sAttributeWillpower") },
            .mDescription = std::string{ getGMSTString(settings, "sWilDesc") },
            .mIcon = "icons\\k\\attribute_wilpower.dds",
            .mWerewolfValue = getGMSTFloat(settings, "fWerewolfWillpower") });
        insertStatic({ .mId = ESM::Attribute::Agility,
            .mName = std::string{ getGMSTString(settings, "sAttributeAgility") },
            .mDescription = std::string{ getGMSTString(settings, "sAgiDesc") },
            .mIcon = "icons\\k\\attribute_agility.dds",
            .mWerewolfValue = getGMSTFloat(settings, "fWerewolfAgility") });
        insertStatic({ .mId = ESM::Attribute::Speed,
            .mName = std::string{ getGMSTString(settings, "sAttributeSpeed") },
            .mDescription = std::string{ getGMSTString(settings, "sSpdDesc") },
            .mIcon = "icons\\k\\attribute_speed.dds",
            .mWerewolfValue = getGMSTFloat(settings, "fWerewolfSpeed") });
        insertStatic({ .mId = ESM::Attribute::Endurance,
            .mName = std::string{ getGMSTString(settings, "sAttributeEndurance") },
            .mDescription = std::string{ getGMSTString(settings, "sEndDesc") },
            .mIcon = "icons\\k\\attribute_endurance.dds",
            .mWerewolfValue = getGMSTFloat(settings, "fWerewolfEndurance") });
        insertStatic({ .mId = ESM::Attribute::Personality,
            .mName = std::string{ getGMSTString(settings, "sAttributePersonality") },
            .mDescription = std::string{ getGMSTString(settings, "sPerDesc") },
            .mIcon = "icons\\k\\attribute_personality.dds",
            .mWerewolfValue = getGMSTFloat(settings, "fWerewolfPersonality") });
        insertStatic({ .mId = ESM::Attribute::Luck,
            .mName = std::string{ getGMSTString(settings, "sAttributeLuck") },
            .mDescription = std::string{ getGMSTString(settings, "sLucDesc") },
            .mIcon = "icons\\k\\attribute_luck.dds",
            .mWerewolfValue = getGMSTFloat(settings, "fWerewolfLuck") });
    }

    // Dialogue
    //=========================================================================

    Store<ESM::Dialogue>::Store()
        : mKeywordSearchModFlag(true)
    {
    }

    void Store<ESM::Dialogue>::setUp()
    {
        // DialInfos marked as deleted are kept during the loading phase, so that the linked list
        // structure is kept intact for inserting further INFOs. Delete them now that loading is done.
        for (auto& [_, dial] : mStatic)
            dial.setUp();

        mShared.clear();
        mShared.reserve(mStatic.size());
        for (auto& [_, dial] : mStatic)
            mShared.push_back(&dial);
        // TODO: verify and document this inconsistent behaviour
        // TODO: if we require this behaviour, maybe we should move it to the place that requires it
        std::sort(mShared.begin(), mShared.end(),
            [](const ESM::Dialogue* l, const ESM::Dialogue* r) -> bool { return l->mId < r->mId; });

        mKeywordSearchModFlag = true;
    }

    const ESM::Dialogue* Store<ESM::Dialogue>::search(const ESM::RefId& id) const
    {
        typename Static::const_iterator it = mStatic.find(id);
        if (it != mStatic.end())
            return &(it->second);

        return nullptr;
    }

    const ESM::Dialogue* Store<ESM::Dialogue>::find(const ESM::RefId& id) const
    {
        const ESM::Dialogue* ptr = search(id);
        if (ptr == nullptr)
        {
            std::stringstream msg;
            msg << ESM::Dialogue::getRecordType() << " '" << id << "' not found";
            throw std::runtime_error(msg.str());
        }
        return ptr;
    }

    typename Store<ESM::Dialogue>::iterator Store<ESM::Dialogue>::begin() const
    {
        return mShared.begin();
    }

    typename Store<ESM::Dialogue>::iterator Store<ESM::Dialogue>::end() const
    {
        return mShared.end();
    }

    size_t Store<ESM::Dialogue>::getSize() const
    {
        return mShared.size();
    }

    inline RecordId Store<ESM::Dialogue>::load(ESM::ESMReader& esm)
    {
        // The original letter case of a dialogue ID is saved, because it's printed
        ESM::Dialogue dialogue;
        bool isDeleted = false;

        dialogue.loadId(esm);

        Static::iterator found = mStatic.find(dialogue.mId);
        if (found == mStatic.end())
        {
            dialogue.loadData(esm, isDeleted);
            mStatic.emplace(dialogue.mId, dialogue);
        }
        else
        {
            found->second.loadData(esm, isDeleted);
            dialogue.mId = found->second.mId;
        }

        mKeywordSearchModFlag = true;

        return RecordId(dialogue.mId, isDeleted);
    }

    bool Store<ESM::Dialogue>::eraseStatic(const ESM::RefId& id)
    {
        if (eraseFromMap(mStatic, id))
            mKeywordSearchModFlag = true;

        return true;
    }

    void Store<ESM::Dialogue>::listIdentifier(std::vector<ESM::RefId>& list) const
    {
        list.reserve(list.size() + getSize());
        for (const auto& dialogue : mShared)
            list.push_back(dialogue->mId);
    }

    const MWDialogue::KeywordSearch<int>& Store<ESM::Dialogue>::getDialogIdKeywordSearch() const
    {
        if (mKeywordSearchModFlag)
        {
            mKeywordSearch.clear();

            for (const ESM::Dialogue& topic : *this)
                mKeywordSearch.seed(topic.mStringId, 0 /*unused*/);

            mKeywordSearchModFlag = false;
        }

        return mKeywordSearch;
    }

    // ESM4 Cell
    //=========================================================================

    const ESM4::Cell* Store<ESM4::Cell>::searchCellName(std::string_view cellName) const
    {
        const auto foundCell = mCellNameIndex.find(cellName);
        if (foundCell == mCellNameIndex.end())
            return nullptr;
        return foundCell->second;
    }

    const ESM4::Cell* Store<ESM4::Cell>::searchExterior(ESM::ExteriorCellLocation cellIndex) const
    {
        const auto foundCell = mExteriors.find(cellIndex);
        if (foundCell == mExteriors.end())
            return nullptr;
        return foundCell->second;
    }

    ESM4::Cell* Store<ESM4::Cell>::insert(const ESM4::Cell& item, bool overrideOnly)
    {
        auto cellPtr = TypedDynamicStore<ESM4::Cell>::insert(item, overrideOnly);
        insertCell(cellPtr);
        return cellPtr;
    }

    ESM4::Cell* Store<ESM4::Cell>::insertStatic(const ESM4::Cell& item)
    {
        auto cellPtr = TypedDynamicStore<ESM4::Cell>::insertStatic(item);
        insertCell(cellPtr);
        return cellPtr;
    }

    void Store<ESM4::Cell>::insertCell(ESM4::Cell* cellPtr)
    {
        // Do not index exterior cells with Rec_Persistent flag because they are not real cells.
        // References from these cells are merged into normal cells.
        if (cellPtr->isExterior() && cellPtr->mFlags & ESM4::Rec_Persistent)
            return;

        if (!cellPtr->mEditorId.empty())
            mCellNameIndex[cellPtr->mEditorId] = cellPtr;
        if (cellPtr->isExterior())
            mExteriors[ESM::ExteriorCellLocation(cellPtr->mX, cellPtr->mY, cellPtr->mParent)] = cellPtr;
    }

    void Store<ESM4::Cell>::clearDynamic()
    {
        for (auto& cellToDeleteIt : mDynamic)
        {
            ESM4::Cell& cellToDelete = cellToDeleteIt.second;
            if (cellToDelete.isExterior())
            {
                mExteriors.erase({ cellToDelete.mX, cellToDelete.mY, cellToDelete.mParent });
            }
            if (!cellToDelete.mEditorId.empty())
                mCellNameIndex.erase(cellToDelete.mEditorId);
        }
        MWWorld::TypedDynamicStore<ESM4::Cell>::clearDynamic();
    }

    // ESM4 Land
    //=========================================================================
    // Needed to avoid include of ESM4::Land in header
    Store<ESM4::Land>::Store() {}

    void Store<ESM4::Land>::updateLandPositions(const Store<ESM4::Cell>& cells)
    {
        for (const auto& [id, value] : mStatic)
        {
            const ESM4::Cell* cell = cells.find(value.mCell);
            mLands[cell->getExteriorCellLocation()] = &value;
        }
        for (const auto& [id, value] : mDynamic)
        {
            const ESM4::Cell* cell = cells.find(value.mCell);
            mLands[cell->getExteriorCellLocation()] = &value;
        }
    }

    const ESM4::Land* MWWorld::Store<ESM4::Land>::search(ESM::ExteriorCellLocation cellLocation) const
    {
        auto foundLand = mLands.find(cellLocation);
        if (foundLand == mLands.end())
            return nullptr;
        return foundLand->second;
    }
}

template class MWWorld::TypedDynamicStore<ESM::Activator>;
template class MWWorld::TypedDynamicStore<ESM::Apparatus>;
template class MWWorld::TypedDynamicStore<ESM::Armor>;
template class MWWorld::TypedDynamicStore<ESM::Attribute>;
template class MWWorld::TypedDynamicStore<ESM::BirthSign>;
template class MWWorld::TypedDynamicStore<ESM::BodyPart>;
template class MWWorld::TypedDynamicStore<ESM::Book>;
// template class MWWorld::Store<ESM::Cell>;
template class MWWorld::TypedDynamicStore<ESM::Class>;
template class MWWorld::TypedDynamicStore<ESM::Clothing>;
template class MWWorld::TypedDynamicStore<ESM::Container>;
template class MWWorld::TypedDynamicStore<ESM::Creature>;
template class MWWorld::TypedDynamicStore<ESM::CreatureLevList>;
// template class MWWorld::Store<ESM::Dialogue>;
template class MWWorld::TypedDynamicStore<ESM::Door>;
template class MWWorld::TypedDynamicStore<ESM::Enchantment>;
template class MWWorld::TypedDynamicStore<ESM::Faction>;
template class MWWorld::TypedDynamicStore<ESM::GameSetting>;
template class MWWorld::TypedDynamicStore<ESM::Global>;
template class MWWorld::TypedDynamicStore<ESM::Ingredient>;
template class MWWorld::TypedDynamicStore<ESM::ItemLevList>;
// template class MWWorld::Store<ESM::Land>;
// template class MWWorld::Store<ESM::LandTexture>;
template class MWWorld::TypedDynamicStore<ESM::Light>;
template class MWWorld::TypedDynamicStore<ESM::Lockpick>;
// template class MWWorld::Store<ESM::MagicEffect>;
template class MWWorld::TypedDynamicStore<ESM::Miscellaneous>;
template class MWWorld::TypedDynamicStore<ESM::NPC>;
// template class MWWorld::Store<ESM::Pathgrid>;
template class MWWorld::TypedDynamicStore<ESM::Potion>;
template class MWWorld::TypedDynamicStore<ESM::Probe>;
template class MWWorld::TypedDynamicStore<ESM::Race>;
template class MWWorld::TypedDynamicStore<ESM::Region>;
template class MWWorld::TypedDynamicStore<ESM::Repair>;
template class MWWorld::TypedDynamicStore<ESM::Script>;
template class MWWorld::TypedDynamicStore<ESM::Skill>;
template class MWWorld::TypedDynamicStore<ESM::Sound>;
template class MWWorld::TypedDynamicStore<ESM::SoundGenerator>;
template class MWWorld::TypedDynamicStore<ESM::Spell>;
template class MWWorld::TypedDynamicStore<ESM::StartScript>;
template class MWWorld::TypedDynamicStore<ESM::Static>;
template class MWWorld::TypedDynamicStore<ESM::Weapon>;

template class MWWorld::TypedDynamicStore<ESM4::Reference, ESM::FormId>;
template class MWWorld::TypedDynamicStore<ESM4::ActorCharacter, ESM::FormId>;
template class MWWorld::TypedDynamicStore<ESM4::ActorCreature, ESM::FormId>;

template class MWWorld::TypedDynamicStore<ESM4::Activator>;
template class MWWorld::TypedDynamicStore<ESM4::Ammunition>;
template class MWWorld::TypedDynamicStore<ESM4::Armor>;
template class MWWorld::TypedDynamicStore<ESM4::ArmorAddon>;
template class MWWorld::TypedDynamicStore<ESM4::Book>;
template class MWWorld::TypedDynamicStore<ESM4::Cell>;
template class MWWorld::TypedDynamicStore<ESM4::Clothing>;
template class MWWorld::TypedDynamicStore<ESM4::Container>;
template class MWWorld::TypedDynamicStore<ESM4::Creature>;
template class MWWorld::TypedDynamicStore<ESM4::Door>;
template class MWWorld::TypedDynamicStore<ESM4::Flora>;
template class MWWorld::TypedDynamicStore<ESM4::Furniture>;
template class MWWorld::TypedDynamicStore<ESM4::Hair>;
template class MWWorld::TypedDynamicStore<ESM4::HeadPart>;
template class MWWorld::TypedDynamicStore<ESM4::Ingredient>;
template class MWWorld::TypedDynamicStore<ESM4::ItemMod>;
template class MWWorld::TypedDynamicStore<ESM4::Land>;
template class MWWorld::TypedDynamicStore<ESM4::LandTexture>;
template class MWWorld::TypedDynamicStore<ESM4::LevelledCreature>;
template class MWWorld::TypedDynamicStore<ESM4::LevelledItem>;
template class MWWorld::TypedDynamicStore<ESM4::LevelledNpc>;
template class MWWorld::TypedDynamicStore<ESM4::Light>;
template class MWWorld::TypedDynamicStore<ESM4::MiscItem>;
template class MWWorld::TypedDynamicStore<ESM4::MovableStatic>;
template class MWWorld::TypedDynamicStore<ESM4::Npc>;
template class MWWorld::TypedDynamicStore<ESM4::Outfit>;
template class MWWorld::TypedDynamicStore<ESM4::Potion>;
template class MWWorld::TypedDynamicStore<ESM4::Race>;
template class MWWorld::TypedDynamicStore<ESM4::Sound>;
template class MWWorld::TypedDynamicStore<ESM4::SoundReference>;
template class MWWorld::TypedDynamicStore<ESM4::Static>;
template class MWWorld::TypedDynamicStore<ESM4::StaticCollection>;
template class MWWorld::TypedDynamicStore<ESM4::Terminal>;
template class MWWorld::TypedDynamicStore<ESM4::TextureSet>;
template class MWWorld::TypedDynamicStore<ESM4::Tree>;
template class MWWorld::TypedDynamicStore<ESM4::Weapon>;
template class MWWorld::TypedDynamicStore<ESM4::World>;
