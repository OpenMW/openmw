#include "esmstore.hpp"

#include <algorithm>
#include <fstream>

#include <components/debug/debuglog.hpp>
#include <components/esm3/esmreader.hpp>
#include <components/esm3/esmwriter.hpp>
#include <components/loadinglistener/loadinglistener.hpp>
#include <components/lua/configuration.hpp>
#include <components/misc/algorithm.hpp>
#include <components/esm3/readerscache.hpp>
#include <components/esmloader/load.hpp>

#include "../mwmechanics/spelllist.hpp"

namespace
{
    struct Ref
    {
        ESM::RefNum mRefNum;
        std::size_t mRefID;

        Ref(ESM::RefNum refNum, std::size_t refID) : mRefNum(refNum), mRefID(refID) {}
    };

    constexpr std::size_t deletedRefID = std::numeric_limits<std::size_t>::max();

    void readRefs(const ESM::Cell& cell, std::vector<Ref>& refs, std::vector<std::string>& refIDs, ESM::ReadersCache& readers)
    {
        // TODO: we have many similar copies of this code.
        for (size_t i = 0; i < cell.mContextList.size(); i++)
        {
            const std::size_t index = static_cast<std::size_t>(cell.mContextList[i].index);
            const ESM::ReadersCache::BusyItem reader = readers.get(index);
            cell.restore(*reader, i);
            ESM::CellRef ref;
            ref.mRefNum.unset();
            bool deleted = false;
            while (cell.getNextRef(*reader, ref, deleted))
            {
                if(deleted)
                    refs.emplace_back(ref.mRefNum, deletedRefID);
                else if (std::find(cell.mMovedRefs.begin(), cell.mMovedRefs.end(), ref.mRefNum) == cell.mMovedRefs.end())
                {
                    refs.emplace_back(ref.mRefNum, refIDs.size());
                    refIDs.push_back(std::move(ref.mRefID));
                }
            }
        }
        for(const auto& [value, deleted] : cell.mLeasedRefs)
        {
            if(deleted)
                refs.emplace_back(value.mRefNum, deletedRefID);
            else
            {
                refs.emplace_back(value.mRefNum, refIDs.size());
                refIDs.push_back(value.mRefID);
            }
        }
    }

    const std::string& getDefaultClass(const MWWorld::Store<ESM::Class>& classes)
    {
        auto it = classes.begin();
        if (it != classes.end())
            return it->mId;
        throw std::runtime_error("List of NPC classes is empty!");
    }

    std::vector<ESM::NPC> getNPCsToReplace(const MWWorld::Store<ESM::Faction>& factions, const MWWorld::Store<ESM::Class>& classes, const std::unordered_map<std::string, ESM::NPC, Misc::StringUtils::CiHash, Misc::StringUtils::CiEqual>& npcs)
    {
        // Cache first class from store - we will use it if current class is not found
        const std::string& defaultCls = getDefaultClass(classes);

        // Validate NPCs for non-existing class and faction.
        // We will replace invalid entries by fixed ones
        std::vector<ESM::NPC> npcsToReplace;

        for (const auto& npcIter : npcs)
        {
            ESM::NPC npc = npcIter.second;
            bool changed = false;

            const std::string& npcFaction = npc.mFaction;
            if (!npcFaction.empty())
            {
                const ESM::Faction *fact = factions.search(npcFaction);
                if (!fact)
                {
                    Log(Debug::Verbose) << "NPC '" << npc.mId << "' (" << npc.mName << ") has nonexistent faction '" << npc.mFaction << "', ignoring it.";
                    npc.mFaction.clear();
                    npc.mNpdt.mRank = 0;
                    changed = true;
                }
            }

            const std::string& npcClass = npc.mClass;
            const ESM::Class *cls = classes.search(npcClass);
            if (!cls)
            {
                Log(Debug::Verbose) << "NPC '" << npc.mId << "' (" << npc.mName << ") has nonexistent class '" << npc.mClass << "', using '" << defaultCls << "' class as replacement.";
                npc.mClass = defaultCls;
                changed = true;
            }

            if (changed)
                npcsToReplace.push_back(npc);
        }

        return npcsToReplace;
    }

