#ifndef OPENMW_MWWORLD_ESMSTORE_H
#define OPENMW_MWWORLD_ESMSTORE_H

#include <sstream>
#include <stdexcept>

#include <components/esm/records.hpp>
#include <extern/esm4/common.hpp>

#include "store.hpp"

namespace Loading
{
    class Listener;
}

namespace MWWorld
{
    class ESMStore
    {
        ESM4Store<ESM4::Book>            mBooks2;
        ESM4Store<ESM4::Activator>     mActivators2;
        ESM4Store<ESM4::Potion>     mPotions2;
        ESM4Store<ESM4::Apparatus>       mAppas2;
        ESM4Store<ESM4::Armor>           mArmors2;
        ESM4Store<ESM4::BodyPart>        mBodyParts2;
        ESM4Store<ESM4::Class>           mClasses2;
        ESM4Store<ESM4::Clothing>        mClothes2;
        ESM4Store<ESM4::Container>       mContainers2;
        ESM4Store<ESM4::Creature>        mCreatures2;
        ESM4Store<ESM4::Door>            mDoors2;
        ESM4Store<ESM4::Ingredient>      mIngreds2;
        ESM4Store<ESM4::Light>           mLights2;
        ESM4Store<ESM4::Race>            mRaces2;
        ESM4Store<ESM4::Region>          mRegions2;
        ESM4Store<ESM4::Sound>           mSounds2;
        ESM4Store<ESM4::Static>          mStatics2;
        ESM4Store<ESM4::Weapon>          mWeapons2;
        ESM4Store<ESM4::Script>          mScripts2;

        // Lists that need special rules
        ESM4Store<ESM4::Cell>        mCells2;
        ESM4Store<ESM4::Land>        mLands2;
        ESM4Store<ESM4::LandTexture> mLandTextures2;
        ESM4Store<ESM4::Npc>        mNpcs2;
        ESM4Store<ESM4::Hair>       mHairs2;
        ESM4Store<ESM4::HeadPart>   mHeadParts2;
        ESM4Store<ESM4::Quest> mQuests2;


        ESM4Store<ESM4::BirthSign>   mBirthSigns2;
        ESM4Store<ESM4::Dialog>   mDialogs2;
        ESM4Store<ESM4::DialogBranch>   mDialogBranchs2;
        ESM4Store<ESM4::Info>   mDialogInfo2;
        //ESM4Store<ESM4::DialogView>   mDialogViews2;
        ESM4Store<ESM4::ActorCharacter>   mActorCharacters2;
        ESM4Store<ESM4::ActorCreature>   mActorCreatures2;
        ESM4Store<ESM4::Grass> mGrasses2;
        ESM4Store<ESM4::Flora> mFloras2;
        ESM4Store<ESM4::Ammo> mAmmos2;
        ESM4Store<ESM4::AnimObject> mAnimObjects2;
        ESM4Store<ESM4::Eyes> mEyes2;
        ESM4Store<ESM4::Global>   mGlobals2;
        ESM4Store<ESM4::Note>   mNotes2;
        ESM4Store<ESM4::ArmorAddon>   mArmorAddons2;
        ESM4Store<ESM4::Furniture>   mFurnitures2;
        ESM4Store<ESM4::Faction>   mFactions2;
ESM4Store<ESM4::Reference> mPersistantrefs2;

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
        std::map<int, StoreBase *> mESM4Stores;

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
        iterator beginESM4() const {
            return mESM4Stores.begin();
        }

