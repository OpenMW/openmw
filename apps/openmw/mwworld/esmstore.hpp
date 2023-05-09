#ifndef OPENMW_MWWORLD_ESMSTORE_H
#define OPENMW_MWWORLD_ESMSTORE_H

#include <filesystem>
#include <memory>
#include <stdexcept>
#include <tuple>
#include <unordered_map>

#include <components/esm/luascripts.hpp>
#include <components/esm/refid.hpp>
#include <components/esm3/loadgmst.hpp>
#include <components/misc/tuplemeta.hpp>

#include "store.hpp"

namespace Loading
{
    class Listener;
}

namespace MWMechanics
{
    class SpellList;
}

namespace ESM4
{
    class Reader;
    struct Static;
    struct Cell;
    struct Light;
    struct Reference;
    struct Activator;
    struct Potion;
    struct Ammunition;
    struct Armor;
    struct Book;
    struct Clothing;
    struct Container;
    struct Door;
    struct Ingredient;
    struct MiscItem;
    struct Weapon;
}

namespace ESM
{
    class ReadersCache;
    struct Activator;
    struct Potion;
    struct Apparatus;
    struct Armor;
    struct BodyPart;
    struct Book;
    struct BirthSign;
    struct Class;
    struct Clothing;
    struct Container;
    struct Creature;
    struct Dialogue;
    struct Door;
    struct Enchantment;
    struct Faction;
    struct Global;
    struct Ingredient;
    struct CreatureLevList;
    struct ItemLevList;
    struct Light;
    struct Lockpick;
    struct Miscellaneous;
    struct NPC;
    struct Probe;
    struct Race;
    struct Region;
    struct Repair;
    struct SoundGenerator;
    struct Sound;
    struct Spell;
    struct StartScript;
    struct Static;
    struct Weapon;
    struct GameSetting;
    class Script;
    struct Cell;
    struct Land;
    struct LandTexture;
    struct Pathgrid;
    struct MagicEffect;
    struct Skill;
    struct Attribute;
}

namespace MWWorld
{
    struct ESMStoreImp;

    class ESMStore
    {
        friend struct ESMStoreImp; // This allows StoreImp to extend esmstore without beeing included everywhere
    public:
        using StoreTuple = std::tuple<Store<ESM::Activator>, Store<ESM::Potion>, Store<ESM::Apparatus>,
            Store<ESM::Armor>, Store<ESM::BodyPart>, Store<ESM::Book>, Store<ESM::BirthSign>, Store<ESM::Class>,
            Store<ESM::Clothing>, Store<ESM::Container>, Store<ESM::Creature>, Store<ESM::Dialogue>, Store<ESM::Door>,
            Store<ESM::Enchantment>, Store<ESM::Faction>, Store<ESM::Global>, Store<ESM::Ingredient>,
            Store<ESM::CreatureLevList>, Store<ESM::ItemLevList>, Store<ESM::Light>, Store<ESM::Lockpick>,
            Store<ESM::Miscellaneous>, Store<ESM::NPC>, Store<ESM::Probe>, Store<ESM::Race>, Store<ESM::Region>,
            Store<ESM::Repair>, Store<ESM::SoundGenerator>, Store<ESM::Sound>, Store<ESM::Spell>,
            Store<ESM::StartScript>, Store<ESM::Static>, Store<ESM::Weapon>, Store<ESM::GameSetting>,
            Store<ESM::Script>,

            // Lists that need special rules
            Store<ESM::Cell>, Store<ESM::Land>, Store<ESM::LandTexture>, Store<ESM::Pathgrid>,

            Store<ESM::MagicEffect>, Store<ESM::Skill>,

            // Special entry which is hardcoded and not loaded from an ESM
            Store<ESM::Attribute>,

            Store<ESM4::Static>, Store<ESM4::Cell>, Store<ESM4::Light>, Store<ESM4::Reference>, Store<ESM4::Activator>,
            Store<ESM4::Potion>, Store<ESM4::Ammunition>, Store<ESM4::Armor>, Store<ESM4::Book>, Store<ESM4::Clothing>,
            Store<ESM4::Container>, Store<ESM4::Door>, Store<ESM4::Ingredient>, Store<ESM4::MiscItem>,
            Store<ESM4::Weapon>>;

    private:
        template <typename T>
        static constexpr std::size_t getTypeIndex()
        {
            static_assert(Misc::TupleHasType<Store<T>, StoreTuple>::value);
            return Misc::TupleTypeIndex<Store<T>, StoreTuple>::value;
        }

        std::unique_ptr<ESMStoreImp> mStoreImp;

        std::unordered_map<ESM::RefId, int> mRefCount;

        std::vector<StoreBase*> mStores;
        std::vector<DynamicStore*> mDynamicStores;

