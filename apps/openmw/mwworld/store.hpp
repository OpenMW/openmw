#ifndef OPENMW_MWWORLD_STORE_H
#define OPENMW_MWWORLD_STORE_H

#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <sstream>

#include <openengine/misc/rng.hpp>

#include <components/esm/esmwriter.hpp>

#include <components/loadinglistener/loadinglistener.hpp>

#include "recordcmp.hpp"

namespace MWWorld
{
    struct StoreBase
    {
        virtual ~StoreBase() {}

        virtual void setUp() {}
        virtual void listIdentifier(std::vector<std::string> &list) const {}

        virtual size_t getSize() const = 0;
        virtual int getDynamicSize() const { return 0; }
        virtual void load(ESM::ESMReader &esm, const std::string &id) = 0;

        virtual bool eraseStatic(const std::string &id) {return false;}
        virtual void clearDynamic() {}

        virtual void write (ESM::ESMWriter& writer, Loading::Listener& progress) const {}

        virtual void read (ESM::ESMReader& reader, const std::string& id) {}
        ///< Read into dynamic storage
    };

    template <class T>
    class SharedIterator
    {
        typedef typename std::vector<T *>::const_iterator Iter;

        Iter mIter;

    public:
        SharedIterator() {}

        SharedIterator(const SharedIterator &orig)
          : mIter(orig.mIter)
        {}

        SharedIterator(const Iter &iter)
          : mIter(iter)
        {}

        SharedIterator &operator++() {
            ++mIter;
            return *this;
        }

        SharedIterator operator++(int) {
            SharedIterator iter = *this;
            ++mIter;

            return iter;
        }

        SharedIterator &operator--() {
            --mIter;
            return *this;
        }

        SharedIterator operator--(int) {
            SharedIterator iter = *this;
            --mIter;

            return iter;
        }

        bool operator==(const SharedIterator &x) const {
            return mIter == x.mIter;
        }

        bool operator!=(const SharedIterator &x) const {
            return !(*this == x);
        }

        const T &operator*() const {
            return **mIter;
        }

        const T *operator->() const {
            return &(**mIter);
        }
    };

    class ESMStore;

    template <class T>
    class Store : public StoreBase
    {
        std::map<std::string, T>      mStatic;
        std::vector<T *>    mShared; // Preserves the record order as it came from the content files (this
                                     // is relevant for the spell autocalc code and selection order
                                     // for heads/hairs in the character creation)
        std::map<std::string, T> mDynamic;

        typedef std::map<std::string, T> Dynamic;
        typedef std::map<std::string, T> Static;

        class GetRecords {
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


        friend class ESMStore;

    public:
        Store()
        {}

        Store(const Store<T> &orig)
          : mStatic(orig.mData)
        {}

        typedef SharedIterator<T> iterator;

        // setUp needs to be called again after
        virtual void clearDynamic()
        {
            // remove the dynamic part of mShared
            assert(mShared.size() >= mStatic.size());
            mShared.erase(mShared.begin() + mStatic.size(), mShared.end());
            mDynamic.clear();
        }

        const T *search(const std::string &id) const {
            T item;
            item.mId = Misc::StringUtils::lowerCase(id);

            typename Dynamic::const_iterator dit = mDynamic.find(item.mId);
            if (dit != mDynamic.end()) {
                return &dit->second;
            }

            typename std::map<std::string, T>::const_iterator it = mStatic.find(item.mId);

            if (it != mStatic.end() && Misc::StringUtils::ciEqual(it->second.mId, id)) {
                return &(it->second);
            }

            return 0;
        }

        /**
         * Does the record with this ID come from the dynamic store?
         */
        bool isDynamic(const std::string &id) const {
            typename Dynamic::const_iterator dit = mDynamic.find(id);
            return (dit != mDynamic.end());
        }

        /** Returns a random record that starts with the named ID, or NULL if not found. */
        const T *searchRandom(const std::string &id) const
        {
            std::vector<const T*> results;
            std::for_each(mShared.begin(), mShared.end(), GetRecords(id, &results));
            if(!results.empty())
                return results[OEngine::Misc::Rng::rollDice(results.size())];
            return NULL;
        }

