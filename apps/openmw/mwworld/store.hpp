#ifndef OPENMW_MWWORLD_STORE_H
#define OPENMW_MWWORLD_STORE_H

#include <string>
#include <vector>
#include <map>

#include "recordcmp.hpp"

namespace ESM
{
    struct Land;
}

namespace Loading
{
    class Listener;
}

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
    class IndexedStore
    {
    protected:
        typedef typename std::map<int, T> Static;
        Static mStatic;

    public:
        typedef typename std::map<int, T>::const_iterator iterator;

        IndexedStore();

        iterator begin() const;
        iterator end() const;

        void load(ESM::ESMReader &esm);

        int getSize() const;
        void setUp();

        const T *search(int index) const;
        const T *find(int index) const;
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

        friend class ESMStore;

    public:
        Store();
        Store(const Store<T> &orig);

        typedef SharedIterator<T> iterator;

        // setUp needs to be called again after
        virtual void clearDynamic();
        void setUp();

        const T *search(const std::string &id) const;

        /**
         * Does the record with this ID come from the dynamic store?
         */
        bool isDynamic(const std::string &id) const;

        /** Returns a random record that starts with the named ID, or NULL if not found. */
        const T *searchRandom(const std::string &id) const;

        const T *find(const std::string &id) const;

        /** Returns a random record that starts with the named ID. An exception is thrown if none
         * are found. */
        const T *findRandom(const std::string &id) const;

        iterator begin() const;
        iterator end() const;

        size_t getSize() const;
        int getDynamicSize() const;

        void listIdentifier(std::vector<std::string> &list) const;

        T *insert(const T &item);
        T *insertStatic(const T &item);

        bool eraseStatic(const std::string &id);
        bool erase(const std::string &id);
        bool erase(const T &item);

        void load(ESM::ESMReader &esm, const std::string &id);
        void write(ESM::ESMWriter& writer, Loading::Listener& progress) const;
        void read(ESM::ESMReader& reader, const std::string& id);
    };

    template <>
    class Store<ESM::LandTexture> : public StoreBase
    {
        // For multiple ESM/ESP files we need one list per file.
        typedef std::vector<ESM::LandTexture> LandTextureList;
        std::vector<LandTextureList> mStatic;

    public:
        Store();

        typedef std::vector<ESM::LandTexture>::const_iterator iterator;

        // Must be threadsafe! Called from terrain background loading threads.
        // Not a big deal here, since ESM::LandTexture can never be modified or inserted/erased
        const ESM::LandTexture *search(size_t index, size_t plugin) const;
        const ESM::LandTexture *find(size_t index, size_t plugin) const;

        size_t getSize() const;
        size_t getSize(size_t plugin) const;

        void load(ESM::ESMReader &esm, const std::string &id, size_t plugin);
        void load(ESM::ESMReader &esm, const std::string &id);

        iterator begin(size_t plugin) const;
        iterator end(size_t plugin) const;
    };

    template <>
    class Store<ESM::Land> : public StoreBase
    {
        std::vector<ESM::Land *> mStatic;

    public:
        typedef SharedIterator<ESM::Land> iterator;

        virtual ~Store();

        size_t getSize() const;
        iterator begin() const;
        iterator end() const;

        // Must be threadsafe! Called from terrain background loading threads.
        // Not a big deal here, since ESM::Land can never be modified or inserted/erased
        ESM::Land *search(int x, int y) const;
        ESM::Land *find(int x, int y) const;

        void load(ESM::ESMReader &esm, const std::string &id);
        void setUp();
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

        const ESM::Cell *search(const ESM::Cell &cell) const;
        void handleMovedCellRefs(ESM::ESMReader& esm, ESM::Cell* cell);

    public:
        typedef SharedIterator<ESM::Cell> iterator;

        const ESM::Cell *search(const std::string &id) const;
        const ESM::Cell *search(int x, int y) const;
        const ESM::Cell *searchOrCreate(int x, int y);

        const ESM::Cell *find(const std::string &id) const;
        const ESM::Cell *find(int x, int y) const;

        void setUp();

        void load(ESM::ESMReader &esm, const std::string &id);

        iterator intBegin() const;
        iterator intEnd() const;
        iterator extBegin() const;
        iterator extEnd() const;

        // Return the northernmost cell in the easternmost column.
        const ESM::Cell *searchExtByName(const std::string &id) const;

        // Return the northernmost cell in the easternmost column.
        const ESM::Cell *searchExtByRegion(const std::string &id) const;

        size_t getSize() const;

        void listIdentifier(std::vector<std::string> &list) const;

        ESM::Cell *insert(const ESM::Cell &cell);

        bool erase(const ESM::Cell &cell);
        bool erase(const std::string &id);

        bool erase(int x, int y);
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

        Store();

        void setCells(Store<ESM::Cell>& cells);
        void load(ESM::ESMReader &esm, const std::string &id);
        size_t getSize() const;

        void setUp();

        const ESM::Pathgrid *search(int x, int y) const;
        const ESM::Pathgrid *search(const std::string& name) const;
        const ESM::Pathgrid *find(int x, int y) const;
        const ESM::Pathgrid* find(const std::string& name) const;
        const ESM::Pathgrid *search(const ESM::Cell &cell) const;
        const ESM::Pathgrid *find(const ESM::Cell &cell) const;
    };


    template <>
    struct Store<ESM::Skill> : public IndexedStore<ESM::Skill>
    {
        Store();
    };

    template <>
    struct Store<ESM::MagicEffect> : public IndexedStore<ESM::MagicEffect>
    {
        Store();
    };

    template <>
    class Store<ESM::Attribute> : public IndexedStore<ESM::Attribute>
    {
        std::vector<ESM::Attribute> mStatic;

    public:
        typedef std::vector<ESM::Attribute>::const_iterator iterator;

        Store();

        const ESM::Attribute *search(size_t index) const;
        const ESM::Attribute *find(size_t index) const;

        void setUp();

        size_t getSize() const;
        iterator begin() const;
        iterator end() const;
    };


} //end namespace

#endif