    // Custom enchanted items can reference scripts that no longer exist, this doesn't necessarily mean the base item no longer exists however.
    // So instead of removing the item altogether, we're only removing the script.
    template<class MapT>
    void removeMissingScripts(const MWWorld::Store<ESM::Script>& scripts, MapT& items)
    {
        for(auto& [id, item] : items)
        {
            if(!item.mScript.empty() && !scripts.search(item.mScript))
            {
                item.mScript.clear();
                Log(Debug::Verbose) << "Item '" << id << "' (" << item.mName << ") has nonexistent script '" << item.mScript << "', ignoring it.";
            }
        }
    }
}

namespace MWWorld
{

    struct ESMStoreImp
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

        template<typename T> 
        static const T* ESM3StoreInsert(ESMStore& stores, const T &toInsert)
        {
            const std::string id = "$dynamic" + std::to_string(stores.mDynamicCount++);

            Store<T> &store = const_cast<Store<T> &>(stores.get<T>());
            if (store.search(id) != nullptr)
            {
                const std::string msg = "Try to override existing record '" + id + "'";
                throw std::runtime_error(msg);
            }
            T record = toInsert;

            record.mId = id;

            T *ptr = store.insert(record);
            for (ESMStore::iterator it = stores.mStores.begin(); it != stores.mStores.end(); ++it) {
                if (it->second == &store) {
                    stores.mIds[ptr->mId] = it->first;
                }
            }
            return ptr;
        }
    };

    ESMStore::ESMStore()
    {
        mStoreImp = std::make_unique<ESMStoreImp>();
        mDynamicCount = 0;
        mStores[ESM::REC_ACTI] = &mStoreImp->mActivators;
        mStores[ESM::REC_ALCH] = &mStoreImp->mPotions;
        mStores[ESM::REC_APPA] = &mStoreImp->mAppas;
        mStores[ESM::REC_ARMO] = &mStoreImp->mArmors;
        mStores[ESM::REC_BODY] = &mStoreImp->mBodyParts;
        mStores[ESM::REC_BOOK] = &mStoreImp->mBooks;
        mStores[ESM::REC_BSGN] = &mStoreImp->mBirthSigns;
        mStores[ESM::REC_CELL] = &mStoreImp->mCells;
        mStores[ESM::REC_CLAS] = &mStoreImp->mClasses;
        mStores[ESM::REC_CLOT] = &mStoreImp->mClothes;
        mStores[ESM::REC_CONT] = &mStoreImp->mContainers;
        mStores[ESM::REC_CREA] = &mStoreImp->mCreatures;
        mStores[ESM::REC_DIAL] = &mStoreImp->mDialogs;
        mStores[ESM::REC_DOOR] = &mStoreImp->mDoors;
        mStores[ESM::REC_ENCH] = &mStoreImp->mEnchants;
        mStores[ESM::REC_FACT] = &mStoreImp->mFactions;
        mStores[ESM::REC_GLOB] = &mStoreImp->mGlobals;
        mStores[ESM::REC_GMST] = &mStoreImp->mGameSettings;
        mStores[ESM::REC_INGR] = &mStoreImp->mIngreds;
        mStores[ESM::REC_LAND] = &mStoreImp->mLands;
        mStores[ESM::REC_LEVC] = &mStoreImp->mCreatureLists;
        mStores[ESM::REC_LEVI] = &mStoreImp->mItemLists;
        mStores[ESM::REC_LIGH] = &mStoreImp->mLights;
        mStores[ESM::REC_LOCK] = &mStoreImp->mLockpicks;
        mStores[ESM::REC_LTEX] = &mStoreImp->mLandTextures;
        mStores[ESM::REC_MISC] = &mStoreImp->mMiscItems;
        mStores[ESM::REC_NPC_] = &mStoreImp->mNpcs;
        mStores[ESM::REC_PGRD] = &mStoreImp->mPathgrids;
        mStores[ESM::REC_PROB] = &mStoreImp->mProbes;
        mStores[ESM::REC_RACE] = &mStoreImp->mRaces;
        mStores[ESM::REC_REGN] = &mStoreImp->mRegions;
        mStores[ESM::REC_REPA] = &mStoreImp->mRepairs;
        mStores[ESM::REC_SCPT] = &mStoreImp->mScripts;
        mStores[ESM::REC_SNDG] = &mStoreImp->mSoundGens;
        mStores[ESM::REC_SOUN] = &mStoreImp->mSounds;
        mStores[ESM::REC_SPEL] = &mStoreImp->mSpells;
        mStores[ESM::REC_SSCR] = &mStoreImp->mStartScripts;
        mStores[ESM::REC_STAT] = &mStoreImp->mStatics;
        mStores[ESM::REC_WEAP] = &mStoreImp->mWeapons;

        mStoreImp->mPathgrids.setCells(mStoreImp->mCells);
    }

    ESMStore::~ESMStore()
    {
    }

