#ifndef OPENMW_MWWORLD_STORE_H
#define OPENMW_MWWORLD_STORE_H

#include <map>
#include <memory>
#include <set>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

#include <components/esm/attr.hpp>
#include <components/esm/refid.hpp>
#include <components/esm/util.hpp>
#include <components/esm3/loadcell.hpp>
#include <components/esm3/loaddial.hpp>
#include <components/esm3/loadglob.hpp>
#include <components/esm3/loadgmst.hpp>
#include <components/esm3/loadland.hpp>
#include <components/esm3/loadpgrd.hpp>
#include <components/esm3/loadskil.hpp>
#include <components/esm4/loadachr.hpp>
#include <components/esm4/loadcell.hpp>
#include <components/esm4/loadland.hpp>
#include <components/esm4/loadrefr.hpp>
#include <components/misc/rng.hpp>
#include <components/misc/strings/algorithm.hpp>

#include "../mwdialogue/keywordsearch.hpp"

namespace ESM
{
    struct LandTexture;
    struct MagicEffect;
    struct WeaponType;
    class ESMReader;
    class ESMWriter;
}

namespace Loading
{
    class Listener;
}

namespace MWWorld
{
    class Cell;
    struct RecordId
    {
        ESM::RefId mId;
        bool mIsDeleted;

        RecordId(const ESM::RefId& id = {}, bool isDeleted = false);
    };

    class StoreBase
    {
    }; // Empty interface to be parent of all store types

    template <class Id>
    class DynamicStoreBase : public StoreBase
    {
    public:
        virtual ~DynamicStoreBase() = default;

        virtual void setUp() {}

        /// List identifiers of records contained in this Store (case-smashed). No-op for Stores that don't use string
        /// IDs.
        virtual void listIdentifier(std::vector<Id>& list) const {}

        virtual size_t getSize() const = 0;
        virtual size_t getDynamicSize() const { return 0; }
        virtual RecordId load(ESM::ESMReader& esm) = 0;

        virtual bool eraseStatic(const Id& id) { return false; }
        virtual void clearDynamic() {}

        virtual void write(ESM::ESMWriter& writer, Loading::Listener& progress) const {}

        virtual RecordId read(ESM::ESMReader& reader, bool overrideOnly = false) { return RecordId(); }
        ///< Read into dynamic storage
    };

    using DynamicStore = DynamicStoreBase<ESM::RefId>;

    template <class T>
    class IndexedStore : public StoreBase
    {
    protected:
        typedef typename std::map<int, T> Static;
        Static mStatic;

    public:
        typedef typename std::map<int, T>::const_iterator iterator;

        IndexedStore();

        iterator begin() const;
        iterator end() const;
        iterator findIter(int index) const { return mStatic.find(index); }

        void load(ESM::ESMReader& esm);

        size_t getSize() const;
        void setUp();

        const T* search(int index) const;

        // calls `search` and throws an exception if not found
        const T* find(int index) const;
    };

    template <class T>
    class SharedIterator
    {
    public:
        using Iter = typename std::vector<T*>::const_iterator;
        using iterator_category = std::random_access_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T*;
        using reference = T&;

        SharedIterator() = default;

        SharedIterator(const SharedIterator& other) = default;

        SharedIterator(const Iter& iter)
            : mIter(iter)
        {
        }

        SharedIterator& operator=(const SharedIterator&) = default;

        SharedIterator& operator++()
        {
            ++mIter;
            return *this;
        }

        SharedIterator operator++(int)
        {
            SharedIterator iter = *this;
            ++mIter;

            return iter;
        }

        SharedIterator& operator+=(difference_type advance)
        {
            mIter += advance;
            return *this;
        }

        SharedIterator& operator--()
        {
            --mIter;
            return *this;
        }

        SharedIterator operator--(int)
        {
            SharedIterator iter = *this;
            --mIter;

            return iter;
        }

        bool operator==(const SharedIterator& x) const { return mIter == x.mIter; }

        bool operator!=(const SharedIterator& x) const { return !(*this == x); }

        const T& operator*() const { return **mIter; }

        const T* operator->() const { return &(**mIter); }

    private:
        Iter mIter;

        friend inline difference_type operator-(const SharedIterator& lhs, const SharedIterator& rhs)
        {
            return lhs.mIter - rhs.mIter;
        }
    };

    class ESMStore;

