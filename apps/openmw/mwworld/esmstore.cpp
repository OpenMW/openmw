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
#include <components/esm4/common.hpp>

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

    template <int storeIndex> 
    struct StoreIndexToRecordType {
        
        typedef void recordType;
    };
}

static int sRecordTypeCounter = 0;

#define OPENMW_ESM_ADD_STORE_TYPE(__Type, __ID_NUM) template<> const int MWWorld::SRecordType<__Type>::sStoreIndex = sRecordTypeCounter ++;  \
template<> struct StoreIndexToRecordType<__ID_NUM > {typedef __Type recordType;}

OPENMW_ESM_ADD_STORE_TYPE(ESM::Activator,0);
OPENMW_ESM_ADD_STORE_TYPE(ESM::Potion,1);
OPENMW_ESM_ADD_STORE_TYPE(ESM::Apparatus,2);
OPENMW_ESM_ADD_STORE_TYPE(ESM::Armor,3);
OPENMW_ESM_ADD_STORE_TYPE(ESM::BodyPart,4);
OPENMW_ESM_ADD_STORE_TYPE(ESM::Book,5);
OPENMW_ESM_ADD_STORE_TYPE(ESM::BirthSign,6);
OPENMW_ESM_ADD_STORE_TYPE(ESM::Class,7);
OPENMW_ESM_ADD_STORE_TYPE(ESM::Clothing,8);
OPENMW_ESM_ADD_STORE_TYPE(ESM::Container,9);
OPENMW_ESM_ADD_STORE_TYPE(ESM::Creature,10);
OPENMW_ESM_ADD_STORE_TYPE(ESM::Dialogue,11);
OPENMW_ESM_ADD_STORE_TYPE(ESM::Door,12);
OPENMW_ESM_ADD_STORE_TYPE(ESM::Enchantment,13);
OPENMW_ESM_ADD_STORE_TYPE(ESM::Faction,14);
OPENMW_ESM_ADD_STORE_TYPE(ESM::Global,15);
OPENMW_ESM_ADD_STORE_TYPE(ESM::Ingredient,16);
OPENMW_ESM_ADD_STORE_TYPE(ESM::CreatureLevList,17);
OPENMW_ESM_ADD_STORE_TYPE(ESM::ItemLevList,18);
OPENMW_ESM_ADD_STORE_TYPE(ESM::Light,19);
OPENMW_ESM_ADD_STORE_TYPE(ESM::Lockpick,20);
OPENMW_ESM_ADD_STORE_TYPE(ESM::Miscellaneous,21);
OPENMW_ESM_ADD_STORE_TYPE(ESM::NPC,22);
OPENMW_ESM_ADD_STORE_TYPE(ESM::Probe,23);
OPENMW_ESM_ADD_STORE_TYPE(ESM::Race,24);
OPENMW_ESM_ADD_STORE_TYPE(ESM::Region,25);
OPENMW_ESM_ADD_STORE_TYPE(ESM::Repair,26);
OPENMW_ESM_ADD_STORE_TYPE(ESM::SoundGenerator,27);
OPENMW_ESM_ADD_STORE_TYPE(ESM::Sound,28);
OPENMW_ESM_ADD_STORE_TYPE(ESM::Spell,29);
OPENMW_ESM_ADD_STORE_TYPE(ESM::StartScript,30);
OPENMW_ESM_ADD_STORE_TYPE(ESM::Static,31);
OPENMW_ESM_ADD_STORE_TYPE(ESM::Weapon,32);
OPENMW_ESM_ADD_STORE_TYPE(ESM::GameSetting,33);
OPENMW_ESM_ADD_STORE_TYPE(ESM::Script,34);

// Lists that need special rules
OPENMW_ESM_ADD_STORE_TYPE(ESM::Cell,35);
OPENMW_ESM_ADD_STORE_TYPE(ESM::Land,36);
OPENMW_ESM_ADD_STORE_TYPE(ESM::LandTexture,37);
OPENMW_ESM_ADD_STORE_TYPE(ESM::Pathgrid,38);

