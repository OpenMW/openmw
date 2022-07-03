#pragma once
namespace MWWorld
{

    class StoreBase;
    template<typename T> class Store;
    template<typename T> class IndexedStore;

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

        void resize(size_t num) { mStatic.resize(num); }

        size_t getSize() const override;
        size_t getSize(size_t plugin) const;

        RecordId load(ESM::ESMReader &esm) override;

        iterator begin(size_t plugin) const;
        iterator end(size_t plugin) const;
    };

    template <>
    class Store<ESM::Land> : public StoreBase
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
        const ESM::Land *search(int x, int y) const;
        const ESM::Land *find(int x, int y) const;

        RecordId load(ESM::ESMReader &esm) override;
        void setUp() override;
    private:
        bool mBuilt = false;
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

        typedef std::unordered_map<std::string, ESM::Cell, Misc::StringUtils::CiHash, Misc::StringUtils::CiEqual> DynamicInt;
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
        const ESM::Cell *searchStatic(int x, int y) const;
        const ESM::Cell *searchOrCreate(int x, int y);

        const ESM::Cell *find(const std::string &id) const;
        const ESM::Cell *find(int x, int y) const;

        void clearDynamic() override;
        void setUp() override;

        RecordId load(ESM::ESMReader &esm) override;

        iterator intBegin() const;
        iterator intEnd() const;
        iterator extBegin() const;
        iterator extEnd() const;

        // Return the northernmost cell in the easternmost column.
        const ESM::Cell *searchExtByName(const std::string &id) const;

        // Return the northernmost cell in the easternmost column.
        const ESM::Cell *searchExtByRegion(const std::string &id) const;

        size_t getSize() const override;
        size_t getExtSize() const;
        size_t getIntSize() const;

        void listIdentifier(std::vector<std::string> &list) const override;

        ESM::Cell *insert(const ESM::Cell &cell);

        bool erase(const ESM::Cell &cell);
        bool erase(const std::string &id);

        bool erase(int x, int y);
    };

    template <>
    class Store<ESM::Pathgrid> : public StoreBase
    {
    private:
        typedef std::unordered_map<std::string, ESM::Pathgrid, Misc::StringUtils::CiHash, Misc::StringUtils::CiEqual> Interior;
        typedef std::map<std::pair<int, int>, ESM::Pathgrid> Exterior;

        Interior mInt;
        Exterior mExt;

        Store<ESM::Cell>* mCells;

    public:

        Store();

        void setCells(Store<ESM::Cell>& cells);
        RecordId load(ESM::ESMReader &esm) override;
        size_t getSize() const override;

        void setUp() override;

        const ESM::Pathgrid *search(int x, int y) const;
        const ESM::Pathgrid *search(const std::string& name) const;
        const ESM::Pathgrid *find(int x, int y) const;
        const ESM::Pathgrid* find(const std::string& name) const;
        const ESM::Pathgrid *search(const ESM::Cell &cell) const;
        const ESM::Pathgrid *find(const ESM::Cell &cell) const;
    };


    template <>
    class Store<ESM::Skill> : public IndexedStore<ESM::Skill>
    {
    public:
        Store();
    };

    template <>
    class Store<ESM::MagicEffect> : public IndexedStore<ESM::MagicEffect>
    {
    public:
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

        // calls `search` and throws an exception if not found
        const ESM::Attribute *find(size_t index) const;

        void setUp();

        size_t getSize() const;
        iterator begin() const;
        iterator end() const;
    };

    template <>
    class Store<ESM::WeaponType> : public StoreBase
    {
        std::map<int, ESM::WeaponType> mStatic;

    public:
        typedef std::map<int, ESM::WeaponType>::const_iterator iterator;

        Store();

        const ESM::WeaponType *search(const int id) const;

        // calls `search` and throws an exception if not found
        const ESM::WeaponType *find(const int id) const;

        RecordId load(ESM::ESMReader &esm) override { return RecordId({}, false); }

        ESM::WeaponType* insert(const ESM::WeaponType &weaponType);

        void setUp() override;

        size_t getSize() const override;
        iterator begin() const;
        iterator end() const;
    };

    template <>
    class Store<ESM::Dialogue> : public StoreBase
    {
        typedef std::unordered_map<std::string, ESM::Dialogue, Misc::StringUtils::CiHash, Misc::StringUtils::CiEqual> Static;
        Static mStatic;
        /// @par mShared usually preserves the record order as it came from the content files (this
        /// is relevant for the spell autocalc code and selection order
        /// for heads/hairs in the character creation)
        /// @warning ESM::Dialogue Store currently implements a sorted order for unknown reasons.
        std::vector<ESM::Dialogue*> mShared;

        mutable bool mKeywordSearchModFlag;
        mutable MWDialogue::KeywordSearch<std::string, int /*unused*/> mKeywordSearch;

    public:
        Store();

        typedef SharedIterator<ESM::Dialogue> iterator;

        void setUp() override;

        const ESM::Dialogue *search(const std::string &id) const;
        const ESM::Dialogue *find(const std::string &id) const;

        iterator begin() const;
        iterator end() const;

        size_t getSize() const override;

        bool eraseStatic(const std::string &id) override;

        RecordId load(ESM::ESMReader &esm) override;

        void listIdentifier(std::vector<std::string> &list) const override;

        const MWDialogue::KeywordSearch<std::string, int>& getDialogIdKeywordSearch() const;
    };
}