        unsigned int mDynamicCount;

        mutable std::unordered_map<ESM::RefId, std::weak_ptr<MWMechanics::SpellList>> mSpellListCache;

        template <class T>
        Store<T>& getWritable()
        {
            return static_cast<Store<T>&>(*mStores[getTypeIndex<T>()]);
        }

        /// Validate entries in store after setup
        void validate();

        void countAllCellRefsAndMarkKeys(ESM::ReadersCache& readers);

        template <class T>
        void removeMissingObjects(Store<T>& store);

        void setIdType(const ESM::RefId& id, ESM::RecNameInts type);

        using LuaContent = std::variant<ESM::LuaScriptsCfg, // data from an omwaddon
            std::filesystem::path>; // path to an omwscripts file
        std::vector<LuaContent> mLuaContent;

    public:
        void addOMWScripts(std::filesystem::path filePath) { mLuaContent.push_back(std::move(filePath)); }
        ESM::LuaScriptsCfg getLuaScriptsCfg() const;

        /// \todo replace with SharedIterator<StoreBase>
        typedef std::vector<DynamicStore*>::const_iterator iterator;

        iterator begin() const { return mDynamicStores.begin(); }

        iterator end() const { return mDynamicStores.end(); }

        /// Look up the given ID in 'all'. Returns 0 if not found.
        int find(const ESM::RefId& id) const;

        int findStatic(const ESM::RefId& id) const;

        ESMStore();
        ~ESMStore();

        void clearDynamic();

        void movePlayerRecord();

        /// Validate entries in store after loading a save
        void validateDynamic();

        void load(ESM::ESMReader& esm, Loading::Listener* listener, ESM::Dialogue*& dialogue);
        void loadESM4(ESM4::Reader& esm);

        template <class T>
        const Store<T>& get() const
        {
            return static_cast<const Store<T>&>(*mStores[getTypeIndex<T>()]);
        }

        /// Insert a custom record (i.e. with a generated ID that will not clash will pre-existing records)
        /// \return pointer to created record
        template <class T>
        const T* insert(const T& x)
        {
            const ESM::RefId id = ESM::RefId::generated(mDynamicCount++);

            Store<T>& store = getWritable<T>();
            if (store.search(id) != nullptr)
                throw std::runtime_error("Try to override existing record: " + id.toDebugString());
            T record = x;

            record.mId = id;

            T* ptr = store.insert(record);
            if constexpr (std::is_convertible_v<Store<T>*, DynamicStore*>)
            {
                setIdType(ptr->mId, T::sRecordId);
            }
            return ptr;
        }

        /// Insert a record with set ID, and allow it to override a pre-existing static record.
        template <class T>
        const T* overrideRecord(const T& x)
        {
            Store<T>& store = getWritable<T>();

            T* ptr = store.insert(x);
            if constexpr (std::is_convertible_v<Store<T>*, DynamicStore*>)
            {
                setIdType(ptr->mId, T::sRecordId);
            }
            return ptr;
        }

        template <class T>
        const T* insertStatic(const T& x)
        {
            Store<T>& store = getWritable<T>();
            if (store.search(x.mId) != nullptr)
                throw std::runtime_error("Try to override existing record " + x.mId.toDebugString());

            T* ptr = store.insertStatic(x);
            if constexpr (std::is_convertible_v<Store<T>*, DynamicStore*>)
            {
                setIdType(ptr->mId, T::sRecordId);
            }
            return ptr;
        }

        // This method must be called once, after loading all master/plugin files. This can only be done
        //  from the outside, so it must be public.
        void setUp();
        void validateRecords(ESM::ReadersCache& readers);

        int countSavedGameRecords() const;

        void write(ESM::ESMWriter& writer, Loading::Listener& progress) const;

        bool readRecord(ESM::ESMReader& reader, uint32_t type);
        ///< \return Known type?

        // To be called when we are done with dynamic record loading
        void checkPlayer();

        /// @return The number of instances defined in the base files. Excludes changes from the save file.
        int getRefCount(const ESM::RefId& id) const;

        /// Actors with the same ID share spells, abilities, etc.
        /// @return The shared spell list to use for this actor and whether or not it has already been initialized.
        std::pair<std::shared_ptr<MWMechanics::SpellList>, bool> getSpellList(const ESM::RefId& id) const;
    };
    template <>
    const ESM::Cell* ESMStore::insert<ESM::Cell>(const ESM::Cell& cell);

    template <>
    const ESM::NPC* ESMStore::insert<ESM::NPC>(const ESM::NPC& npc);

    template <class T, class = std::void_t<>>
    struct HasRecordId : std::false_type
    {
    };

    template <class T>
    struct HasRecordId<T, std::void_t<decltype(T::sRecordId)>> : std::true_type
    {
    };
}

#endif