OPENMW_ESM_ADD_STORE_TYPE(ESM::MagicEffect,39);
OPENMW_ESM_ADD_STORE_TYPE(ESM::Skill,40);

// Special entry which is hardcoded and not loaded from an ESM
OPENMW_ESM_ADD_STORE_TYPE(ESM::Attribute,41);

static const int sRecordTypeCount = sRecordTypeCounter;
constexpr int sRecordIndexCount = 42;

template <auto Start, auto End, auto Inc, class F>
constexpr void constexpr_for(F&& f)
{
    if constexpr (Start < End)
    {
        f(std::integral_constant<decltype(Start), Start>());
        constexpr_for<Start + Inc, End, Inc>(f);
    }
}


namespace MWWorld
{
    using IDMap = std::unordered_map<std::string, int, Misc::StringUtils::CiHash, Misc::StringUtils::CiEqual>;


    struct ESMStoreImp
    {
        //These 3 don't inherit from store base
        Store<ESM::MagicEffect> mMagicEffect;
        Store<ESM::Skill> mSkills;
        Store<ESM::Attribute> mAttributes;

        std::map<ESM::RecNameInts, StoreBase*>                   mESM3RecordToStore;
        std::unordered_map<const StoreBase*, ESM::RecNameInts>   mStoreToEsm3Record;

        // Lookup of all IDs. Makes looking up references faster. Just
        // maps the id name to the record type.
        IDMap mIds;
        IDMap mStaticIds;


        template<typename T> 
        static const T* esm3StoreInsert(ESMStore& stores, const T &toInsert)
        {
            const std::string id = "$dynamic" + std::to_string(stores.mDynamicCount++);

            Store<T> &store = stores.getWritable<T>();
            if (store.search(id) != nullptr)
            {
                const std::string msg = "Try to override existing record '" + id + "'";
                throw std::runtime_error(msg);
            }
            T record = toInsert;

            record.mId = id;

            T *ptr = store.insert(record);
            auto esm3RecordType_find = stores.mStoreImp->mStoreToEsm3Record.find(&stores.get<T>());

            if (esm3RecordType_find != stores.mStoreImp->mStoreToEsm3Record.end())
            {
                stores.mStoreImp->mIds[ptr->mId] = esm3RecordType_find->second;
            }
            return ptr;
        }

        template <class T>
        static const T * esm3overrideRecord(ESMStore& stores, const T &x) {
            Store<T> &store = stores.getWritable<T>();

            T *ptr = store.insert(x);
            auto esm3RecordType_find = stores.mStoreImp->mStoreToEsm3Record.find(&stores.get<T>());
            if (esm3RecordType_find != stores.mStoreImp->mStoreToEsm3Record.end())
            {
                stores.mStoreImp->mIds[ptr->mId] = esm3RecordType_find->second;
            }
            return ptr;
        }

                template <class T>
        static const T *esm3insertStatic(ESMStore& stores, const T &x)
        {
            const std::string id = "$dynamic" + std::to_string(stores.mDynamicCount++);

            Store<T> &store = stores.getWritable<T>();
            if (store.search(id) != nullptr)
            {
                const std::string msg = "Try to override existing record '" + id + "'";
                throw std::runtime_error(msg);
            }
            T record = x;

            T *ptr = store.insertStatic(record);
            auto esm3RecordType_find = stores.mStoreImp->mStoreToEsm3Record.find(&stores.get<T>());
            if (esm3RecordType_find != stores.mStoreImp->mStoreToEsm3Record.end())
            {
                stores.mStoreImp->mIds[ptr->mId] = esm3RecordType_find->second;
            }
            return ptr;
        }

