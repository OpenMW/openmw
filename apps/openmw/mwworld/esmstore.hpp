#ifndef OPENMW_MWWORLD_ESMSTORE_H
#define OPENMW_MWWORLD_ESMSTORE_H

#include <sstream>
#include <stdexcept>

#include <components/esm/records.hpp>
#include "store.hpp"

namespace Loading
{
    class Listener;
}

namespace MWWorld
{
    class ESMStore
    {
        Store<ESM::Activator>       mActivators;
        Store<ESM::Potion>          mPotions;
        Store<ESM::Apparatus>       mAppas;
        Store<ESM::Armor>           mArmors;
        Store<ESM::BodyPart>        mBodyParts;
        Store<ESM::Book>            mBooks;
        Store<ESM::BirthSign>       mBirthSigns;
        Store<ESM::Class>           mClasses;
        Store<ESM::Clothing>        mClothes;
        Store<ESM::Container>       mContainers;
        Store<ESM::Creature>        mCreatures;
        Store<ESM::Dialogue>        mDialogs;
        Store<ESM::Door>            mDoors;
        Store<ESM::Enchantment>     mEnchants;
        Store<ESM::Faction>         mFactions;
        Store<ESM::Global>          mGlobals;
        Store<ESM::Ingredient>      mIngreds;
        Store<ESM::CreatureLevList> mCreatureLists;
        Store<ESM::ItemLevList>     mItemLists;
        Store<ESM::Light>           mLights;
        Store<ESM::Lockpick>        mLockpicks;
        Store<ESM::Miscellaneous>   mMiscItems;
        Store<ESM::NPC>             mNpcs;
        Store<ESM::Probe>           mProbes;
        Store<ESM::Race>            mRaces;
        Store<ESM::Region>          mRegions;
        Store<ESM::Repair>          mRepairs;
        Store<ESM::SoundGenerator>  mSoundGens;
        Store<ESM::Sound>           mSounds;
        Store<ESM::Spell>           mSpells;
        Store<ESM::StartScript>     mStartScripts;
        Store<ESM::Static>          mStatics;
        Store<ESM::Weapon>          mWeapons;

        Store<ESM::GameSetting>     mGameSettings;
        Store<ESM::Script>          mScripts;

        // Lists that need special rules
        Store<ESM::Cell>        mCells;
        Store<ESM::Land>        mLands;
        Store<ESM::LandTexture> mLandTextures;
        Store<ESM::Pathgrid>    mPathgrids;

        Store<ESM::MagicEffect> mMagicEffects;
        Store<ESM::Skill>       mSkills;

        // Special entry which is hardcoded and not loaded from an ESM
        Store<ESM::Attribute>   mAttributes;

        // Lookup of all IDs. Makes looking up references faster. Just
        // maps the id name to the record type.
        std::map<std::string, int> mIds;
        std::map<int, StoreBase *> mStores;

        ESM::NPC mPlayerTemplate;

        unsigned int mDynamicCount;

        /// Validate entries in store after setup
        void validate();

    public:
        /// \todo replace with SharedIterator<StoreBase>
        typedef std::map<int, StoreBase *>::const_iterator iterator;

        iterator begin() const {
            return mStores.begin();
        }

        iterator end() const {
            return mStores.end();
        }

        /// Look up the given ID in 'all'. Returns 0 if not found.
        /// \note id must be in lower case.
        int find(const std::string &id) const
        {
            std::map<std::string, int>::const_iterator it = mIds.find(id);
            if (it == mIds.end()) {
                return 0;
            }
            return it->second;
        }