    template <class T, class Id = ESM::RefId>
    class TypedDynamicStore : public DynamicStoreBase<Id>
    {
    protected:
        typedef std::unordered_map<Id, T> Static;
        Static mStatic;
        /// @par mShared usually preserves the record order as it came from the content files (this
        /// is relevant for the spell autocalc code and selection order
        /// for heads/hairs in the character creation)
        std::vector<T*> mShared;
        typedef std::unordered_map<Id, T> Dynamic;
        Dynamic mDynamic;

        friend class ESMStore;

    public:
        TypedDynamicStore();
        TypedDynamicStore(const TypedDynamicStore<T, Id>& orig);

        typedef SharedIterator<T> iterator;

        // setUp needs to be called again after
        void clearDynamic() override;
        void setUp() override;

        const T* search(const Id& id) const;
        const T* searchStatic(const Id& id) const;

        /**
         * Does the record with this ID come from the dynamic store?
         */
        bool isDynamic(const Id& id) const;

        /** Returns a random record that starts with the named ID, or nullptr if not found. */
        const T* searchRandom(const std::string_view prefix, Misc::Rng::Generator& prng) const;

        // calls `search` and throws an exception if not found
        const T* find(const Id& id) const;

        iterator begin() const;
        iterator end() const;
        const T* at(size_t index) const { return mShared.at(index); }

        size_t getSize() const override;
        size_t getDynamicSize() const override;

        /// @note The record identifiers are listed in the order that the records were defined by the content files.
        void listIdentifier(std::vector<Id>& list) const override;

        T* insert(const T& item, bool overrideOnly = false);
        T* insertStatic(const T& item);

        bool eraseStatic(const Id& id) override;
        bool erase(const Id& id);
        bool erase(const T& item);

        RecordId load(ESM::ESMReader& esm) override;
        void write(ESM::ESMWriter& writer, Loading::Listener& progress) const override;
        RecordId read(ESM::ESMReader& reader, bool overrideOnly = false) override;
    };

    template <class T>
    class Store : public TypedDynamicStore<T>
    {
    };

    template <>
    class Store<ESM::LandTexture> : public DynamicStore
    {
        using PluginIndex = std::pair<int, std::uint32_t>; // This is essentially a FormId
        std::unordered_map<ESM::RefId, std::string> mStatic;
        std::map<PluginIndex, ESM::RefId> mMappings;

    public:
        Store();

        // Must be threadsafe! Called from terrain background loading threads.
        // Not a big deal here, since ESM::LandTexture can never be modified or inserted/erased
        const std::string* search(std::uint32_t index, int plugin) const;

        size_t getSize() const override;
        bool eraseStatic(const ESM::RefId& id) override;

        RecordId load(ESM::ESMReader& esm) override;
    };

    template <>
    class Store<ESM::GameSetting> : public TypedDynamicStore<ESM::GameSetting>
    {
    public:
        const ESM::GameSetting* search(const ESM::RefId& id) const;

        const ESM::GameSetting* find(const std::string_view id) const;
        const ESM::GameSetting* search(const std::string_view id) const;

        void setUp() override;
    };

    template <>
    class Store<ESM4::Cell> : public TypedDynamicStore<ESM4::Cell>
    {
        std::unordered_map<std::string, ESM4::Cell*, Misc::StringUtils::CiHash, Misc::StringUtils::CiEqual>
            mCellNameIndex;
        std::unordered_map<ESM::ExteriorCellLocation, ESM4::Cell*> mExteriors;

    public:
        const ESM4::Cell* searchCellName(std::string_view) const;
        const ESM4::Cell* searchExterior(ESM::ExteriorCellLocation cellIndex) const;
        ESM4::Cell* insert(const ESM4::Cell& item, bool overrideOnly = false);
        ESM4::Cell* insertStatic(const ESM4::Cell& item);
        void insertCell(ESM4::Cell* cell);
        void clearDynamic() override;
    };

    template <>
    class Store<ESM4::Land> : public TypedDynamicStore<ESM4::Land>
    {
        std::unordered_map<ESM::ExteriorCellLocation, const ESM4::Land*> mLands;

    public:
        Store();
        void updateLandPositions(const Store<ESM4::Cell>& cells);

        const ESM4::Land* search(ESM::ExteriorCellLocation cellLocation) const;
        const std::unordered_map<ESM::ExteriorCellLocation, const ESM4::Land*>& getLands() const { return mLands; }
    };