        ESMStoreImp(ESMStore& store)
        {
            mESM3RecordToStore[ESM::REC_ACTI] = &store.getWritable<ESM::Activator>();
            mESM3RecordToStore[ESM::REC_ALCH] = &store.getWritable<ESM::Potion>();
            mESM3RecordToStore[ESM::REC_APPA] = &store.getWritable<ESM::Apparatus>();
            mESM3RecordToStore[ESM::REC_ARMO] = &store.getWritable<ESM::Armor>();
            mESM3RecordToStore[ESM::REC_BODY] = &store.getWritable<ESM::BodyPart>();
            mESM3RecordToStore[ESM::REC_BOOK] = &store.getWritable<ESM::Book>();
            mESM3RecordToStore[ESM::REC_BSGN] = &store.getWritable<ESM::BirthSign>();
            mESM3RecordToStore[ESM::REC_CELL] = &store.getWritable<ESM::Cell>();
            mESM3RecordToStore[ESM::REC_CLAS] = &store.getWritable<ESM::Class>();
            mESM3RecordToStore[ESM::REC_CLOT] = &store.getWritable<ESM::Clothing>();
            mESM3RecordToStore[ESM::REC_CONT] = &store.getWritable<ESM::Container>();
            mESM3RecordToStore[ESM::REC_CREA] = &store.getWritable<ESM::Creature>();
            mESM3RecordToStore[ESM::REC_DIAL] = &store.getWritable<ESM::Dialogue>();
            mESM3RecordToStore[ESM::REC_DOOR] = &store.getWritable<ESM::Door>();
            mESM3RecordToStore[ESM::REC_ENCH] = &store.getWritable<ESM::Enchantment>();
            mESM3RecordToStore[ESM::REC_FACT] = &store.getWritable<ESM::Faction>();
            mESM3RecordToStore[ESM::REC_GLOB] = &store.getWritable<ESM::Global>();
            mESM3RecordToStore[ESM::REC_GMST] = &store.getWritable<ESM::GameSetting>();
            mESM3RecordToStore[ESM::REC_INGR] = &store.getWritable<ESM::Ingredient>();
            mESM3RecordToStore[ESM::REC_LAND] = &store.getWritable<ESM::Land>();
            mESM3RecordToStore[ESM::REC_LEVC] = &store.getWritable<ESM::CreatureLevList>();
            mESM3RecordToStore[ESM::REC_LEVI] = &store.getWritable<ESM::ItemLevList>();
            mESM3RecordToStore[ESM::REC_LIGH] = &store.getWritable<ESM::Light>();
            mESM3RecordToStore[ESM::REC_LOCK] = &store.getWritable<ESM::Lockpick>();
            mESM3RecordToStore[ESM::REC_LTEX] = &store.getWritable<ESM::LandTexture>();
            mESM3RecordToStore[ESM::REC_MISC] = &store.getWritable<ESM::Miscellaneous>();
            mESM3RecordToStore[ESM::REC_NPC_] = &store.getWritable<ESM::NPC>();
            mESM3RecordToStore[ESM::REC_PGRD] = &store.getWritable<ESM::Pathgrid>();
            mESM3RecordToStore[ESM::REC_PROB] = &store.getWritable<ESM::Probe>();
            mESM3RecordToStore[ESM::REC_RACE] = &store.getWritable<ESM::Race>();
            mESM3RecordToStore[ESM::REC_REGN] = &store.getWritable<ESM::Region>();
            mESM3RecordToStore[ESM::REC_REPA] = &store.getWritable<ESM::Repair>();
            mESM3RecordToStore[ESM::REC_SCPT] = &store.getWritable<ESM::Script>();
            mESM3RecordToStore[ESM::REC_SNDG] = &store.getWritable<ESM::SoundGenerator>();
            mESM3RecordToStore[ESM::REC_SOUN] = &store.getWritable<ESM::Sound>();
            mESM3RecordToStore[ESM::REC_SPEL] = &store.getWritable<ESM::Spell>(); 
            mESM3RecordToStore[ESM::REC_SSCR] = &store.getWritable<ESM::StartScript>();
            mESM3RecordToStore[ESM::REC_STAT] = &store.getWritable<ESM::Static>();
            mESM3RecordToStore[ESM::REC_WEAP] = &store.getWritable<ESM::Weapon>();

            for (const auto& recordStorePair : mESM3RecordToStore)
            {
                const StoreBase* storePtr = recordStorePair.second;
                mStoreToEsm3Record[storePtr] = recordStorePair.first;
            }
        }

