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
#include "storeSpecialization.hpp"

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

static int sRecordTypeCounter = 0;
template<typename T> struct SRecordType
{
    static const int recordId;
    static int getId() { return recordId; };
};

#define AddStoreType(__Type) const int SRecordType<__Type>::recordId = sRecordTypeCounter++;
#define GetRecordTypeId(__Type) SRecordType<__Type>::recordId

AddStoreType(ESM::Activator);
AddStoreType(ESM::Potion);
AddStoreType(ESM::Apparatus);
AddStoreType(ESM::Armor);
AddStoreType(ESM::BodyPart);
AddStoreType(ESM::Book);
AddStoreType(ESM::BirthSign);
AddStoreType(ESM::Class);
AddStoreType(ESM::Clothing);
AddStoreType(ESM::Container);
AddStoreType(ESM::Creature);
AddStoreType(ESM::Dialogue);
AddStoreType(ESM::Door);
AddStoreType(ESM::Enchantment);
AddStoreType(ESM::Faction);
AddStoreType(ESM::Global);
AddStoreType(ESM::Ingredient);
AddStoreType(ESM::CreatureLevList);
AddStoreType(ESM::ItemLevList);
AddStoreType(ESM::Light);
AddStoreType(ESM::Lockpick);
AddStoreType(ESM::Miscellaneous);
AddStoreType(ESM::NPC);
AddStoreType(ESM::Probe);
AddStoreType(ESM::Race);
AddStoreType(ESM::Region);
AddStoreType(ESM::Repair);
AddStoreType(ESM::SoundGenerator);
AddStoreType(ESM::Sound);
AddStoreType(ESM::Spell);
AddStoreType(ESM::StartScript);
AddStoreType(ESM::Static);
AddStoreType(ESM::Weapon);
AddStoreType(ESM::GameSetting);
AddStoreType(ESM::Script);

// Lists that need special rules
AddStoreType(ESM::Cell);
AddStoreType(ESM::Land);
AddStoreType(ESM::LandTexture);
AddStoreType(ESM::Pathgrid);

AddStoreType(ESM::MagicEffect);
AddStoreType(ESM::Skill);

// Special entry which is hardcoded and not loaded from an ESM
AddStoreType(ESM::Attribute);

static const int sRecordTypeCount = sRecordTypeCounter;



namespace MWWorld
{
    struct ESMStoreImp
    {
        //These 3 don't inherit from store base
        Store<ESM::MagicEffect> mMagicEffect;
        Store<ESM::Skill> mSkills;
        Store<ESM::Attribute> mAttributes;

        std::map<int, int>      mESM3RecordToRecordId;


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
            auto esm3RecordType_find = stores.mStoreImp->mESM3RecordToRecordId.find(GetRecordTypeId(T));

            if (esm3RecordType_find != stores.mStoreImp->mESM3RecordToRecordId.end())
            {
                stores.mIds[ptr->mId] = esm3RecordType_find->first;
            }
            return ptr;
        }

        template <class T>
        static const T * ESM3overrideRecord(ESMStore& stores, const T &x) {
            Store<T> &store = const_cast<Store<T> &>( stores.get<T>());

            T *ptr = store.insert(x);
            auto esm3RecordType_find = stores.mStoreImp->mESM3RecordToRecordId.find(GetRecordTypeId(T));
            if (esm3RecordType_find != stores.mStoreImp->mESM3RecordToRecordId.end())
            {
                stores.mIds[ptr->mId] = esm3RecordType_find->first;
            }
            return ptr;
        }

                template <class T>
        static const T *ESM3insertStatic(ESMStore& stores, const T &x)
        {
            const std::string id = "$dynamic" + std::to_string(stores.mDynamicCount++);

            Store<T> &store = const_cast<Store<T> &>( stores.get<T>());
            if (store.search(id) != nullptr)
            {
                const std::string msg = "Try to override existing record '" + id + "'";
                throw std::runtime_error(msg);
            }
            T record = x;

            T *ptr = store.insertStatic(record);
            auto esm3RecordType_find = stores.mStoreImp->mESM3RecordToRecordId.find(GetRecordTypeId(T));
            if (esm3RecordType_find != stores.mStoreImp->mESM3RecordToRecordId.end())
            {
                stores.mIds[ptr->mId] = esm3RecordType_find->first;
            }
            return ptr;
        }