        ESMStore()
          : mDynamicCount(0)
        {
            mStores[ESM::REC_ACTI] = &mActivators;
            mStores[ESM::REC_ALCH] = &mPotions;
            mStores[ESM::REC_APPA] = &mAppas;
            mStores[ESM::REC_ARMO] = &mArmors;
            mStores[ESM::REC_BODY] = &mBodyParts;
            mStores[ESM::REC_BOOK] = &mBooks;
            mStores[ESM::REC_BSGN] = &mBirthSigns;
            mStores[ESM::REC_CELL] = &mCells;
            mStores[ESM::REC_CLAS] = &mClasses;
            mStores[ESM::REC_CLOT] = &mClothes;
            mStores[ESM::REC_CONT] = &mContainers;
            mStores[ESM::REC_CREA] = &mCreatures;
            mStores[ESM::REC_DIAL] = &mDialogs;
            mStores[ESM::REC_DOOR] = &mDoors;
            mStores[ESM::REC_ENCH] = &mEnchants;
            mStores[ESM::REC_FACT] = &mFactions;
            mStores[ESM::REC_GLOB] = &mGlobals;
            mStores[ESM::REC_GMST] = &mGameSettings;
            mStores[ESM::REC_INGR] = &mIngreds;
            mStores[ESM::REC_LAND] = &mLands;
            mStores[ESM::REC_LEVC] = &mCreatureLists;
            mStores[ESM::REC_LEVI] = &mItemLists;
            mStores[ESM::REC_LIGH] = &mLights;
            mStores[ESM::REC_LOCK] = &mLockpicks;
            mStores[ESM::REC_LTEX] = &mLandTextures;
            mStores[ESM::REC_MISC] = &mMiscItems;
            mStores[ESM::REC_NPC_] = &mNpcs;
            mStores[ESM::REC_PGRD] = &mPathgrids;
            mStores[ESM::REC_PROB] = &mProbes;
            mStores[ESM::REC_RACE] = &mRaces;
            mStores[ESM::REC_REGN] = &mRegions;
            mStores[ESM::REC_REPA] = &mRepairs;
            mStores[ESM::REC_SCPT] = &mScripts;
            mStores[ESM::REC_SNDG] = &mSoundGens;
            mStores[ESM::REC_SOUN] = &mSounds;
            mStores[ESM::REC_SPEL] = &mSpells;
            mStores[ESM::REC_SSCR] = &mStartScripts;
            mStores[ESM::REC_STAT] = &mStatics;
            mStores[ESM::REC_WEAP] = &mWeapons;

            mPathgrids.setCells(mCells);
        }

        void clearDynamic ()
        {
            for (std::map<int, StoreBase *>::iterator it = mStores.begin(); it != mStores.end(); ++it)
                it->second->clearDynamic();

            mNpcs.insert(mPlayerTemplate);
        }

        void movePlayerRecord ()
        {
            mPlayerTemplate = *mNpcs.find("player");
            mNpcs.eraseStatic(mPlayerTemplate.mId);
            mNpcs.insert(mPlayerTemplate);
        }

        void load(ESM::ESMReader &esm, Loading::Listener* listener);

        template <class T>
        const Store<T> &get() const {
            throw std::runtime_error("Storage for this type not exist");
        }

        /// Insert a custom record (i.e. with a generated ID that will not clash will pre-existing records)
        template <class T>
        const T *insert(const T &x)
        {
            const std::string id = "$dynamic" + std::to_string(mDynamicCount++);

            Store<T> &store = const_cast<Store<T> &>(get<T>());
            if (store.search(id) != 0)
            {
                const std::string msg = "Try to override existing record '" + id + "'";
                throw std::runtime_error(msg);
            }
            T record = x;

            record.mId = id;

            T *ptr = store.insert(record);
            for (iterator it = mStores.begin(); it != mStores.end(); ++it) {
                if (it->second == &store) {
                    mIds[ptr->mId] = it->first;
                }
            }
            return ptr;
        }

        /// Insert a record with set ID, and allow it to override a pre-existing static record.
        template <class T>
        const T *overrideRecord(const T &x) {
            Store<T> &store = const_cast<Store<T> &>(get<T>());

            T *ptr = store.insert(x);
            for (iterator it = mStores.begin(); it != mStores.end(); ++it) {
                if (it->second == &store) {
                    mIds[ptr->mId] = it->first;
                }
            }
            return ptr;
        }