static bool isCacheableRecord(int id)
{
    if (id == ESM::REC_ACTI || id == ESM::REC_ALCH || id == ESM::REC_APPA || id == ESM::REC_ARMO ||
        id == ESM::REC_BOOK || id == ESM::REC_CLOT || id == ESM::REC_CONT || id == ESM::REC_CREA ||
        id == ESM::REC_DOOR || id == ESM::REC_INGR || id == ESM::REC_LEVC || id == ESM::REC_LEVI ||
        id == ESM::REC_LIGH || id == ESM::REC_LOCK || id == ESM::REC_MISC || id == ESM::REC_NPC_ ||
        id == ESM::REC_PROB || id == ESM::REC_REPA || id == ESM::REC_STAT || id == ESM::REC_WEAP ||
        id == ESM::REC_BODY)
    {
        return true;
    }
    return false;
}

void ESMStore::load(ESM::ESMReader &esm, Loading::Listener* listener, ESM::Dialogue*& dialogue)
{
    if (listener != nullptr)
        listener->setProgressRange(::EsmLoader::fileProgress);

    // Land texture loading needs to use a separate internal store for each plugin.
    // We set the number of plugins here so we can properly verify if valid plugin
    // indices are being passed to the LandTexture Store retrieval methods.
    mStoreImp->mLandTextures.resize(esm.getIndex()+1);

    // Loop through all records
    while(esm.hasMoreRecs())
    {
        ESM::NAME n = esm.getRecName();
        esm.getRecHeader();
        if (esm.getRecordFlags() & ESM::FLAG_Ignored)
        {
            esm.skipRecord();
            continue;
        }

        // Look up the record type.
        std::map<int, StoreBase *>::iterator it = mStores.find(n.toInt());

        if (it == mStores.end()) {
            if (n.toInt() == ESM::REC_INFO) {
                if (dialogue)
                {
                    dialogue->readInfo(esm, esm.getIndex() != 0);
                }
                else
                {
                    Log(Debug::Error) << "Error: info record without dialog";
                    esm.skipRecord();
                }
            } else if (n.toInt() == ESM::REC_MGEF) {
                mStoreImp->mMagicEffects.load (esm);
            } else if (n.toInt() == ESM::REC_SKIL) {
                mStoreImp->mSkills.load (esm);
            }
            else if (n.toInt() == ESM::REC_FILT || n.toInt() == ESM::REC_DBGP)
            {
                // ignore project file only records
                esm.skipRecord();
            }
            else if (n.toInt() == ESM::REC_LUAL)
            {
                ESM::LuaScriptsCfg cfg;
                cfg.load(esm);
                cfg.adjustRefNums(esm);
                mLuaContent.push_back(std::move(cfg));
            }
            else {
                throw std::runtime_error("Unknown record: " + n.toString());
            }
        } else {
            RecordId id = it->second->load(esm);
            if (id.mIsDeleted)
            {
                it->second->eraseStatic(id.mId);
                continue;
            }

            if (n.toInt() == ESM::REC_DIAL) {
                dialogue = const_cast<ESM::Dialogue*>(mStoreImp->mDialogs.find(id.mId));
            } else {
                dialogue = nullptr;
            }
        }
        if (listener != nullptr)
            listener->setProgress(::EsmLoader::fileProgress * esm.getFileOffset() / esm.getFileSize());
    }
}