        ESMStoreImp()
        {
            mESM3RecordToRecordId[ESM::REC_ACTI] = GetRecordTypeId(ESM::Activator);
            mESM3RecordToRecordId[ESM::REC_ALCH] = GetRecordTypeId(ESM::Potion);
            mESM3RecordToRecordId[ESM::REC_APPA] = GetRecordTypeId(ESM::Apparatus);
            mESM3RecordToRecordId[ESM::REC_ARMO] = GetRecordTypeId(ESM::Armor);
            mESM3RecordToRecordId[ESM::REC_BODY] = GetRecordTypeId(ESM::BodyPart);
            mESM3RecordToRecordId[ESM::REC_BOOK] = GetRecordTypeId(ESM::Book);
            mESM3RecordToRecordId[ESM::REC_BSGN] = GetRecordTypeId(ESM::BirthSign);
            mESM3RecordToRecordId[ESM::REC_CELL] = GetRecordTypeId(ESM::Cell);
            mESM3RecordToRecordId[ESM::REC_CLAS] = GetRecordTypeId(ESM::Class);
            mESM3RecordToRecordId[ESM::REC_CLOT] = GetRecordTypeId(ESM::Clothing);
            mESM3RecordToRecordId[ESM::REC_CONT] = GetRecordTypeId(ESM::Container);
            mESM3RecordToRecordId[ESM::REC_CREA] = GetRecordTypeId(ESM::Creature);
            mESM3RecordToRecordId[ESM::REC_DIAL] = GetRecordTypeId(ESM::Dialogue);
            mESM3RecordToRecordId[ESM::REC_DOOR] = GetRecordTypeId(ESM::Door);
            mESM3RecordToRecordId[ESM::REC_ENCH] = GetRecordTypeId(ESM::Enchantment);
            mESM3RecordToRecordId[ESM::REC_FACT] = GetRecordTypeId(ESM::Faction);
            mESM3RecordToRecordId[ESM::REC_GLOB] = GetRecordTypeId(ESM::Global);
            mESM3RecordToRecordId[ESM::REC_GMST] = GetRecordTypeId(ESM::GameSetting);
            mESM3RecordToRecordId[ESM::REC_INGR] = GetRecordTypeId(ESM::Ingredient);
            mESM3RecordToRecordId[ESM::REC_LAND] = GetRecordTypeId(ESM::Land);
            mESM3RecordToRecordId[ESM::REC_LEVC] = GetRecordTypeId(ESM::CreatureLevList);
            mESM3RecordToRecordId[ESM::REC_LEVI] = GetRecordTypeId(ESM::ItemLevList);
            mESM3RecordToRecordId[ESM::REC_LIGH] = GetRecordTypeId(ESM::Light);
            mESM3RecordToRecordId[ESM::REC_LOCK] = GetRecordTypeId(ESM::Lockpick);
            mESM3RecordToRecordId[ESM::REC_LTEX] = GetRecordTypeId(ESM::LandTexture);
            mESM3RecordToRecordId[ESM::REC_MISC] = GetRecordTypeId(ESM::Miscellaneous);
            mESM3RecordToRecordId[ESM::REC_NPC_] = GetRecordTypeId(ESM::NPC);
            mESM3RecordToRecordId[ESM::REC_PGRD] = GetRecordTypeId(ESM::Pathgrid);
            mESM3RecordToRecordId[ESM::REC_PROB] = GetRecordTypeId(ESM::Probe);
            mESM3RecordToRecordId[ESM::REC_RACE] = GetRecordTypeId(ESM::Race);
            mESM3RecordToRecordId[ESM::REC_REGN] = GetRecordTypeId(ESM::Region);
            mESM3RecordToRecordId[ESM::REC_REPA] = GetRecordTypeId(ESM::Repair);
            mESM3RecordToRecordId[ESM::REC_SCPT] = GetRecordTypeId(ESM::Script);
            mESM3RecordToRecordId[ESM::REC_SNDG] = GetRecordTypeId(ESM::SoundGenerator);
            mESM3RecordToRecordId[ESM::REC_SOUN] = GetRecordTypeId(ESM::Sound);
            mESM3RecordToRecordId[ESM::REC_SPEL] = GetRecordTypeId(ESM::Spell);
            mESM3RecordToRecordId[ESM::REC_SSCR] = GetRecordTypeId(ESM::StartScript);
            mESM3RecordToRecordId[ESM::REC_STAT] = GetRecordTypeId(ESM::Static);
            mESM3RecordToRecordId[ESM::REC_WEAP] = GetRecordTypeId(ESM::Weapon);
        }
    };