        template <class T>
        const T *insertStatic(const T &x)
        {
            const std::string id = "$dynamic" + std::to_string(mDynamicCount++);

            Store<T> &store = const_cast<Store<T> &>(get<T>());
            if (store.search(id) != 0)
            {
                const std::string msg = "Try to override existing record '" + id + "'";
                throw std::runtime_error(msg);
            }
            T record = x;

            T *ptr = store.insertStatic(record);
            for (iterator it = mStores.begin(); it != mStores.end(); ++it) {
                if (it->second == &store) {
                    mIds[ptr->mId] = it->first;
                }
            }
            return ptr;
        }

        // This method must be called once, after loading all master/plugin files. This can only be done
        //  from the outside, so it must be public.
        void setUp(bool validateRecords = false);

        int countSavedGameRecords() const;

        void write (ESM::ESMWriter& writer, Loading::Listener& progress) const;

        bool readRecord (ESM::ESMReader& reader, uint32_t type);
        ///< \return Known type?
    };

    template <>
    inline const ESM::Cell *ESMStore::insert<ESM::Cell>(const ESM::Cell &cell) {
        return mCells.insert(cell);
    }

    template <>
    inline const ESM::NPC *ESMStore::insert<ESM::NPC>(const ESM::NPC &npc)
    {
        const std::string id = "$dynamic" + std::to_string(mDynamicCount++);

        if (Misc::StringUtils::ciEqual(npc.mId, "player"))
        {
            return mNpcs.insert(npc);
        }
        else if (mNpcs.search(id) != 0)
        {
            const std::string msg = "Try to override existing record '" + id + "'";
            throw std::runtime_error(msg);
        }
        ESM::NPC record = npc;

        record.mId = id;

        ESM::NPC *ptr = mNpcs.insert(record);
        mIds[ptr->mId] = ESM::REC_NPC_;
        return ptr;
    }

    template <>
    inline const Store<ESM::Activator> &ESMStore::get<ESM::Activator>() const {
        return mActivators;
    }

    template <>
    inline const Store<ESM::Potion> &ESMStore::get<ESM::Potion>() const {
        return mPotions;
    }

    template <>
    inline const Store<ESM::Apparatus> &ESMStore::get<ESM::Apparatus>() const {
        return mAppas;
    }

    template <>
    inline const Store<ESM::Armor> &ESMStore::get<ESM::Armor>() const {
        return mArmors;
    }

    template <>
    inline const Store<ESM::BodyPart> &ESMStore::get<ESM::BodyPart>() const {
        return mBodyParts;
    }

    template <>
    inline const Store<ESM::Book> &ESMStore::get<ESM::Book>() const {
        return mBooks;
    }

    template <>
    inline const Store<ESM::BirthSign> &ESMStore::get<ESM::BirthSign>() const {
        return mBirthSigns;
    }

    template <>
    inline const Store<ESM::Class> &ESMStore::get<ESM::Class>() const {
        return mClasses;
    }

    template <>
    inline const Store<ESM::Clothing> &ESMStore::get<ESM::Clothing>() const {
        return mClothes;
    }

    template <>
    inline const Store<ESM::Container> &ESMStore::get<ESM::Container>() const {
        return mContainers;
    }

    template <>
    inline const Store<ESM::Creature> &ESMStore::get<ESM::Creature>() const {
        return mCreatures;
    }

    template <>
    inline const Store<ESM::Dialogue> &ESMStore::get<ESM::Dialogue>() const {
        return mDialogs;
    }

    template <>
    inline const Store<ESM::Door> &ESMStore::get<ESM::Door>() const {
        return mDoors;
    }

    template <>
    inline const Store<ESM::Enchantment> &ESMStore::get<ESM::Enchantment>() const {
        return mEnchants;
    }