        const T *find(const std::string &id) const {
            const T *ptr = search(id);
            if (ptr == 0) {
                std::ostringstream msg;
                msg << "Object '" << id << "' not found (const)";
                throw std::runtime_error(msg.str());
            }
            return ptr;
        }

        /** Returns a random record that starts with the named ID. An exception is thrown if none
         * are found. */
        const T *findRandom(const std::string &id) const
        {
            const T *ptr = searchRandom(id);
            if(ptr == 0)
            {
                std::ostringstream msg;
                msg << "Object starting with '"<<id<<"' not found (const)";
                throw std::runtime_error(msg.str());
            }
            return ptr;
        }

        void load(ESM::ESMReader &esm, const std::string &id) {
            std::string idLower = Misc::StringUtils::lowerCase(id);

            std::pair<typename Static::iterator, bool> inserted = mStatic.insert(std::make_pair(idLower, T()));
            if (inserted.second)
                mShared.push_back(&inserted.first->second);

            inserted.first->second.mId = idLower;
            inserted.first->second.load(esm);
        }

        void setUp() {
        }

        iterator begin() const {
            return mShared.begin();
        }

        iterator end() const {
            return mShared.end();
        }

        size_t getSize() const {
            return mShared.size();
        }

        int getDynamicSize() const
        {
            return static_cast<int> (mDynamic.size()); // truncated from unsigned __int64 if _MSC_VER && _WIN64
        }

        void listIdentifier(std::vector<std::string> &list) const {
            list.reserve(list.size() + getSize());
            typename std::vector<T *>::const_iterator it = mShared.begin();
            for (; it != mShared.end(); ++it) {
                list.push_back((*it)->mId);
            }
        }