ESM::LuaScriptsCfg ESMStore::getLuaScriptsCfg() const
{
    ESM::LuaScriptsCfg cfg;
    for (const LuaContent& c : mLuaContent)
    {
        if (std::holds_alternative<std::string>(c))
        {
            // *.omwscripts are intentionally reloaded every time when `getLuaScriptsCfg` is called.
            // It is important for the `reloadlua` console command.
            try
            {
                auto file = std::ifstream(std::get<std::string>(c));
                std::string fileContent(std::istreambuf_iterator<char>(file), {});
                LuaUtil::parseOMWScripts(cfg, fileContent);
            }
            catch (std::exception& e) { Log(Debug::Error) << e.what(); }
        }
        else
        {
            const ESM::LuaScriptsCfg& addition = std::get<ESM::LuaScriptsCfg>(c);
            cfg.mScripts.insert(cfg.mScripts.end(), addition.mScripts.begin(), addition.mScripts.end());
        }
    }
    return cfg;
}

void ESMStore::setUp()
{
    mIds.clear();

    std::map<int, StoreBase *>::iterator storeIt = mStores.begin();
    for (; storeIt != mStores.end(); ++storeIt) {
        storeIt->second->setUp();

        if (isCacheableRecord(storeIt->first))
        {
            std::vector<std::string> identifiers;
            storeIt->second->listIdentifier(identifiers);

            for (std::vector<std::string>::const_iterator record = identifiers.begin(); record != identifiers.end(); ++record)
                mIds[*record] = storeIt->first;
        }
    }

    if (mStaticIds.empty())
        for (const auto& [k, v] : mIds)
            mStaticIds.emplace(Misc::StringUtils::lowerCase(k), v);

    mStoreImp->mSkills.setUp();
    mStoreImp->mMagicEffects.setUp();
    mStoreImp->mAttributes.setUp();
    mStoreImp->mDialogs.setUp();
}

void ESMStore::validateRecords(ESM::ReadersCache& readers)
{
    validate();
    countAllCellRefs(readers);
}

void ESMStore::countAllCellRefs(ESM::ReadersCache& readers)
{
    // TODO: We currently need to read entire files here again.
    // We should consider consolidating or deferring this reading.
    if(!mRefCount.empty())
        return;
    std::vector<Ref> refs;
    std::vector<std::string> refIDs;
    for(auto it = mStoreImp->mCells.intBegin(); it != mStoreImp->mCells.intEnd(); ++it)
        readRefs(*it, refs, refIDs, readers);
    for(auto it = mStoreImp->mCells.extBegin(); it != mStoreImp->mCells.extEnd(); ++it)
        readRefs(*it, refs, refIDs, readers);
    const auto lessByRefNum = [] (const Ref& l, const Ref& r) { return l.mRefNum < r.mRefNum; };
    std::stable_sort(refs.begin(), refs.end(), lessByRefNum);
    const auto equalByRefNum = [] (const Ref& l, const Ref& r) { return l.mRefNum == r.mRefNum; };
    const auto incrementRefCount = [&] (const Ref& value)
    {
        if (value.mRefID != deletedRefID)
        {
            std::string& refId = refIDs[value.mRefID];
            // We manually lower case IDs here for the time being to improve performance.
            Misc::StringUtils::lowerCaseInPlace(refId);
            ++mRefCount[std::move(refId)];
        }
    };
    Misc::forEachUnique(refs.rbegin(), refs.rend(), equalByRefNum, incrementRefCount);
}

int ESMStore::getRefCount(std::string_view id) const
{
    const std::string lowerId = Misc::StringUtils::lowerCase(id);
    auto it = mRefCount.find(lowerId);
    if(it == mRefCount.end())
        return 0;
    return it->second;
}