    template <>
    inline const Store<ESM::Faction> &ESMStore::get<ESM::Faction>() const {
        return mFactions;
    }

    template <>
    inline const Store<ESM::Global> &ESMStore::get<ESM::Global>() const {
        return mGlobals;
    }

    template <>
    inline const Store<ESM::Ingredient> &ESMStore::get<ESM::Ingredient>() const {
        return mIngreds;
    }

    template <>
    inline const Store<ESM::CreatureLevList> &ESMStore::get<ESM::CreatureLevList>() const {
        return mCreatureLists;
    }

    template <>
    inline const Store<ESM::ItemLevList> &ESMStore::get<ESM::ItemLevList>() const {
        return mItemLists;
    }

    template <>
    inline const Store<ESM::Light> &ESMStore::get<ESM::Light>() const {
        return mLights;
    }

    template <>
    inline const Store<ESM::Lockpick> &ESMStore::get<ESM::Lockpick>() const {
        return mLockpicks;
    }

    template <>
    inline const Store<ESM::Miscellaneous> &ESMStore::get<ESM::Miscellaneous>() const {
        return mMiscItems;
    }

    template <>
    inline const Store<ESM::NPC> &ESMStore::get<ESM::NPC>() const {
        return mNpcs;
    }

    template <>
    inline const Store<ESM::Probe> &ESMStore::get<ESM::Probe>() const {
        return mProbes;
    }

    template <>
    inline const Store<ESM::Race> &ESMStore::get<ESM::Race>() const {
        return mRaces;
    }

    template <>
    inline const Store<ESM::Region> &ESMStore::get<ESM::Region>() const {
        return mRegions;
    }

    template <>
    inline const Store<ESM::Repair> &ESMStore::get<ESM::Repair>() const {
        return mRepairs;
    }

    template <>
    inline const Store<ESM::SoundGenerator> &ESMStore::get<ESM::SoundGenerator>() const {
        return mSoundGens;
    }

    template <>
    inline const Store<ESM::Sound> &ESMStore::get<ESM::Sound>() const {
        return mSounds;
    }

    template <>
    inline const Store<ESM::Spell> &ESMStore::get<ESM::Spell>() const {
        return mSpells;
    }

    template <>
    inline const Store<ESM::StartScript> &ESMStore::get<ESM::StartScript>() const {
        return mStartScripts;
    }

    template <>
    inline const Store<ESM::Static> &ESMStore::get<ESM::Static>() const {
        return mStatics;
    }

    template <>
    inline const Store<ESM::Weapon> &ESMStore::get<ESM::Weapon>() const {
        return mWeapons;
    }

    template <>
    inline const Store<ESM::GameSetting> &ESMStore::get<ESM::GameSetting>() const {
        return mGameSettings;
    }

    template <>
    inline const Store<ESM::Script> &ESMStore::get<ESM::Script>() const {
        return mScripts;
    }

    template <>
    inline const Store<ESM::Cell> &ESMStore::get<ESM::Cell>() const {
        return mCells;
    }

    template <>
    inline const Store<ESM::Land> &ESMStore::get<ESM::Land>() const {
        return mLands;
    }

    template <>
    inline const Store<ESM::LandTexture> &ESMStore::get<ESM::LandTexture>() const {
        return mLandTextures;
    }

    template <>
    inline const Store<ESM::Pathgrid> &ESMStore::get<ESM::Pathgrid>() const {
        return mPathgrids;
    }

    template <>
    inline const Store<ESM::MagicEffect> &ESMStore::get<ESM::MagicEffect>() const {
        return mMagicEffects;
    }

    template <>
    inline const Store<ESM::Skill> &ESMStore::get<ESM::Skill>() const {
        return mSkills;
    }

    template <>
    inline const Store<ESM::Attribute> &ESMStore::get<ESM::Attribute>() const {
        return mAttributes;
    }
}

#endif