        template<typename T>
        static void createStore(ESMStore& stores)
        {
            if constexpr (std::is_convertible<Store<T>*, StoreBase*>::value)
            {
                stores.mStores[SRecordType<T>::sStoreIndex] = std::make_unique<Store<T>>();
            }
        }
    };


    int ESMStore::find(const std::string& id) const
    {
        
        IDMap::const_iterator it = mStoreImp->mIds.find(id);
        if (it == mStoreImp->mIds.end()) {
            return 0;
        }
        return it->second;
        
    }

    int ESMStore::findStatic(const std::string& id) const
    {
        IDMap::const_iterator it = mStoreImp-> mStaticIds.find(id);
        if (it == mStoreImp->mStaticIds.end()) {
            return 0;
        }
        return it->second;
    }

    ESMStore::ESMStore()
    {
        mStores.resize(sRecordTypeCount);
        assert(sRecordTypeCounter == sRecordIndexCount); //Otherwise something wen wrong with assigning index to stores
        constexpr_for<0, sRecordIndexCount,1> ([this](auto storeIndex)
            {
                ESMStoreImp::createStore<StoreIndexToRecordType<storeIndex>::recordType>(*this);
            });

        mStoreImp = std::make_unique<ESMStoreImp>(*this);
        mDynamicCount = 0;

        getWritable<ESM::Pathgrid>().setCells(getWritable<ESM::Cell>());
    }

    ESMStore::~ESMStore()
    {
    }