#define defineGetters(__Type) template <> const Store<__Type>& ESMStore::get<__Type>() const { return static_cast<const Store<__Type>&>(*mStores[GetRecordTypeId(__Type)]);  } \
    template <> Store<__Type>& ESMStore::getWritable<__Type>() { return static_cast<Store<__Type>&>(*mStores[GetRecordTypeId(__Type)]);  }

    defineGetters(ESM::Activator);
    defineGetters(ESM::Potion);
    defineGetters(ESM::Apparatus);
    defineGetters(ESM::Armor);
    defineGetters(ESM::BodyPart);
    defineGetters(ESM::Book);
    defineGetters(ESM::BirthSign);
    defineGetters(ESM::Class);
    defineGetters(ESM::Clothing);
    defineGetters(ESM::Container);
    defineGetters(ESM::Creature);
    defineGetters(ESM::Dialogue);
    defineGetters(ESM::Door);
    defineGetters(ESM::Enchantment);
    defineGetters(ESM::Faction);
    defineGetters(ESM::Global);
    defineGetters(ESM::Ingredient);
    defineGetters(ESM::CreatureLevList);
    defineGetters(ESM::ItemLevList);
    defineGetters(ESM::Light);
    defineGetters(ESM::Lockpick);
    defineGetters(ESM::Miscellaneous);
    defineGetters(ESM::NPC);
    defineGetters(ESM::Probe);
    defineGetters(ESM::Race);
    defineGetters(ESM::Region);
    defineGetters(ESM::Repair);
    defineGetters(ESM::SoundGenerator);
    defineGetters(ESM::Sound);
    defineGetters(ESM::Spell);
    defineGetters(ESM::StartScript);
    defineGetters(ESM::Static);
    defineGetters(ESM::Weapon);
    defineGetters(ESM::GameSetting);
    defineGetters(ESM::Script);
    defineGetters(ESM::Cell);
    defineGetters(ESM::Land);
    defineGetters(ESM::LandTexture);
    defineGetters(ESM::Pathgrid);