        T *insert(const T &item) {
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

        T *insertStatic(const T &item) {
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


        bool eraseStatic(const std::string &id) {
            T item;
            item.mId = Misc::StringUtils::lowerCase(id);

            typename std::map<std::string, T>::iterator it = mStatic.find(item.mId);

            if (it != mStatic.end() && Misc::StringUtils::ciEqual(it->second.mId, id)) {
                // delete from the static part of mShared
                typename std::vector<T *>::iterator sharedIter = mShared.begin();
                typename std::vector<T *>::iterator end = sharedIter + mStatic.size();

                while (sharedIter != mShared.end() && sharedIter != end) {
                    if((*sharedIter)->mId == item.mId) {
                        mShared.erase(sharedIter);
                        break;
                    }
                    ++sharedIter;
                }
                mStatic.erase(it);
            }

            return true;
        }

        bool erase(const std::string &id) {
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

        bool erase(const T &item) {
            return erase(item.mId);
        }

        void write (ESM::ESMWriter& writer, Loading::Listener& progress) const
        {
            for (typename Dynamic::const_iterator iter (mDynamic.begin()); iter!=mDynamic.end();
                 ++iter)
            {
                writer.startRecord (T::sRecordId);
                writer.writeHNString ("NAME", iter->second.mId);
                iter->second.save (writer);
                writer.endRecord (T::sRecordId);
            }
        }

        void read (ESM::ESMReader& reader, const std::string& id)
        {
            T record;
            record.mId = id;
            record.load (reader);
            insert (record);
        }
    };

    template <>
    inline void Store<ESM::Dialogue>::load(ESM::ESMReader &esm, const std::string &id) {
        std::string idLower = Misc::StringUtils::lowerCase(id);

        std::map<std::string, ESM::Dialogue>::iterator it = mStatic.find(idLower);
        if (it == mStatic.end()) {
            it = mStatic.insert( std::make_pair( idLower, ESM::Dialogue() ) ).first;
            it->second.mId = id; // don't smash case here, as this line is printed
        }

        it->second.load(esm);
    }

    template <>
    inline void Store<ESM::Script>::load(ESM::ESMReader &esm, const std::string &id) {
        ESM::Script scpt;
        scpt.load(esm);
        Misc::StringUtils::toLower(scpt.mId);

        std::pair<typename Static::iterator, bool> inserted = mStatic.insert(std::make_pair(scpt.mId, scpt));
        if (inserted.second)
            mShared.push_back(&inserted.first->second);
        else
            inserted.first->second = scpt;
    }

    template <>
    inline void Store<ESM::StartScript>::load(ESM::ESMReader &esm, const std::string &id)
    {
        ESM::StartScript s;
        s.load(esm);
        s.mId = Misc::StringUtils::toLower(s.mId);
        std::pair<typename Static::iterator, bool> inserted = mStatic.insert(std::make_pair(s.mId, s));
        if (inserted.second)
            mShared.push_back(&inserted.first->second);
        else
            inserted.first->second = s;
    }

    template <>
    class Store<ESM::LandTexture> : public StoreBase
    {
        // For multiple ESM/ESP files we need one list per file.
        typedef std::vector<ESM::LandTexture> LandTextureList;
        std::vector<LandTextureList> mStatic;

    public:
        Store<ESM::LandTexture>() {
            mStatic.push_back(LandTextureList());
            LandTextureList &ltexl = mStatic[0];
            // More than enough to hold Morrowind.esm. Extra lists for plugins will we
            //  added on-the-fly in a different method.
            ltexl.reserve(128);
        }

        typedef std::vector<ESM::LandTexture>::const_iterator iterator;

        // Must be threadsafe! Called from terrain background loading threads.
        // Not a big deal here, since ESM::LandTexture can never be modified or inserted/erased
        const ESM::LandTexture *search(size_t index, size_t plugin) const {
            assert(plugin < mStatic.size());
            const LandTextureList &ltexl = mStatic[plugin];

            assert(index < ltexl.size());
            return &ltexl.at(index);
        }

        const ESM::LandTexture *find(size_t index, size_t plugin) const {
            const ESM::LandTexture *ptr = search(index, plugin);
            if (ptr == 0) {
                std::ostringstream msg;
                msg << "Land texture with index " << index << " not found";
                throw std::runtime_error(msg.str());
            }
            return ptr;
        }

        size_t getSize() const {
            return mStatic.size();
        }

        size_t getSize(size_t plugin) const {
            assert(plugin < mStatic.size());
            return mStatic[plugin].size();
        }

        void load(ESM::ESMReader &esm, const std::string &id, size_t plugin) {
            ESM::LandTexture lt;
            lt.load(esm);
            lt.mId = id;

            // Make sure we have room for the structure
            if (plugin >= mStatic.size()) {
                mStatic.resize(plugin+1);
            }
            LandTextureList &ltexl = mStatic[plugin];
            if(lt.mIndex + 1 > (int)ltexl.size())
                ltexl.resize(lt.mIndex+1);

            // Store it
            ltexl[lt.mIndex] = lt;
        }

        void load(ESM::ESMReader &esm, const std::string &id);

        iterator begin(size_t plugin) const {
            assert(plugin < mStatic.size());
            return mStatic[plugin].begin();
        }

        iterator end(size_t plugin) const {
            assert(plugin < mStatic.size());
            return mStatic[plugin].end();
        }
    };

    template <>
    class Store<ESM::Land> : public StoreBase
    {
        std::vector<ESM::Land *> mStatic;

        struct Compare
        {
            bool operator()(const ESM::Land *x, const ESM::Land *y) {
                if (x->mX == y->mX) {
                    return x->mY < y->mY;
                }
                return x->mX < y->mX;
            }
        };

    public:
        typedef SharedIterator<ESM::Land> iterator;

        virtual ~Store<ESM::Land>()
        {
            for (std::vector<ESM::Land *>::const_iterator it =
                             mStatic.begin(); it != mStatic.end(); ++it)
            {
                delete *it;
            }

        }

        size_t getSize() const {
            return mStatic.size();
        }

        iterator begin() const {
            return iterator(mStatic.begin());
        }

        iterator end() const {
            return iterator(mStatic.end());
        }

        // Must be threadsafe! Called from terrain background loading threads.
        // Not a big deal here, since ESM::Land can never be modified or inserted/erased
        ESM::Land *search(int x, int y) const {
            ESM::Land land;
            land.mX = x, land.mY = y;

            std::vector<ESM::Land *>::const_iterator it =
                std::lower_bound(mStatic.begin(), mStatic.end(), &land, Compare());

            if (it != mStatic.end() && (*it)->mX == x && (*it)->mY == y) {
                return const_cast<ESM::Land *>(*it);
            }
            return 0;
        }

        ESM::Land *find(int x, int y) const{
            ESM::Land *ptr = search(x, y);
            if (ptr == 0) {
                std::ostringstream msg;
                msg << "Land at (" << x << ", " << y << ") not found";
                throw std::runtime_error(msg.str());
            }
            return ptr;
        }

        void load(ESM::ESMReader &esm, const std::string &id) {
            ESM::Land *ptr = new ESM::Land();
            ptr->load(esm);

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
        }

        void setUp() {
            std::sort(mStatic.begin(), mStatic.end(), Compare());
        }
    };

    template <>
    class Store<ESM::Cell> : public StoreBase
    {
        struct DynamicExtCmp
        {
            bool operator()(const std::pair<int, int> &left, const std::pair<int, int> &right) const {
                if (left.first == right.first && left.second == right.second)
                    return false;

                if (left.first == right.first)
                    return left.second > right.second;

                // Exterior cells are listed in descending, row-major order,
                // this is a workaround for an ambiguous chargen_plank reference in the vanilla game.
                // there is one at -22,16 and one at -2,-9, the latter should be used.
                return left.first > right.first;
            }
        };

        typedef std::map<std::string, ESM::Cell>                           DynamicInt;
        typedef std::map<std::pair<int, int>, ESM::Cell, DynamicExtCmp>    DynamicExt;

        DynamicInt      mInt;
        DynamicExt      mExt;

        std::vector<ESM::Cell *>    mSharedInt;
        std::vector<ESM::Cell *>    mSharedExt;

        DynamicInt mDynamicInt;
        DynamicExt mDynamicExt;

        const ESM::Cell *search(const ESM::Cell &cell) const {
            if (cell.isExterior()) {
                return search(cell.getGridX(), cell.getGridY());
            }
            return search(cell.mName);
        }

        void handleMovedCellRefs(ESM::ESMReader& esm, ESM::Cell* cell);

    public:
        typedef SharedIterator<ESM::Cell> iterator;

        const ESM::Cell *search(const std::string &id) const {
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

        const ESM::Cell *search(int x, int y) const {
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

        const ESM::Cell *searchOrCreate(int x, int y) {
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

        const ESM::Cell *find(const std::string &id) const {
            const ESM::Cell *ptr = search(id);
            if (ptr == 0) {
                std::ostringstream msg;
                msg << "Interior cell '" << id << "' not found";
                throw std::runtime_error(msg.str());
            }
            return ptr;
        }

        const ESM::Cell *find(int x, int y) const {
            const ESM::Cell *ptr = search(x, y);
            if (ptr == 0) {
                std::ostringstream msg;
                msg << "Exterior at (" << x << ", " << y << ") not found";
                throw std::runtime_error(msg.str());
            }
            return ptr;
        }

        void setUp() {
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

        // HACK: Method implementation had to be moved to a separate cpp file, as we would otherwise get
        //  errors related to the compare operator used in std::find for ESM::MovedCellRefTracker::find.
        //  There some nasty three-way cyclic header dependency involved, which I could only fix by moving
        //  this method.
        void load(ESM::ESMReader &esm, const std::string &id);

        iterator intBegin() const {
            return iterator(mSharedInt.begin());
        }

        iterator intEnd() const {
            return iterator(mSharedInt.end());
        }

        iterator extBegin() const {
            return iterator(mSharedExt.begin());
        }

        iterator extEnd() const {
            return iterator(mSharedExt.end());
        }

        // Return the northernmost cell in the easternmost column.
        const ESM::Cell *searchExtByName(const std::string &id) const {
            ESM::Cell *cell = 0;
            std::vector<ESM::Cell *>::const_iterator it = mSharedExt.begin();
            for (; it != mSharedExt.end(); ++it) {
                if (Misc::StringUtils::ciEqual((*it)->mName, id)) {
                    if ( cell == 0 ||
                        ( (*it)->mData.mX > cell->mData.mX ) ||
                        ( (*it)->mData.mX == cell->mData.mX && (*it)->mData.mY > cell->mData.mY ) )
                    {
                        cell = *it;
                    }
                }
            }
            return cell;
        }

        // Return the northernmost cell in the easternmost column.
        const ESM::Cell *searchExtByRegion(const std::string &id) const {
            ESM::Cell *cell = 0;
            std::vector<ESM::Cell *>::const_iterator it = mSharedExt.begin();
            for (; it != mSharedExt.end(); ++it) {
                if (Misc::StringUtils::ciEqual((*it)->mRegion, id)) {
                    if ( cell == 0 ||
                        ( (*it)->mData.mX > cell->mData.mX ) ||
                        ( (*it)->mData.mX == cell->mData.mX && (*it)->mData.mY > cell->mData.mY ) )
                    {
                        cell = *it;
                    }
                }
            }
            return cell;
        }

        size_t getSize() const {
            return mSharedInt.size() + mSharedExt.size();
        }

        void listIdentifier(std::vector<std::string> &list) const {
            list.reserve(list.size() + mSharedInt.size());

            std::vector<ESM::Cell *>::const_iterator it = mSharedInt.begin();
            for (; it != mSharedInt.end(); ++it) {
                list.push_back((*it)->mName);
            }
        }

        ESM::Cell *insert(const ESM::Cell &cell) {
            if (search(cell) != 0) {
                std::ostringstream msg;
                msg << "Failed to create ";
                msg << ((cell.isExterior()) ? "exterior" : "interior");
                msg << " cell";

                throw std::runtime_error(msg.str());
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

        bool erase(const ESM::Cell &cell) {
            if (cell.isExterior()) {
                return erase(cell.getGridX(), cell.getGridY());
            }
            return erase(cell.mName);
        }

        bool erase(const std::string &id) {
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

        bool erase(int x, int y) {
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
    };

    template <>
    class Store<ESM::Pathgrid> : public StoreBase
    {
    private:
        typedef std::map<std::string, ESM::Pathgrid> Interior;
        typedef std::map<std::pair<int, int>, ESM::Pathgrid> Exterior;

        Interior mInt;
        Exterior mExt;

        Store<ESM::Cell>* mCells;

    public:

        Store<ESM::Pathgrid>()
            : mCells(NULL)
        {
        }

        void setCells(Store<ESM::Cell>& cells)
        {
            mCells = &cells;
        }

        void load(ESM::ESMReader &esm, const std::string &id) {
            ESM::Pathgrid pathgrid;
            pathgrid.load(esm);

            // Unfortunately the Pathgrid record model does not specify whether the pathgrid belongs to an interior or exterior cell.
            // For interior cells, mCell is the cell name, but for exterior cells it is either the cell name or if that doesn't exist, the cell's region name.
            // mX and mY will be (0,0) for interior cells, but there is also an exterior cell with the coordinates of (0,0), so that doesn't help.
            // Check whether mCell is an interior cell. This isn't perfect, will break if a Region with the same name as an interior cell is created.
            // A proper fix should be made for future versions of the file format.
            bool interior = mCells->search(pathgrid.mCell) != NULL;

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
        }

        size_t getSize() const {
            return mInt.size() + mExt.size();
        }

        void setUp() {
        }

        const ESM::Pathgrid *search(int x, int y) const {
            Exterior::const_iterator it = mExt.find(std::make_pair(x,y));
            if (it != mExt.end())
                return &(it->second);
            return NULL;
        }

        const ESM::Pathgrid *search(const std::string& name) const {
            Interior::const_iterator it = mInt.find(name);
            if (it != mInt.end())
                return &(it->second);
            return NULL;
        }

        const ESM::Pathgrid *find(int x, int y) const {
            const ESM::Pathgrid* pathgrid = search(x,y);
            if (!pathgrid)
            {
                std::ostringstream msg;
                msg << "Pathgrid in cell '" << x << " " << y << "' not found";
                throw std::runtime_error(msg.str());
            }
            return pathgrid;
        }

        const ESM::Pathgrid* find(const std::string& name) const {
            const ESM::Pathgrid* pathgrid = search(name);
            if (!pathgrid)
            {
                std::ostringstream msg;
                msg << "Pathgrid in cell '" << name << "' not found";
                throw std::runtime_error(msg.str());
            }
            return pathgrid;
        }

        const ESM::Pathgrid *search(const ESM::Cell &cell) const {
            if (!(cell.mData.mFlags & ESM::Cell::Interior))
                return search(cell.mData.mX, cell.mData.mY);
            else
                return search(cell.mName);
        }

        const ESM::Pathgrid *find(const ESM::Cell &cell) const {
            if (!(cell.mData.mFlags & ESM::Cell::Interior))
                return find(cell.mData.mX, cell.mData.mY);
            else
                return find(cell.mName);
        }
    };

    template <class T>
    class IndexedStore
    {
    protected:
        typedef typename std::map<int, T> Static;
        Static mStatic;

    public:
        typedef typename std::map<int, T>::const_iterator iterator;

        IndexedStore() {}

        iterator begin() const {
            return mStatic.begin();
        }

        iterator end() const {
            return mStatic.end();
        }

        void load(ESM::ESMReader &esm) {
            T record;
            record.load(esm);

            // Try to overwrite existing record
            std::pair<typename Static::iterator, bool> ret = mStatic.insert(std::make_pair(record.mIndex, record));
            if (!ret.second)
                ret.first->second = record;
        }

        int getSize() const {
            return mStatic.size();
        }

        void setUp() {
        }

        const T *search(int index) const {
            typename Static::const_iterator it = mStatic.find(index);
            if (it != mStatic.end())
                return &(it->second);
            return NULL;
        }

        const T *find(int index) const {
            const T *ptr = search(index);
            if (ptr == 0) {
                std::ostringstream msg;
                msg << "Object with index " << index << " not found";
                throw std::runtime_error(msg.str());
            }
            return ptr;
        }
    };

    template <>
    struct Store<ESM::Skill> : public IndexedStore<ESM::Skill>
    {
        Store() {}
    };

    template <>
    struct Store<ESM::MagicEffect> : public IndexedStore<ESM::MagicEffect>
    {
        Store() {}
    };

    template <>
    class Store<ESM::Attribute> : public IndexedStore<ESM::Attribute>
    {
        std::vector<ESM::Attribute> mStatic;

    public:
        typedef std::vector<ESM::Attribute>::const_iterator iterator;

        Store() {
            mStatic.reserve(ESM::Attribute::Length);
        }

        const ESM::Attribute *search(size_t index) const {
            if (index >= mStatic.size()) {
                return 0;
            }
            return &mStatic.at(index);
        }

        const ESM::Attribute *find(size_t index) const {
            const ESM::Attribute *ptr = search(index);
            if (ptr == 0) {
                std::ostringstream msg;
                msg << "Attribute with index " << index << " not found";
                throw std::runtime_error(msg.str());
            }
            return ptr;
        }

        void setUp() {
            for (int i = 0; i < ESM::Attribute::Length; ++i) {
                mStatic.push_back(
                    ESM::Attribute(
                        ESM::Attribute::sAttributeIds[i],
                        ESM::Attribute::sGmstAttributeIds[i],
                        ESM::Attribute::sGmstAttributeDescIds[i]
                    )
                );
            }
        }

        size_t getSize() const {
            return mStatic.size();
        }

        iterator begin() const {
            return mStatic.begin();
        }

        iterator end() const {
            return mStatic.end();
        }
    };

    template<>
    inline void Store<ESM::Dialogue>::setUp()
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

} //end namespace

#endif