        iterator endESM4() const {
            return mESM4Stores.end();
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


            mESM4Stores[ESM4::REC_DIAL] = &mDialogs2;

            mESM4Stores[ESM4::REC_BSGN] = &mBirthSigns2;

            mESM4Stores[ESM4::REC_BOOK] = &mBooks2;
            mESM4Stores[ESM4::REC_ACTI] = &mActivators2;
            mESM4Stores[ESM4::REC_ALCH] = &mPotions2;
            mESM4Stores[ESM4::REC_APPA] = &mAppas2;
            mESM4Stores[ESM4::REC_ARMO] = &mArmors2;
            mESM4Stores[ESM4::REC_CELL] = &mCells2;
            mESM4Stores[ESM4::REC_CLAS] = &mClasses2;
            mESM4Stores[ESM4::REC_CLOT] = &mClothes2;
            mESM4Stores[ESM4::REC_CONT] = &mContainers2;
            mESM4Stores[ESM4::REC_CREA] = &mCreatures2;
            mESM4Stores[ESM4::REC_DOOR] = &mDoors2;
           /// mESM4Stores[ESM4::REC_ENCH] = &mEnchants2;
           /// mESM4Stores[ESM4::REC_FACT] = &mFactions2;
          ///  mESM4Stores[ESM4::REC_GLOB] = &mGlobals2;
          //  mESM4Stores[ESM4::REC_GMST] = &mGameSettings;
            mESM4Stores[ESM4::REC_INGR] = &mIngreds2;
            mESM4Stores[ESM4::REC_LAND] = &mLands2;
           // mESM4Stores[ESM4::REC_LEVC] = &mCreatureLists;
          //  mESM4Stores[ESM4::REC_LEVI] = &mItemLists;
            mESM4Stores[ESM4::REC_LIGH] = &mLights2;
           // mESM4Stores[ESM4::REC_LOCK] = &mLockpicks;
            mESM4Stores[ESM4::REC_LTEX] = &mLandTextures2;
          ///  mESM4Stores[ESM4::REC_MISC] = &mMiscItems2;
          ///
            mESM4Stores[ESM4::REC_DIAL] = &mDialogs2;
            mESM4Stores[ESM4::REC_DLBR] = &mDialogBranchs2;
            mESM4Stores[ESM4::REC_INFO] = &mDialogInfo2;
            mESM4Stores[ESM4::REC_ACHR] = &mActorCharacters2;
            mESM4Stores[ESM4::REC_ACRE] = &mActorCreatures2;
           // mESM4Stores[ESM4::REC_DLVW] = &mDialogViews2;


            mESM4Stores[ESM4::REC_HDPT] = &mHeadParts2;
          mESM4Stores[ESM4::REC_HAIR] = &mHairs2;
          mESM4Stores[ESM4::REC_NPC_] = &mNpcs2;
          mESM4Stores[ESM4::REC_QUST] = &mQuests2;
          mESM4Stores[ESM4::REC_GRAS] = &mGrasses2;
          mESM4Stores[ESM4::REC_FLOR] = &mFloras2;
          mESM4Stores[ESM4::REC_AMMO] = &mAmmos2;
          mESM4Stores[ESM4::REC_ANIO] = &mAnimObjects2;
          mESM4Stores[ESM4::REC_EYES] = &mEyes2;
          mESM4Stores[ESM4::REC_GLOB] = &mGlobals2;
          mESM4Stores[ESM4::REC_NOTE] = &mNotes2;

         ///   mESM4Stores[ESM4::REC_PGRD] = &mPathgrids2;
          //  mESM4Stores[ESM4::REC_PROB] = &mProbes;
            mESM4Stores[ESM4::REC_RACE] = &mRaces2;
            mESM4Stores[ESM4::REC_REGN] = &mRegions2;
           // mESM4Stores[ESM4::REC_REPA] = &mRepairs;
            mESM4Stores[ESM4::REC_SCPT] = &mScripts2;
          //  mESM4Stores[ESM4::REC_SNDG] = &mSoundGens;
            mESM4Stores[ESM4::REC_SOUN] = &mSounds2;
           /// mESM4Stores[ESM4::REC_SPEL] = &mSpells2;
            //mESM4Stores[ESM::REC_SSCR] = &mStartScripts2;
            mESM4Stores[ESM4::REC_STAT] = &mStatics2;
            mESM4Stores[ESM4::REC_WEAP] = &mWeapons2;
            mESM4Stores[ESM4::REC_ARMA] = &mArmorAddons2;
            mESM4Stores[ESM4::REC_FURN] = &mFurnitures2;
            mESM4Stores[ESM4::REC_LIGH] = &mLights2;
            mESM4Stores[ESM4::REC_FACT] = &mFactions2;
            mESM4Stores[ESM4::REC_REFR] = &mPersistantrefs2;



            mPathgrids.setCells(mCells);
        }