void ESMStore::validate()
{
    std::vector<ESM::NPC> npcsToReplace = getNPCsToReplace(mStoreImp->mFactions, mStoreImp->mClasses, mStoreImp->mNpcs.mStatic);

    for (const ESM::NPC &npc : npcsToReplace)
    {
        mStoreImp->mNpcs.eraseStatic(npc.mId);
        mStoreImp->mNpcs.insertStatic(npc);
    }

    // Validate spell effects for invalid arguments
    std::vector<ESM::Spell> spellsToReplace;
    for (ESM::Spell spell : mStoreImp->mSpells)
    {
        if (spell.mEffects.mList.empty())
            continue;

        bool changed = false;
        auto iter = spell.mEffects.mList.begin();
        while (iter != spell.mEffects.mList.end())
        {
            const ESM::MagicEffect* mgef = mStoreImp->mMagicEffects.search(iter->mEffectID);
            if (!mgef)
            {
                Log(Debug::Verbose) << "Spell '" << spell.mId << "' has an invalid effect (index " << iter->mEffectID << ") present. Dropping the effect.";
                iter = spell.mEffects.mList.erase(iter);
                changed = true;
                continue;
            }

            if (mgef->mData.mFlags & ESM::MagicEffect::TargetSkill)
            {
                if (iter->mAttribute != -1)
                {
                    iter->mAttribute = -1;
                    Log(Debug::Verbose) << ESM::MagicEffect::effectIdToString(iter->mEffectID) <<
                        " effect of spell '" << spell.mId << "' has an attribute argument present. Dropping the argument.";
                    changed = true;
                }
            }
            else if (mgef->mData.mFlags & ESM::MagicEffect::TargetAttribute)
            {
                if (iter->mSkill != -1)
                {
                    iter->mSkill = -1;
                    Log(Debug::Verbose) << ESM::MagicEffect::effectIdToString(iter->mEffectID) <<
                        " effect of spell '" << spell.mId << "' has a skill argument present. Dropping the argument.";
                    changed = true;
                }
            }
            else if (iter->mSkill != -1 || iter->mAttribute != -1)
            {
                iter->mSkill = -1;
                iter->mAttribute = -1;
                Log(Debug::Verbose) << ESM::MagicEffect::effectIdToString(iter->mEffectID) <<
                    " effect of spell '" << spell.mId << "' has argument(s) present. Dropping the argument(s).";
                changed = true;
            }

            ++iter;
        }

        if (changed)
            spellsToReplace.emplace_back(spell);
    }

    for (const ESM::Spell &spell : spellsToReplace)
    {
        mStoreImp->mSpells.eraseStatic(spell.mId);
        mStoreImp->mSpells.insertStatic(spell);
    }
}

void ESMStore::movePlayerRecord()
{
    auto player = mStoreImp->mNpcs.find("player");
    mStoreImp->mNpcs.insert(*player);
}

void ESMStore::validateDynamic()
{
    std::vector<ESM::NPC> npcsToReplace = getNPCsToReplace(mStoreImp->mFactions, mStoreImp->mClasses, mStoreImp->mNpcs.mDynamic);

    for (const ESM::NPC &npc : npcsToReplace)
        mStoreImp->mNpcs.insert(npc);

    removeMissingScripts(mStoreImp->mScripts, mStoreImp->mArmors.mDynamic);
    removeMissingScripts(mStoreImp->mScripts, mStoreImp->mBooks.mDynamic);
    removeMissingScripts(mStoreImp->mScripts, mStoreImp->mClothes.mDynamic);
    removeMissingScripts(mStoreImp->mScripts, mStoreImp->mWeapons.mDynamic);

    removeMissingObjects(mStoreImp->mCreatureLists);
    removeMissingObjects(mStoreImp->mItemLists);
}