    void ESMStore::clearDynamic()
    {
        for (const auto& store : mStores)
            store->clearDynamic();

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
        ESM::RecNameInts recName = static_cast<ESM::RecNameInts>(n.toInt());
        const auto& it = mStoreImp->mESM3RecordToStore.find(recName);

        if (it == mStoreImp->mESM3RecordToStore.end()) {
            if (recName == ESM::REC_INFO) {
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
            RecordId id = it->second->load(esm);
            if (id.mIsDeleted)
            {
                it->second->eraseStatic(id.mId);
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
    mStoreImp->mIds.clear();

    std::map<ESM::RecNameInts, StoreBase*>::iterator storeIt = mStoreImp->mESM3RecordToStore.begin();
    for (; storeIt != mStoreImp->mESM3RecordToStore.end(); ++storeIt) {
        storeIt->second->setUp();

        if (isCacheableRecord(storeIt->first))
        {
            std::vector<std::string> identifiers;
            storeIt->second->listIdentifier(identifiers);

            for (std::vector<std::string>::const_iterator record = identifiers.begin(); record != identifiers.end(); ++record)
                mStoreImp->mIds[*record] = storeIt->first;
        }
    }

    if (mStoreImp->mStaticIds.empty())
        for (const auto& [k, v] : mStoreImp->mIds)
            mStoreImp->mStaticIds.emplace(Misc::StringUtils::lowerCase(k), v);

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
    auto& npcs = getWritable<ESM::NPC>();
    std::vector<ESM::NPC> npcsToReplace = getNPCsToReplace(getWritable<ESM::Faction>(),  getWritable<ESM::Class>(),  npcs.mStatic);

    for (const ESM::NPC &npc : npcsToReplace)
    {
        npcs.eraseStatic(npc.mId);
        npcs.insertStatic(npc);
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
    auto& npcs = getWritable<ESM::NPC>();
    auto player = npcs.find("player");
    npcs.insert(*player);
}

void ESMStore::validateDynamic()
{
    auto& npcs = getWritable<ESM::NPC>();
    auto& scripts = getWritable<ESM::Script>();

    std::vector<ESM::NPC> npcsToReplace = getNPCsToReplace(getWritable<ESM::Faction>(), getWritable<ESM::Class>(), npcs.mDynamic);

    for (const ESM::NPC &npc : npcsToReplace)
        npcs.insert(npc);

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

    bool ESMStore::readRecord (ESM::ESMReader& reader, uint32_t type_id)
    {
        ESM::RecNameInts type = (ESM::RecNameInts)type_id;
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
                mStoreImp->mESM3RecordToStore[type]->read (reader);
                return true;
            case ESM::REC_NPC_:
            case ESM::REC_CREA:
            case ESM::REC_CONT:
                mStoreImp->mESM3RecordToStore[type]->read (reader, true);
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
        auto& npcs = getWritable<ESM::NPC>();
        if (Misc::StringUtils::ciEqual(npc.mId, "player"))
        {
            return npcs.insert(npc);
        }
        else if (npcs.search(id) != nullptr)
        {
            const std::string msg = "Try to override existing record '" + id + "'";
            throw std::runtime_error(msg);
        }
        ESM::NPC record = npc;

        record.mId = id;

        ESM::NPC *ptr = npcs.insert(record);
        mStoreImp->mIds[ptr->mId] = ESM::REC_NPC_;
        return ptr;
    }

#define OPENMW_ESM3_INSERT(__Type) template<> const __Type* ESMStore::insert<__Type>(const __Type &toInsert) { return ESMStoreImp::esm3StoreInsert(*this, toInsert);   }
    OPENMW_ESM3_INSERT(ESM::Book)
    OPENMW_ESM3_INSERT(ESM::Armor)
    OPENMW_ESM3_INSERT(ESM::Class)
    OPENMW_ESM3_INSERT(ESM::Enchantment)
    OPENMW_ESM3_INSERT(ESM::Potion)
    OPENMW_ESM3_INSERT(ESM::Weapon)
    OPENMW_ESM3_INSERT(ESM::Clothing)
    OPENMW_ESM3_INSERT(ESM::Spell)
#undef OPENMW_ESM3_INSERT

#define OPENMW_ESM3_INSERT_STATIC(__Type) template<> const __Type* ESMStore::insertStatic<__Type>(const __Type &toInsert) { return ESMStoreImp::esm3insertStatic(*this, toInsert);   }
    OPENMW_ESM3_INSERT_STATIC(ESM::GameSetting)
    OPENMW_ESM3_INSERT_STATIC(ESM::Static)
    OPENMW_ESM3_INSERT_STATIC(ESM::Door)
    OPENMW_ESM3_INSERT_STATIC(ESM::Global)
    OPENMW_ESM3_INSERT_STATIC(ESM::NPC)
#undef OPENMW_ESM3_INSERT_STATIC


#define OPENMW_ESM3_OVERRIDE_RECORD(__Type) template<> const __Type* ESMStore::overrideRecord<__Type>(const __Type &toInsert) { return ESMStoreImp::esm3overrideRecord(*this, toInsert);   }
    OPENMW_ESM3_OVERRIDE_RECORD(ESM::Container)
    OPENMW_ESM3_OVERRIDE_RECORD(ESM::Creature)
    OPENMW_ESM3_OVERRIDE_RECORD(ESM::CreatureLevList)
    OPENMW_ESM3_OVERRIDE_RECORD(ESM::Door)
    OPENMW_ESM3_OVERRIDE_RECORD(ESM::ItemLevList)
    OPENMW_ESM3_OVERRIDE_RECORD(ESM::NPC)
#undef OPENMW_ESM3_OVERRIDE_RECORD

    template <> const Store<ESM::MagicEffect>& ESMStore::get<ESM::MagicEffect>() const { return mStoreImp->mMagicEffect; }
    template <> Store<ESM::MagicEffect>& ESMStore::getWritable<ESM::MagicEffect>() { return mStoreImp->mMagicEffect; }

    template <> const Store<ESM::Skill>& ESMStore::get<ESM::Skill>() const { return mStoreImp->mSkills; }
    template <> Store<ESM::Skill>& ESMStore::getWritable<ESM::Skill>() { return mStoreImp->mSkills; }

    template <> const Store<ESM::Attribute>& ESMStore::get<ESM::Attribute>() const { return mStoreImp->mAttributes; }
    template <> Store<ESM::Attribute>& ESMStore::getWritable<ESM::Attribute>() { return mStoreImp->mAttributes; }

} // end namespace