    template <>
    class Store<ESM::Land> : public DynamicStore
    {
        struct SpatialComparator
        {
            using is_transparent = void;

            bool operator()(const ESM::Land& x, const ESM::Land& y) const
            {
                return std::tie(x.mX, x.mY) < std::tie(y.mX, y.mY);
            }
            bool operator()(const ESM::Land& x, const std::pair<int, int>& y) const
            {
                return std::tie(x.mX, x.mY) < std::tie(y.first, y.second);
            }
            bool operator()(const std::pair<int, int>& x, const ESM::Land& y) const
            {
                return std::tie(x.first, x.second) < std::tie(y.mX, y.mY);
            }
        };
        using Statics = std::set<ESM::Land, SpatialComparator>;
        Statics mStatic;

    public:
        typedef typename Statics::iterator iterator;

        virtual ~Store();

        size_t getSize() const override;
        iterator begin() const;
        iterator end() const;

        // Must be threadsafe! Called from terrain background loading threads.
        // Not a big deal here, since ESM::Land can never be modified or inserted/erased
        const ESM::Land* search(int x, int y) const;
        const ESM::Land* find(int x, int y) const;

        RecordId load(ESM::ESMReader& esm) override;
        void setUp() override;

    private:
        bool mBuilt = false;
    };

    template <>
    class Store<ESM::Cell> : public DynamicStore
    {
        struct DynamicExtCmp
        {
            bool operator()(const std::pair<int, int>& left, const std::pair<int, int>& right) const
            {
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

        typedef std::unordered_map<std::string, ESM::Cell*, Misc::StringUtils::CiHash, Misc::StringUtils::CiEqual>
            DynamicInt;

        typedef std::map<std::pair<int, int>, ESM::Cell*, DynamicExtCmp> DynamicExt;

        std::unordered_map<ESM::RefId, ESM::Cell> mCells;

        DynamicInt mInt;
        DynamicExt mExt;

        std::vector<ESM::Cell*> mSharedInt;
        std::vector<ESM::Cell*> mSharedExt;

        DynamicInt mDynamicInt;
        DynamicExt mDynamicExt;

        const ESM::Cell* search(const ESM::Cell& cell) const;
        void handleMovedCellRefs(ESM::ESMReader& esm, ESM::Cell* cell);

    public:
        typedef SharedIterator<ESM::Cell> iterator;

        const ESM::Cell* search(const ESM::RefId& id) const;
        const ESM::Cell* search(std::string_view id) const;
        const ESM::Cell* search(int x, int y) const;
        const ESM::Cell* searchStatic(int x, int y) const;
        const ESM::Cell* searchOrCreate(int x, int y);

        const ESM::Cell* find(const ESM::RefId& id) const;
        const ESM::Cell* find(std::string_view id) const;
        const ESM::Cell* find(int x, int y) const;

        void clearDynamic() override;
        void setUp() override;

        RecordId load(ESM::ESMReader& esm) override;

        iterator intBegin() const;
        iterator intEnd() const;
        iterator extBegin() const;
        iterator extEnd() const;

        // Return the northernmost cell in the easternmost column.
        const ESM::Cell* searchExtByName(std::string_view id) const;

        // Return the northernmost cell in the easternmost column.
        const ESM::Cell* searchExtByRegion(const ESM::RefId& id) const;

        size_t getSize() const override;
        size_t getExtSize() const;
        size_t getIntSize() const;

        const ESM::Cell* at(size_t index) const
        {
            if (index < mSharedInt.size())
                return mSharedInt.at(index);
            else
                return mSharedExt.at(index - mSharedInt.size());
        }

        void listIdentifier(std::vector<ESM::RefId>& list) const override;

        ESM::Cell* insert(const ESM::Cell& cell);
    };

    template <>
    class Store<ESM::Pathgrid> : public DynamicStore
    {
    private:
        std::unordered_map<ESM::RefId, ESM::Pathgrid> mStatic;
        Store<ESM::Cell>* mCells;

    public:
        Store();

        void setCells(Store<ESM::Cell>& cells);
        RecordId load(ESM::ESMReader& esm) override;
        size_t getSize() const override;

        void setUp() override;

