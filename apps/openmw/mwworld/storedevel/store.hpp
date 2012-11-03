#ifndef OPENMW_MWWORLD_STORE_H
#define OPENMW_MWWORLD_STORE_H

#include <string>
#include <vector>
#include <stdexcept>

#include "recordcmp.hpp"

namespace MWWorld
{
    struct StoreBase
    {
        virtual ~StoreBase() {}

        virtual void setUp() {}
        virtual void listIdentifier(std::vector<std::string> &list) const {}

        virtual int getSize() const = 0;
        virtual void load(ESM::ESMReader &esm, const std::string &id) = 0;
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
            mIter++;

            return iter;
        }

        SharedIterator &operator--() {
            --mIter;
            return *this;
        }

        SharedIterator operator--(int) {
            SharedIterator iter = *this;
            mIter--;

            return iter;
        }

        SharedIterator operator==(const SharedIterator &x) const {
            return mIter == x.mIter;
        }

        SharedIterator operator!=(const SharedIterator &x) const {
            return !(*this == x);
        }

        const T &operator*() const {
            return **mIter;
        }

        const T *operator->() const {
            return &(**mIter);
        }
    };

    template <class T>
    class Store : public StoreBase
    {
        std::vector<T>      mData;
        std::vector<T *>    mShared;

    public:
        Store()
        {}

        Store(const Store<T> &orig)
          : mData(orig.mData)
        {}

        typedef SharedIterator<T> iterator;

        const T* search(const std::string &id) const {
            T item;
            item.mId = StringUtils::lowerCase(id);

            typename std::vector<T>::const_iterator it =
                std::lower_bound(mData.begin(), mData.end(), item, RecordCmp());

            if (it != mData.end() && it->mId == item.mId) {
                return &(*it);
            }
            return 0;
        }

        const T *find(const std::string &id) const {
            const T *ptr = search(id);
            if (ptr == 0) {
                std::ostringstream msg;
                msg << "Object '" << id << "' not found";
                throw std::runtime_error(msg.str());
            }
            return ptr;
        }

        void load(ESM::ESMReader &esm, const std::string &id) {
            mData.push_back(T());
            mData.back().mId = StringUtils::lowerCase(id);
            mData.back().load(esm);
        }

        void setUp() {
            std::sort(mData.begin(), mData.end(), RecordCmp());

            mShared.reserve(mData.size());
            typename std::vector<T>::iterator it = mData.begin();
            for (; it != mData.end(); ++it) {
                mShared.push_back(&(*it));
            }
        }

        iterator begin() {
            return mShared.begin();
        }

        iterator end() {
            return mShared.end();
        }

        int getSize() const {
            return mShared.size();
        }

        void listIdentifier(std::vector<std::string> &list) const {
            list.reserve(list.size() + getSize());
            typename std::vector<T *>::const_iterator it = mShared.begin();
            for (; it != mShared.end(); ++it) {
                list.push_back((*it)->mId);
            }
        }
    };

    template <>
    void Store<ESM::Dialogue>::load(ESM::ESMReader &esm, const std::string &id) {
        mData.push_back(ESM::Dialogue());
        mData.back().mId = id;
        mData.back().load(esm);
    }

    template <>
    const ESM::Dialogue *Store<ESM::Dialogue>::search(const std::string &id) const {
        ESM::Dialogue item;
        item.mId = id;

        std::vector<ESM::Dialogue>::const_iterator it =
            std::lower_bound(mData.begin(), mData.end(), item, RecordCmp());

        if (it != mData.end() && StringUtils::ciEqual(it->mId, id)) {
            return &(*it);
        }
        return 0;
    }

    template <>
    void Store<ESM::Script>::load(ESM::ESMReader &esm, const std::string &id) {
        mData.push_back(ESM::Script());
        mData.back().load(esm);
        StringUtils::toLower(mData.back().mId);
    }

    template <>
    class Store<ESM::LandTexture> : public StoreBase
    {
        std::vector<ESM::LandTexture> mData;

    public:
        Store<ESM::LandTexture>() {
            mData.reserve(128);
        }

        typedef std::vector<ESM::LandTexture>::const_iterator iterator;

        const ESM::LandTexture *search(size_t index) const {
            if (index < mData.size()) {
                return &mData.at(index);
            }
            return 0;
        }

        const ESM::LandTexture *find(size_t index) const {
            const ESM::LandTexture *ptr = search(index);
            if (ptr == 0) {
                std::ostringstream msg;
                msg << "Land texture with index " << index << " not found";
                throw std::runtime_error(msg.str());
            }
            return ptr;
        }

        int getSize() const {
            return mData.size();
        }

        void load(ESM::ESMReader &esm, const std::string &id) {
            ESM::LandTexture ltex;
            ltex.load(esm);

            if (ltex.mIndex >= (int) mData.size()) {
                mData.resize(ltex.mIndex + 1);
            }
            mData[ltex.mIndex] = ltex;
            mData[ltex.mIndex].mId = id;
        }

        iterator begin() {
            return mData.begin();
        }

        iterator end() {
            return mData.end();
        }
    };

    template <>
    class Store<ESM::Land> : public StoreBase
    {
        std::vector<ESM::Land *> mData;

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

        int getSize() const {
            return mData.size();
        }

        iterator begin() {
            return iterator(mData.begin());
        }

        iterator end() {
            return iterator(mData.end());
        }

        ESM::Land *search(int x, int y) {
            ESM::Land land;
            land.mX = x, land.mY = y;

            std::vector<ESM::Land *>::iterator it =
                std::lower_bound(mData.begin(), mData.end(), &land, Compare());

            if (it != mData.end() && (*it)->mX == x && (*it)->mY == y) {
                return *it;
            }
            return 0;
        }

        ESM::Land *find(int x, int y) {
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

            mData.push_back(ptr);
        }

        void setUp() {
            std::sort(mData.begin(), mData.end(), Compare());
        }
    };

    template <>
    class Store<ESM::Cell> : public StoreBase
    {
    public:
        typedef std::vector<ESM::Cell>::const_iterator iterator;

    private:

        struct ExtCompare
        {
            bool operator()(const ESM::Cell &x, const ESM::Cell &y) {
                if (x.mData.mX == y.mData.mX) {
                    return x.mData.mY < y.mData.mY;
                }
                return x.mData.mX < y.mData.mX;
            }
        };

        struct IntExtOrdering
        {
            bool operator()(const ESM::Cell &x, const ESM::Cell &y) {
                // interiors precedes exteriors (x < y)
                if ((x.mData.mFlags & ESM::Cell::Interior) != 0 &&
                    (y.mData.mFlags & ESM::Cell::Interior) == 0)
                {
                    return true;
                }
                return false;
            }
        };

        std::vector<ESM::Cell>  mData;
        std::vector<ESM::Cell>::iterator mIntBegin, mIntEnd, mExtBegin, mExtEnd;

    public:
        Store<ESM::Cell>()
        {}

        const ESM::Cell *search(const std::string &id) const {
            ESM::Cell cell;
            cell.mName = id;

            iterator it =
                std::lower_bound(mIntBegin, mIntEnd, cell, RecordCmp());

            if (it != mIntEnd && StringUtils::ciEqual(it->mName, id)) {
                return &(*it);
            }
            return 0;
        }

        const ESM::Cell *search(int x, int y) const {
            ESM::Cell cell;
            cell.mData.mX = x, cell.mData.mY = y;

            iterator it =
                std::lower_bound(mExtBegin, mExtEnd, cell, ExtCompare());

            if (it != mExtEnd && it->mData.mX == x && it->mData.mY == y) {
                return &(*it);
            }
            return 0;
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
            IntExtOrdering cmp;
            std::sort(mData.begin(), mData.end(), cmp);

            ESM::Cell cell;
            cell.mData.mFlags = 0;
            mExtBegin =
                std::lower_bound(mData.begin(), mData.end(), cell, cmp);
            mExtEnd = mData.end();

            mIntBegin = mData.begin();
            mIntEnd = mExtBegin;

            std::sort(mIntBegin, mIntEnd, RecordCmp());
            std::sort(mExtBegin, mExtEnd, ExtCompare());
        }

        void load(ESM::ESMReader &esm, const std::string &id) {
            mData.push_back(ESM::Cell());
            mData.back().mName = id;
            mData.back().load(esm);
        }

        iterator begin() const {
            return mData.begin();
        }

        iterator end() const {
            return mData.end();
        }

        iterator interiorsBegin() const {
            return mIntBegin;
        }

        iterator interiorsEnd() const {
            return mIntEnd;
        }

        iterator exteriorsBegin() const {
            return mExtBegin;
        }

        iterator exteriorsEnd() const {
            return mExtEnd;
        }

        /// \todo implement appropriate index
        const ESM::Cell *searchExteriorByName(const std::string &id) const {
            for (iterator it = mExtBegin; it != mExtEnd; ++it) {
                if (StringUtils::ciEqual(it->mName, id)) {
                    return &(*it);
                }
            }
            return 0;
        }

        /// \todo implement appropriate index
        const ESM::Cell *searchExteriorByRegion(const std::string &id) const {
            for (iterator it = mExtBegin; it != mExtEnd; ++it) {
                if (StringUtils::ciEqual(it->mRegion, id)) {
                    return &(*it);
                }
            }
            return 0;
        }

        int getSize() const {
            return mData.size();
        }

        void listIdentifier(std::vector<std::string> &list) const {
            list.reserve(list.size() + (mIntEnd - mIntBegin));
            for (iterator it = mIntBegin; it != mIntEnd; ++it) {
                list.push_back(it->mName);
            }
        }
    };

    template <>
    class Store<ESM::Pathgrid> : public StoreBase
    {
    public:
        typedef std::vector<ESM::Pathgrid>::const_iterator iterator;

    private:
        std::vector<ESM::Pathgrid>  mData;

        std::vector<ESM::Pathgrid>::iterator mIntBegin, mIntEnd, mExtBegin, mExtEnd;

        struct IntExtOrdering
        {
            bool operator()(const ESM::Pathgrid &x, const ESM::Pathgrid &y) const {
                // interior pathgrids precedes exterior ones (x < y)
                if ((x.mData.mX == 0 && x.mData.mY == 0) &&
                    (y.mData.mX != 0 || y.mData.mY != 0))
                {
                    return false;
                }
            }
        };

        struct ExtCompare
        {
            bool operator()(const ESM::Pathgrid &x, const ESM::Pathgrid &y) const {
                if (x.mData.mX == y.mData.mX) {
                    return x.mData.mY < y.mData.mY;
                }
                return x.mData.mX < y.mData.mX;
            }
        };

    public:

        void load(ESM::ESMReader &esm, const std::string &id) {
            mData.push_back(ESM::Pathgrid());
            mData.back().load(esm);
        }

        int getSize() const {
            return mData.size();
        }

        void setUp() {
            IntExtOrdering cmp;
            std::sort(mData.begin(), mData.end(), cmp);

            ESM::Pathgrid pg;
            pg.mData.mX = pg.mData.mY = 1;
            mExtBegin =
                std::lower_bound(mData.begin(), mData.end(), pg, cmp);
            mExtEnd = mData.end();

            mIntBegin = mData.begin();
            mIntEnd = mExtBegin;

            std::sort(mIntBegin, mIntEnd, RecordCmp());
            std::sort(mExtBegin, mExtEnd, ExtCompare());
        }

        const ESM::Pathgrid *search(int x, int y) const {
            ESM::Pathgrid pg;
            pg.mData.mX = x;
            pg.mData.mY = y;

            iterator it =
                std::lower_bound(mExtBegin, mExtEnd, pg, ExtCompare());
            if (it != mExtEnd && it->mData.mX == x && it->mData.mY == y) {
                return &(*it);
            }
            return 0;
        }

        const ESM::Pathgrid *find(int x, int y) const {
            const ESM::Pathgrid *ptr = search(x, y);
            if (ptr == 0) {
                std::ostringstream msg;
                msg << "Pathgrid at (" << x << ", " << y << ") not found";
                throw std::runtime_error(msg.str());
            }
            return ptr;
        }

        const ESM::Pathgrid *search(const std::string &name) const {
            ESM::Pathgrid pg;
            pg.mCell = name;

            iterator it = std::lower_bound(mIntBegin, mIntEnd, pg, RecordCmp());
            if (it != mIntEnd && StringUtils::ciEqual(it->mCell, name)) {
                return &(*it);
            }
            return 0;
        }

        const ESM::Pathgrid *find(const std::string &name) const {
            const ESM::Pathgrid *ptr = search(name);
            if (ptr == 0) {
                std::ostringstream msg;
                msg << "Pathgrid in cell '" << name << "' not found";
                throw std::runtime_error(msg.str());
            }
            return ptr;
        }

        const ESM::Pathgrid *search(const ESM::Cell &cell) const {
            if (cell.mData.mFlags & ESM::Cell::Interior) {
                return search(cell.mName);
            }
            return search(cell.mData.mX, cell.mData.mY);
        }

        const ESM::Pathgrid *find(const ESM::Cell &cell) const {
            if (cell.mData.mFlags & ESM::Cell::Interior) {
                return find(cell.mName);
            }
            return find(cell.mData.mX, cell.mData.mY);
        }

        iterator begin() const {
            return mData.begin();
        }

        iterator end() const {
            return mData.end();
        }

        iterator interiorPathsBegin() const {
            return mIntBegin;
        }

        iterator interiorPathsEnd() const {
            return mIntEnd;
        }

        iterator exteriorPathsBegin() const {
            return mExtBegin;
        }

        iterator exteriorPathsEnd() const {
            return mExtEnd;
        }
    };

    template <class T>
    class IndexedStore
    {
        struct Compare
        {
            bool operator()(const T &x, const T &y) const {
                return x.mIndex < y.mIndex;
            }
        };
    protected:
        std::vector<T> mData;

    public:
        typedef typename std::vector<T>::const_iterator iterator;

        IndexedStore() {}

        IndexedStore(unsigned int size) {
            mData.reserve(size);
        }

        iterator begin() const {
            return mData.begin();
        }

        iterator end() const {
            return mData.end();
        }

        /// \todo refine loading order
        void load(ESM::ESMReader &esm) {
            mData.push_back(T());
            mData.back().load(esm);
        }

        int getSize() const {
            return mData.size();
        }

        void setUp() {
            std::sort(mData.begin(), mData.end(), Compare());
        }

        const T *search(int index) const {
            T item;
            item.mIndex = index;

            iterator it =
                std::lower_bound(mData.begin(), mData.end(), item, Compare());
            if (it != mData.end() && it->mIndex == index) {
                return &(*it);
            }
            return 0;
        }

        const T *find(int index) const {
            T *ptr = search(index);
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
        Store(unsigned int size)
          : IndexedStore<ESM::Skill>(size)
        {}
    };

    template <>
    struct Store<ESM::MagicEffect> : public IndexedStore<ESM::MagicEffect>
    {
        Store() {}
        Store(unsigned int size)
          : IndexedStore<ESM::MagicEffect>(size)
        {}
    };

    template <>
    class Store<ESM::Attribute> : public IndexedStore<ESM::Attribute>
    {
        std::vector<ESM::Attribute> mData;

    public:
        typedef std::vector<ESM::Attribute>::const_iterator iterator;

        Store() {
            mData.reserve(ESM::Attribute::Length);
        }

        const ESM::Attribute *search(size_t index) const {
            if (index >= mData.size()) {
                return 0;
            }
            return &mData.at(index);
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
                mData.push_back(
                    ESM::Attribute(
                        ESM::Attribute::sAttributeIds[i],
                        ESM::Attribute::sGmstAttributeIds[i],
                        ESM::Attribute::sGmstAttributeDescIds[i]
                    )
                );
            }
        }

        int getSize() const {
            return mData.size();
        }

        iterator begin() const {
            return mData.begin();
        }

        iterator end() const {
            return mData.end();
        }
    };

} //end namespace

#endif