// Leveled lists can be modified by scripts. This removes items that no longer exist (presumably because the plugin was removed) from modified lists
template<class T>
void ESMStore::removeMissingObjects(Store<T>& store)
{
    for(auto& entry : store.mDynamic)
    {
        auto first = std::remove_if(entry.second.mList.begin(), entry.second.mList.end(), [&] (const auto& item)
        {
            if(!find(item.mId))
            {
                Log(Debug::Verbose) << "Leveled list '" << entry.first << "' has nonexistent object '" << item.mId << "', ignoring it.";
                return true;
            }
            return false;
        });
        entry.second.mList.erase(first, entry.second.mList.end());
    }
}

    int ESMStore::countSavedGameRecords() const
    {
        return 1 // DYNA (dynamic name counter)
            +mStoreImp->mPotions.getDynamicSize()
            +mStoreImp->mArmors.getDynamicSize()
            +mStoreImp->mBooks.getDynamicSize()
            +mStoreImp->mClasses.getDynamicSize()
            +mStoreImp->mClothes.getDynamicSize()
            +mStoreImp->mEnchants.getDynamicSize()
            +mStoreImp->mNpcs.getDynamicSize()
            +mStoreImp->mSpells.getDynamicSize()
            +mStoreImp->mWeapons.getDynamicSize()
            +mStoreImp->mCreatureLists.getDynamicSize()
            +mStoreImp->mItemLists.getDynamicSize()
            +mStoreImp->mCreatures.getDynamicSize()
            +mStoreImp->mContainers.getDynamicSize();
    }

    void ESMStore::write (ESM::ESMWriter& writer, Loading::Listener& progress) const
    {
        writer.startRecord(ESM::REC_DYNA);
        writer.startSubRecord("COUN");
        writer.writeT(mDynamicCount);
        writer.endRecord("COUN");
        writer.endRecord(ESM::REC_DYNA);

        mStoreImp->mPotions.write (writer, progress);
        mStoreImp->mArmors.write (writer, progress);
        mStoreImp->mBooks.write (writer, progress);
        mStoreImp->mClasses.write (writer, progress);
        mStoreImp->mClothes.write (writer, progress);
        mStoreImp->mEnchants.write (writer, progress);
        mStoreImp->mSpells.write (writer, progress);
        mStoreImp->mWeapons.write (writer, progress);
        mStoreImp->mNpcs.write (writer, progress);
        mStoreImp->mItemLists.write (writer, progress);
        mStoreImp->mCreatureLists.write (writer, progress);
        mStoreImp->mCreatures.write (writer, progress);
        mStoreImp->mContainers.write (writer, progress);
    }

    bool ESMStore::readRecord (ESM::ESMReader& reader, uint32_t type)
    {
        switch (type)
        {
            case ESM::REC_ALCH:
            case ESM::REC_ARMO:
            case ESM::REC_BOOK:
            case ESM::REC_CLAS:
            case ESM::REC_CLOT:
            case ESM::REC_ENCH:
            case ESM::REC_SPEL:
            case ESM::REC_WEAP:
            case ESM::REC_LEVI:
            case ESM::REC_LEVC:
                mStores[type]->read (reader);
                return true;
            case ESM::REC_NPC_:
            case ESM::REC_CREA:
            case ESM::REC_CONT:
                mStores[type]->read (reader, true);
                return true;

            case ESM::REC_DYNA:
                reader.getSubNameIs("COUN");
                reader.getHT(mDynamicCount);
                return true;

            default:

                return false;
        }
    }

    void ESMStore::checkPlayer()
    {
        setUp();

        const ESM::NPC *player = mStoreImp->mNpcs.find ("player");

        if (!mStoreImp->mRaces.find (player->mRace) ||
            !mStoreImp->mClasses.find (player->mClass))
            throw std::runtime_error ("Invalid player record (race or class unavailable");
    }

    std::pair<std::shared_ptr<MWMechanics::SpellList>, bool> ESMStore::getSpellList(const std::string& id) const
    {
        auto result = mSpellListCache.find(id);
        std::shared_ptr<MWMechanics::SpellList> ptr;
        if (result != mSpellListCache.end())
            ptr = result->second.lock();
        if (!ptr)
        {
            int type = find(id);
            ptr = std::make_shared<MWMechanics::SpellList>(id, type);
            if (result != mSpellListCache.end())
                result->second = ptr;
            else
                mSpellListCache.insert({id, ptr});
            return {ptr, false};
        }
        return {ptr, true};
    }

    template <>
    const ESM::Cell *ESMStore::insert<ESM::Cell>(const ESM::Cell &cell) {
        return mStoreImp->mCells.insert(cell);
    }

    template <>
    const ESM::NPC *ESMStore::insert<ESM::NPC>(const ESM::NPC &npc)
    {
        const std::string id = "$dynamic" + std::to_string(mDynamicCount++);

        if (Misc::StringUtils::ciEqual(npc.mId, "player"))
        {
            return mStoreImp->mNpcs.insert(npc);
        }
        else if (mStoreImp->mNpcs.search(id) != nullptr)
        {
            const std::string msg = "Try to override existing record '" + id + "'";
            throw std::runtime_error(msg);
        }
        ESM::NPC record = npc;

        record.mId = id;

        ESM::NPC *ptr = mStoreImp->mNpcs.insert(record);
        mIds[ptr->mId] = ESM::REC_NPC_;
        return ptr;
    }