        void clearDynamic ()
        {
            for (std::map<int, StoreBase *>::iterator it = mStores.begin(); it != mStores.end(); ++it)
                it->second->clearDynamic();

            for (std::map<int, StoreBase *>::iterator it = mESM4Stores.begin(); it != mESM4Stores.end(); ++it)
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

        template <class T>
        const ESM4Store<T> &getESM4() const {
            throw std::runtime_error("ESM4Store for this type not exist");
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
            for (iterator it = mESM4Stores.begin(); it != mESM4Stores.end(); ++it) {
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
            for (iterator it = mESM4Stores.begin(); it != mESM4Stores.end(); ++it) {
                if (it->second == &store) {
                    mIds[ptr->mId] = it->first;
                }
            }
            return ptr;
        }

        void loadTes4Record (ESM::ESMReader& esm);
        // Can't use ESM4::Reader& as the parameter here because we need esm.hasMoreRecs() for
        // checking an empty group followed by EOF
        void loadTes4Group (ESM::ESMReader &esm);
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

    ///ESM4
    template <>
    inline const ESM4Store<ESM4::Activator> &ESMStore::getESM4<ESM4::Activator>() const {
        return mActivators2;
    }

    template <>
    inline const ESM4Store<ESM4::Potion> &ESMStore::getESM4<ESM4::Potion>() const {
        return mPotions2;
    }

    template <>
    inline const ESM4Store<ESM4::Apparatus> &ESMStore::getESM4<ESM4::Apparatus>() const {
        return mAppas2;
    }

    template <>
    inline const ESM4Store<ESM4::Armor> &ESMStore::getESM4<ESM4::Armor>() const {
        return mArmors2;
    }

    template <>
    inline const ESM4Store<ESM4::BodyPart> &ESMStore::getESM4<ESM4::BodyPart>() const {
        return mBodyParts2;
    }

    template <>
    inline const ESM4Store<ESM4::Book> &ESMStore::getESM4<ESM4::Book>() const {
        return mBooks2;
    }



    template <>
    inline const ESM4Store<ESM4::Class> &ESMStore::getESM4<ESM4::Class>() const {
        return mClasses2;
    }

    template <>
    inline const ESM4Store<ESM4::Clothing> &ESMStore::getESM4<ESM4::Clothing>() const {
        return mClothes2;
    }

    template <>
    inline const ESM4Store<ESM4::Container> &ESMStore::getESM4<ESM4::Container>() const {
        return mContainers2;
    }

    template <>
    inline const ESM4Store<ESM4::Creature> &ESMStore::getESM4<ESM4::Creature>() const {
        return mCreatures2;
    }

    template <>
    inline const ESM4Store<ESM4::Door> &ESMStore::getESM4<ESM4::Door>() const {
        return mDoors2;
    }


    template <>
    inline const ESM4Store<ESM4::Ingredient> &ESMStore::getESM4<ESM4::Ingredient>() const {
        return mIngreds2;
    }


    template <>
    inline const ESM4Store<ESM4::Light> &ESMStore::getESM4<ESM4::Light>() const {
        return mLights2;
    }


    template <>
    inline const ESM4Store<ESM4::Race> &ESMStore::getESM4<ESM4::Race>() const {
        return mRaces2;
    }

    template <>
    inline const ESM4Store<ESM4::Region> &ESMStore::getESM4<ESM4::Region>() const {
        return mRegions2;
    }


    template <>
    inline const ESM4Store<ESM4::Sound> &ESMStore::getESM4<ESM4::Sound>() const {
        return mSounds2;
    }


    template <>
    inline const ESM4Store<ESM4::Static> &ESMStore::getESM4<ESM4::Static>() const {
        return mStatics2;
    }

    template <>
    inline const ESM4Store<ESM4::Weapon> &ESMStore::getESM4<ESM4::Weapon>() const {
        return mWeapons2;
    }



    template <>
    inline const ESM4Store<ESM4::Script> &ESMStore::getESM4<ESM4::Script>() const {
        return mScripts2;
    }

    template <>
    inline const ESM4Store<ESM4::Cell> &ESMStore::getESM4<ESM4::Cell>() const {
        return mCells2;
    }

    template <>
    inline const ESM4Store<ESM4::Land> &ESMStore::getESM4<ESM4::Land>() const {
        return mLands2;
    }

    template <>
    inline const ESM4Store<ESM4::LandTexture> &ESMStore::getESM4<ESM4::LandTexture>() const {
        return mLandTextures2;
    }

    template <>
    inline const ESM4Store<ESM4::Npc> &ESMStore::getESM4<ESM4::Npc>() const {
        return mNpcs2;
    }

    template <>
    inline const ESM4Store<ESM4::BirthSign> &ESMStore::getESM4<ESM4::BirthSign>() const {
        return mBirthSigns2;
    }
    template <>
    inline const ESM4Store<ESM4::Dialog> &ESMStore::getESM4<ESM4::Dialog>() const {
        return mDialogs2;
    }
    template <>
    inline const ESM4Store<ESM4::DialogBranch> &ESMStore::getESM4<ESM4::DialogBranch>() const {
        return mDialogBranchs2;
    }
    template <>
    inline const ESM4Store<ESM4::Hair> &ESMStore::getESM4<ESM4::Hair>() const {
        return mHairs2;
    }
    template <>
    inline const ESM4Store<ESM4::HeadPart> &ESMStore::getESM4<ESM4::HeadPart>() const {
        return mHeadParts2;
    }

    template <>
    inline const ESM4Store<ESM4::Quest> &ESMStore::getESM4<ESM4::Quest>() const {
        return mQuests2;
    }
    template <>
    inline const ESM4Store<ESM4::Info> &ESMStore::getESM4<ESM4::Info>() const {
        return mDialogInfo2;
    }
    template <>
    inline const ESM4Store<ESM4::ActorCharacter> &ESMStore::getESM4<ESM4::ActorCharacter>() const {
        return mActorCharacters2;
    }

    template <>
    inline const ESM4Store<ESM4::Grass> &ESMStore::getESM4<ESM4::Grass>() const {
        return mGrasses2;
    }
    template <>
    inline const ESM4Store<ESM4::Flora> &ESMStore::getESM4<ESM4::Flora>() const {
        return mFloras2;
    }
    template <>
    inline const ESM4Store<ESM4::Ammo> &ESMStore::getESM4<ESM4::Ammo>() const {
        return mAmmos2;
    }
    template <>
    inline const ESM4Store<ESM4::AnimObject> &ESMStore::getESM4<ESM4::AnimObject>() const {
        return mAnimObjects2;
    }
    template <>
    inline const ESM4Store<ESM4::Eyes> &ESMStore::getESM4<ESM4::Eyes>() const {
        return mEyes2;
    }
    template <>
    inline const ESM4Store<ESM4::Global> &ESMStore::getESM4<ESM4::Global>() const {
        return mGlobals2;
    }

    template <>
    inline const ESM4Store<ESM4::Note> &ESMStore::getESM4<ESM4::Note>() const {
        return mNotes2;
    }

    template <>
    inline const ESM4Store<ESM4::ArmorAddon> &ESMStore::getESM4<ESM4::ArmorAddon>() const {
        return mArmorAddons2;
    }

    template <>
    inline const ESM4Store<ESM4::Furniture> &ESMStore::getESM4<ESM4::Furniture>() const {
        return mFurnitures2;
    }

    template <>
    inline const ESM4Store<ESM4::Faction> &ESMStore::getESM4<ESM4::Faction>() const {
        return mFactions2;
    }
    template <>
    inline const ESM4Store<ESM4::Reference> &ESMStore::getESM4<ESM4::Reference>() const {
        return mPersistantrefs2;
    }

 /*  template <>
    inline const ESM4Store<ESM4::Enchantment> &ESMStore::get<ESM4::Enchantment>() const {
        return mEnchants2;
    }
    template <>
    inline const ESM4Store<ESM4::Dialogue> &ESMStore::get<ESM4::Dialogue>() const {
        return mDialogs2;
    }

    template <>
    inline const ESM4Store<ESM4::Faction> &ESMStore::get<ESM4::Faction>() const {
        return mFactions2;
    }

    template <>
    inline const ESM4Store<ESM4::Global> &ESMStore::get<ESM4::Global>() const {
        return mGlobals2;
    }
    template <>
    inline const ESM4Store<ESM4::CreatureLevList> &ESMStore::get<ESM4::CreatureLevList>() const {
        return mCreatureLists2;
    }

    template <>
    inline const ESM4Store<ESM4::ItemLevList> &ESMStore::get<ESM4::ItemLevList>() const {
        return mItemLists2;
    }

    template <>
    inline const ESM4Store<ESM4::Lockpick> &ESMStore::get<ESM4::Lockpick>() const {
        return mLockpicks2;
    }

    template <>
    inline const ESM4Store<ESM4::Miscellaneous> &ESMStore::get<ESM4::Miscellaneous>() const {
        return mMiscItems2;
    }

    template <>
    inline const ESM4Store<ESM4::Probe> &ESMStore::get<ESM4::Probe>() const {
        return mProbes2;
    }
    template <>
    inline const ESM4Store<ESM4::Repair> &ESMStore::get<ESM4::Repair>() const {
        return mRepairs2;
    }

    template <>
    inline const ESM4Store<ESM4::SoundGenerator> &ESMStore::get<ESM4::SoundGenerator>() const {
        return mSoundGens2;
    }

    template <>
    inline const ESM4Store<ESM4::Spell> &ESMStore::get<ESM4::Spell>() const {
        return mSpells2;
    }

    template <>
    inline const ESM4Store<ESM4::StartScript> &ESMStore::get<ESM4::StartScript>() const {
        return mStartScripts2;
    }
    template <>
    inline const ESM4Store<ESM4::GameSetting> &ESMStore::getESM4<ESM4::GameSetting>() const {
        return mGameSettings2;
    }
    template <>
    inline const ESM4Store<ESM4::Pathgrid> &ESMStore::getESM4<ESM4::Pathgrid>() const {
        return mPathgrids2;
    }

    template <>
    inline const ESM4Store<ESM4::MagicEffect> &ESMStore::getESM4<ESM4::MagicEffect>() const {
        return mMagicEffects2;
    }

    template <>
    inline const ESM4Store<ESM4::Skill> &ESMStore::getESM4<ESM4::Skill>() const {
        return mSkills2;
    }

    template <>
    inline const ESM4Store<ESM4::Attribute> &ESMStore::getESM4<ESM4::Attribute>() const {
        return mAttributes2;
    }*/
}

#endif