        const ESM::Pathgrid* search(const ESM::RefId& name) const;
        const ESM::Pathgrid* find(const ESM::RefId& name) const;
        const ESM::Pathgrid* search(const ESM::Cell& cell) const;
        const ESM::Pathgrid* search(const MWWorld::Cell& cell) const;
        const ESM::Pathgrid* find(const ESM::Cell& cell) const;
    };

    template <>
    class Store<ESM::Skill> : public TypedDynamicStore<ESM::Skill>
    {
        using TypedDynamicStore<ESM::Skill>::setUp;

    public:
        Store() = default;

        void setUp(const MWWorld::Store<ESM::GameSetting>& settings);
    };

    template <>
    class Store<ESM::MagicEffect> : public IndexedStore<ESM::MagicEffect>
    {
    public:
        Store();
    };

    template <>
    class Store<ESM::Attribute> : public TypedDynamicStore<ESM::Attribute>
    {
        using TypedDynamicStore<ESM::Attribute>::setUp;

    public:
        Store() = default;

        void setUp(const MWWorld::Store<ESM::GameSetting>& settings);
    };

    template <>
    class Store<ESM::WeaponType> : public DynamicStore
    {
        std::map<int, ESM::WeaponType> mStatic;

    public:
        typedef std::map<int, ESM::WeaponType>::const_iterator iterator;

        Store();

        const ESM::WeaponType* search(const int id) const;

        // calls `search` and throws an exception if not found
        const ESM::WeaponType* find(const int id) const;

        RecordId load(ESM::ESMReader& esm) override { return RecordId({}, false); }

        ESM::WeaponType* insert(const ESM::WeaponType& weaponType);

        void setUp() override;

        size_t getSize() const override;
        iterator begin() const;
        iterator end() const;
    };

    template <>
    class Store<ESM::Dialogue> : public DynamicStore
    {
        typedef std::unordered_map<ESM::RefId, ESM::Dialogue> Static;
        Static mStatic;
        /// @par mShared usually preserves the record order as it came from the content files (this
        /// is relevant for the spell autocalc code and selection order
        /// for heads/hairs in the character creation)
        /// @warning ESM::Dialogue Store currently implements a sorted order for unknown reasons.
        std::vector<ESM::Dialogue*> mShared;

        mutable bool mKeywordSearchModFlag;
        mutable MWDialogue::KeywordSearch<int /*unused*/> mKeywordSearch;

    public:
        Store();

        typedef SharedIterator<ESM::Dialogue> iterator;

        void setUp() override;

        const ESM::Dialogue* search(const ESM::RefId& id) const;
        const ESM::Dialogue* find(const ESM::RefId& id) const;

        iterator begin() const;
        iterator end() const;

        size_t getSize() const override;

        bool eraseStatic(const ESM::RefId& id) override;

        RecordId load(ESM::ESMReader& esm) override;

        void listIdentifier(std::vector<ESM::RefId>& list) const override;

        const MWDialogue::KeywordSearch<int>& getDialogIdKeywordSearch() const;
    };

    template <typename T>
    class ESM4RefsStore : public TypedDynamicStore<T, ESM::FormId>
    {
    public:
        void preprocessReferences(const Store<ESM4::Cell>& cells)
        {
            for (auto& [_, ref] : this->mStatic)
            {
                const ESM4::Cell* cell = cells.find(ref.mParent);
                if (cell->isExterior() && (cell->mFlags & ESM4::Rec_Persistent))
                {
                    const ESM4::Cell* actualCell = cells.searchExterior(
                        positionToExteriorCellLocation(ref.mPos.pos[0], ref.mPos.pos[1], cell->mParent));
                    if (actualCell)
                        ref.mParent = actualCell->mId;
                }
                mPerCellReferences[ref.mParent].push_back(&ref);
            }
        }

        std::span<const T* const> getByCell(ESM::RefId cellId) const
        {
            auto it = mPerCellReferences.find(cellId);
            if (it == mPerCellReferences.end())
                return {};
            return it->second;
        }

    private:
        std::unordered_map<ESM::RefId, std::vector<T*>> mPerCellReferences;
    };

    template <>
    class Store<ESM4::Reference> : public ESM4RefsStore<ESM4::Reference>
    {
    };

    template <>
    class Store<ESM4::ActorCharacter> : public ESM4RefsStore<ESM4::ActorCharacter>
    {
    };

    template <>
    class Store<ESM4::ActorCreature> : public ESM4RefsStore<ESM4::ActorCreature>
    {
    };

} // end namespace

#endif