#define ESM3Insert(__Type) template<> const __Type* ESMStore::insert<__Type>(const __Type &toInsert) { return ESMStoreImp::ESM3StoreInsert(*this, toInsert);   }
    ESM3Insert(ESM::Book);
    ESM3Insert(ESM::Armor);
    ESM3Insert(ESM::Class);
    ESM3Insert(ESM::Enchantment);
    ESM3Insert(ESM::Potion);
    ESM3Insert(ESM::Weapon);
    ESM3Insert(ESM::Clothing);
    ESM3Insert(ESM::Spell);
#undef ESM3Insert

    template <>
    const Store<ESM::Activator> &ESMStore::get<ESM::Activator>() const {
        return mStoreImp->mActivators;
    }

    template <>
    const Store<ESM::Potion> &ESMStore::get<ESM::Potion>() const {
        return mStoreImp->mPotions;
    }

    template <>
    const Store<ESM::Apparatus> &ESMStore::get<ESM::Apparatus>() const {
        return mStoreImp->mAppas;
    }

    template <>
    const Store<ESM::Armor> &ESMStore::get<ESM::Armor>() const {
        return mStoreImp->mArmors;
    }

    template <>
    const Store<ESM::BodyPart> &ESMStore::get<ESM::BodyPart>() const {
        return mStoreImp->mBodyParts;
    }

    template <>
    const Store<ESM::Book> &ESMStore::get<ESM::Book>() const {
        return mStoreImp->mBooks;
    }

    template <>
    const Store<ESM::BirthSign> &ESMStore::get<ESM::BirthSign>() const {
        return mStoreImp->mBirthSigns;
    }

    template <>
    const Store<ESM::Class> &ESMStore::get<ESM::Class>() const {
        return mStoreImp->mClasses;
    }

    template <>
    const Store<ESM::Clothing> &ESMStore::get<ESM::Clothing>() const {
        return mStoreImp->mClothes;
    }

    template <>
    const Store<ESM::Container> &ESMStore::get<ESM::Container>() const {
        return mStoreImp->mContainers;
    }

    template <>
    const Store<ESM::Creature> &ESMStore::get<ESM::Creature>() const {
        return mStoreImp->mCreatures;
    }

    template <>
    const Store<ESM::Dialogue> &ESMStore::get<ESM::Dialogue>() const {
        return mStoreImp->mDialogs;
    }

    template <>
    const Store<ESM::Door> &ESMStore::get<ESM::Door>() const {
        return mStoreImp->mDoors;
    }

    template <>
    const Store<ESM::Enchantment> &ESMStore::get<ESM::Enchantment>() const {
        return mStoreImp->mEnchants;
    }

    template <>
    const Store<ESM::Faction> &ESMStore::get<ESM::Faction>() const {
        return mStoreImp->mFactions;
    }

    template <>
    const Store<ESM::Global> &ESMStore::get<ESM::Global>() const {
        return mStoreImp->mGlobals;
    }

    template <>
    const Store<ESM::Ingredient> &ESMStore::get<ESM::Ingredient>() const {
        return mStoreImp->mIngreds;
    }

    template <>
    const Store<ESM::CreatureLevList> &ESMStore::get<ESM::CreatureLevList>() const {
        return mStoreImp->mCreatureLists;
    }

    template <>
    const Store<ESM::ItemLevList> &ESMStore::get<ESM::ItemLevList>() const {
        return mStoreImp->mItemLists;
    }

    template <>
    const Store<ESM::Light> &ESMStore::get<ESM::Light>() const {
        return mStoreImp->mLights;
    }

    template <>
    const Store<ESM::Lockpick> &ESMStore::get<ESM::Lockpick>() const {
        return mStoreImp->mLockpicks;
    }

    template <>
    const Store<ESM::Miscellaneous> &ESMStore::get<ESM::Miscellaneous>() const {
        return mStoreImp->mMiscItems;
    }

    template <>
    const Store<ESM::NPC> &ESMStore::get<ESM::NPC>() const {
        return mStoreImp->mNpcs;
    }

    template <>
    const Store<ESM::Probe> &ESMStore::get<ESM::Probe>() const {
        return mStoreImp->mProbes;
    }

    template <>
    const Store<ESM::Race> &ESMStore::get<ESM::Race>() const {
        return mStoreImp->mRaces;
    }

    template <>
    const Store<ESM::Region> &ESMStore::get<ESM::Region>() const {
        return mStoreImp->mRegions;
    }

    template <>
    const Store<ESM::Repair> &ESMStore::get<ESM::Repair>() const {
        return mStoreImp->mRepairs;
    }

    template <>
    const Store<ESM::SoundGenerator> &ESMStore::get<ESM::SoundGenerator>() const {
        return mStoreImp->mSoundGens;
    }

    template <>
    const Store<ESM::Sound> &ESMStore::get<ESM::Sound>() const {
        return mStoreImp->mSounds;
    }

    template <>
    const Store<ESM::Spell> &ESMStore::get<ESM::Spell>() const {
        return mStoreImp->mSpells;
    }

    template <>
    const Store<ESM::StartScript> &ESMStore::get<ESM::StartScript>() const {
        return mStoreImp->mStartScripts;
    }

    template <>
    const Store<ESM::Static> &ESMStore::get<ESM::Static>() const {
        return mStoreImp->mStatics;
    }

    template <>
    const Store<ESM::Weapon> &ESMStore::get<ESM::Weapon>() const {
        return mStoreImp->mWeapons;
    }

    template <>
    const Store<ESM::GameSetting> &ESMStore::get<ESM::GameSetting>() const {
        return mStoreImp->mGameSettings;
    }

    template <>
    const Store<ESM::Script> &ESMStore::get<ESM::Script>() const {
        return mStoreImp->mScripts;
    }

    template <>
    const Store<ESM::Cell> &ESMStore::get<ESM::Cell>() const {
        return mStoreImp->mCells;
    }

    template <>
    const Store<ESM::Land> &ESMStore::get<ESM::Land>() const {
        return mStoreImp->mLands;
    }

    template <>
    const Store<ESM::LandTexture> &ESMStore::get<ESM::LandTexture>() const {
        return mStoreImp->mLandTextures;
    }

    template <>
    const Store<ESM::Pathgrid> &ESMStore::get<ESM::Pathgrid>() const {
        return mStoreImp->mPathgrids;
    }

    template <>
    const Store<ESM::MagicEffect> &ESMStore::get<ESM::MagicEffect>() const {
        return mStoreImp->mMagicEffects;
    }

    template <>
    const Store<ESM::Skill> &ESMStore::get<ESM::Skill>() const {
        return mStoreImp->mSkills;
    }

    template <>
    const Store<ESM::Attribute> &ESMStore::get<ESM::Attribute>() const {
        return mStoreImp->mAttributes;
    }
} // end namespace