#undef defineGetters
    template <> const Store<ESM::MagicEffect>& ESMStore::get<ESM::MagicEffect>() const { return mStoreImp->mMagicEffect; }
    template <> Store<ESM::MagicEffect>& ESMStore::getWritable<ESM::MagicEffect>() { return mStoreImp->mMagicEffect; }

    template <> const Store<ESM::Skill>& ESMStore::get<ESM::Skill>() const { return mStoreImp->mSkills; }
    template <> Store<ESM::Skill>& ESMStore::getWritable<ESM::Skill>() { return mStoreImp->mSkills; }

    template <> const Store<ESM::Attribute>& ESMStore::get<ESM::Attribute>() const { return mStoreImp->mAttributes; }
    template <> Store<ESM::Attribute>& ESMStore::getWritable<ESM::Attribute>() { return mStoreImp->mAttributes; }

    ESMStore::ESMStore()
    {
        mStores.resize(sRecordTypeCount);
#define createStore(__Type) mStores[GetRecordTypeId(__Type)] = std::make_unique<Store<__Type>>();

        createStore(ESM::Activator);
        createStore(ESM::Potion);
        createStore(ESM::Apparatus);
        createStore(ESM::Armor);
        createStore(ESM::BodyPart);
        createStore(ESM::Book);
        createStore(ESM::BirthSign);
        createStore(ESM::Class);
        createStore(ESM::Clothing);
        createStore(ESM::Container);
        createStore(ESM::Creature);
        createStore(ESM::Dialogue);
        createStore(ESM::Door);
        createStore(ESM::Enchantment);
        createStore(ESM::Faction);
        createStore(ESM::Global);
        createStore(ESM::Ingredient);
        createStore(ESM::CreatureLevList);
        createStore(ESM::ItemLevList);
        createStore(ESM::Light);
        createStore(ESM::Lockpick);
        createStore(ESM::Miscellaneous);
        createStore(ESM::NPC);
        createStore(ESM::Probe);
        createStore(ESM::Race);
        createStore(ESM::Region);
        createStore(ESM::Repair);
        createStore(ESM::SoundGenerator);
        createStore(ESM::Sound);
        createStore(ESM::Spell);
        createStore(ESM::StartScript);
        createStore(ESM::Static);
        createStore(ESM::Weapon);
        createStore(ESM::GameSetting);
        createStore(ESM::Script);
        createStore(ESM::Cell);
        createStore(ESM::Land);
        createStore(ESM::LandTexture);
        createStore(ESM::Pathgrid);
#undef createStore

        mStoreImp = std::make_unique<ESMStoreImp>();
        mDynamicCount = 0;

        getWritable<ESM::Pathgrid>().setCells(getWritable<ESM::Cell>());
    }

    ESMStore::~ESMStore()
    {
    }

    void ESMStore::clearDynamic()
    {
        for (std::vector<std::unique_ptr< StoreBase >>::iterator it = mStores.begin(); it != mStores.end(); ++it)
            (*it)->clearDynamic();

        movePlayerRecord();
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
    getWritable<ESM::LandTexture>().resize(esm.getIndex()+1);

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
        std::map<int, int>::iterator it = mStoreImp->mESM3RecordToRecordId.find(n.toInt());

        if (it == mStoreImp->mESM3RecordToRecordId.end()) {
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
                getWritable<ESM::MagicEffect>().load (esm);
            } else if (n.toInt() == ESM::REC_SKIL) {
                getWritable<ESM::Skill>().load (esm);
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
            RecordId id = mStores[ it->second]->load(esm);
            if (id.mIsDeleted)
            {
                mStores[ it->second]->eraseStatic(id.mId);
                continue;
            }

            if (n.toInt() == ESM::REC_DIAL) {
                dialogue = const_cast<ESM::Dialogue*>(getWritable<ESM::Dialogue>().find(id.mId));
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

    std::map<int, int>::iterator storeIt = mStoreImp->mESM3RecordToRecordId.begin();
    for (; storeIt != mStoreImp->mESM3RecordToRecordId.end(); ++storeIt) {
        mStores[storeIt->second]->setUp();

        if (isCacheableRecord(storeIt->first))
        {
            std::vector<std::string> identifiers;
            mStores[storeIt->second]->listIdentifier(identifiers);

            for (std::vector<std::string>::const_iterator record = identifiers.begin(); record != identifiers.end(); ++record)
                mIds[*record] = storeIt->first;
        }
    }

    if (mStaticIds.empty())
        for (const auto& [k, v] : mIds)
            mStaticIds.emplace(Misc::StringUtils::lowerCase(k), v);

    getWritable<ESM::Skill>().setUp();
    getWritable<ESM::MagicEffect>().setUp();;
    getWritable<ESM::Attribute>().setUp();
    getWritable<ESM::Dialogue>().setUp();
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
    Store<ESM::Cell> Cells = getWritable < ESM::Cell>();
    for(auto it = Cells.intBegin(); it != Cells.intEnd(); ++it)
        readRefs(*it, refs, refIDs, readers);
    for(auto it = Cells.extBegin(); it != Cells.extEnd(); ++it)
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
    auto& NPCs = getWritable<ESM::NPC>();
    std::vector<ESM::NPC> npcsToReplace = getNPCsToReplace(getWritable<ESM::Faction>(),  getWritable<ESM::Class>(),  NPCs.mStatic);

    for (const ESM::NPC &npc : npcsToReplace)
    {
        NPCs.eraseStatic(npc.mId);
        NPCs.insertStatic(npc);
    }

    // Validate spell effects for invalid arguments
    std::vector<ESM::Spell> spellsToReplace;
    auto& Spells = getWritable<ESM::Spell>();
    for (ESM::Spell spell : Spells)
    {
        if (spell.mEffects.mList.empty())
            continue;

        bool changed = false;
        auto iter = spell.mEffects.mList.begin();
        while (iter != spell.mEffects.mList.end())
        {
            const ESM::MagicEffect* mgef = getWritable<ESM::MagicEffect>().search(iter->mEffectID);
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
        Spells.eraseStatic(spell.mId);
        Spells.insertStatic(spell);
    }
}

void ESMStore::movePlayerRecord()
{
    auto& NPCs = getWritable<ESM::NPC>();
    auto player = NPCs.find("player");
    NPCs.insert(*player);
}

void ESMStore::validateDynamic()
{
    auto& NPCs = getWritable<ESM::NPC>();
    auto& scripts = getWritable<ESM::Script>();

    std::vector<ESM::NPC> npcsToReplace = getNPCsToReplace(getWritable<ESM::Faction>(), getWritable<ESM::Class>(), NPCs.mDynamic);

    for (const ESM::NPC &npc : npcsToReplace)
        NPCs.insert(npc);

    removeMissingScripts(scripts, getWritable<ESM::Armor>().mDynamic);
    removeMissingScripts(scripts, getWritable<ESM::Book>().mDynamic);
    removeMissingScripts(scripts, getWritable<ESM::Clothing>().mDynamic);
    removeMissingScripts(scripts, getWritable<ESM::Weapon>().mDynamic);

    removeMissingObjects(getWritable<ESM::CreatureLevList>());
    removeMissingObjects(getWritable<ESM::ItemLevList>());
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
            + get<ESM::Potion>().getDynamicSize()
            + get<ESM::Armor>().getDynamicSize()
            + get<ESM::Book>().getDynamicSize()
            + get<ESM::Class>().getDynamicSize()
            + get<ESM::Clothing>().getDynamicSize()
            + get<ESM::Enchantment>().getDynamicSize()
            + get<ESM::NPC>().getDynamicSize()
            + get<ESM::Spell>().getDynamicSize()
            + get<ESM::Weapon>().getDynamicSize()
            + get<ESM::CreatureLevList>().getDynamicSize()
            + get<ESM::ItemLevList>().getDynamicSize()
            + get<ESM::Creature>().getDynamicSize()
            + get<ESM::Container>().getDynamicSize();

    }

    void ESMStore::write (ESM::ESMWriter& writer, Loading::Listener& progress) const
    {
        writer.startRecord(ESM::REC_DYNA);
        writer.startSubRecord("COUN");
        writer.writeT(mDynamicCount);
        writer.endRecord("COUN");
        writer.endRecord(ESM::REC_DYNA);

        get<ESM::Potion>().write (writer, progress);
        get<ESM::Armor>().write (writer, progress);
        get<ESM::Book>().write (writer, progress);
        get<ESM::Class>().write (writer, progress);
        get<ESM::Clothing>().write (writer, progress);
        get<ESM::Enchantment>().write (writer, progress);
        get<ESM::NPC>().write (writer, progress);
        get<ESM::Spell>().write (writer, progress);
        get<ESM::Weapon>().write (writer, progress);
        get<ESM::CreatureLevList>().write (writer, progress);
        get<ESM::ItemLevList>().write (writer, progress);
        get<ESM::Creature>().write (writer, progress);
        get<ESM::Container>().write (writer, progress);
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
                mStores[mStoreImp->mESM3RecordToRecordId[type] ]->read (reader);
                return true;
            case ESM::REC_NPC_:
            case ESM::REC_CREA:
            case ESM::REC_CONT:
                mStores[mStoreImp->mESM3RecordToRecordId[type]]->read (reader, true);
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

        const ESM::NPC *player = get<ESM::NPC>().find ("player");

        if (!get<ESM::Race>().find (player->mRace) ||
            !get<ESM::Class>().find (player->mClass))
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
        return getWritable<ESM::Cell>().insert(cell);
    }

    template <>
    const ESM::NPC *ESMStore::insert<ESM::NPC>(const ESM::NPC &npc)
    {
        const std::string id = "$dynamic" + std::to_string(mDynamicCount++);
        auto& NPCs = getWritable<ESM::NPC>();
        if (Misc::StringUtils::ciEqual(npc.mId, "player"))
        {
            return NPCs.insert(npc);
        }
        else if (NPCs.search(id) != nullptr)
        {
            const std::string msg = "Try to override existing record '" + id + "'";
            throw std::runtime_error(msg);
        }
        ESM::NPC record = npc;

        record.mId = id;

        ESM::NPC *ptr = NPCs.insert(record);
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

#define ESM3InsertStatic(__Type) template<> const __Type* ESMStore::insertStatic<__Type>(const __Type &toInsert) { return ESMStoreImp::ESM3insertStatic(*this, toInsert);   }
    ESM3InsertStatic(ESM::GameSetting);
    ESM3InsertStatic(ESM::Static);
    ESM3InsertStatic(ESM::Door);
    ESM3InsertStatic(ESM::Global);
    ESM3InsertStatic(ESM::NPC);
#undef ESM3InsertStatic


#define ESM3overrideRecord(__Type) template<> const __Type* ESMStore::overrideRecord<__Type>(const __Type &toInsert) { return ESMStoreImp::ESM3overrideRecord(*this, toInsert);   }
    ESM3overrideRecord(ESM::Container);
    ESM3overrideRecord(ESM::Creature);
    ESM3overrideRecord(ESM::CreatureLevList);
    ESM3overrideRecord(ESM::Door);
    ESM3overrideRecord(ESM::ItemLevList);
    ESM3overrideRecord(ESM::NPC);
#undef ESM3overrideRecord


} // end namespace
