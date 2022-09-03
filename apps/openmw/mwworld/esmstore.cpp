#include "esmstore.hpp"

#include <algorithm>
#include <fstream>
#include <tuple>

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


}

namespace MWWorld
{
    using IDMap = std::unordered_map<std::string, int, Misc::StringUtils::CiHash, Misc::StringUtils::CiEqual>;

    struct ESMStoreImp
    {
        std::tuple <
            Store<ESM::Activator>,
            Store<ESM::Potion>,
            Store<ESM::Apparatus>,
            Store<ESM::Armor>,
            Store<ESM::BodyPart>,
            Store<ESM::Book>,
            Store<ESM::BirthSign>,
            Store<ESM::Class>,
            Store<ESM::Clothing>,
            Store<ESM::Container>,
            Store<ESM::Creature>,
            Store<ESM::Dialogue>,
            Store<ESM::Door>,
            Store<ESM::Enchantment>,
            Store<ESM::Faction>,
            Store<ESM::Global>,
            Store<ESM::Ingredient>,
            Store<ESM::CreatureLevList>,
            Store<ESM::ItemLevList>,
            Store<ESM::Light>,
            Store<ESM::Lockpick>,
            Store<ESM::Miscellaneous>,
            Store<ESM::NPC>,
            Store<ESM::Probe>,
            Store<ESM::Race>,
            Store<ESM::Region>,
            Store<ESM::Repair>,
            Store<ESM::SoundGenerator>,
            Store<ESM::Sound>,
            Store<ESM::Spell>,
            Store<ESM::StartScript>,
            Store<ESM::Static>,
            Store<ESM::Weapon>,
            Store<ESM::GameSetting>,
            Store<ESM::Script>,

            // Lists that need special rules
            Store<ESM::Cell>,
            Store<ESM::Land>,
            Store<ESM::LandTexture>,
            Store<ESM::Pathgrid>,

            Store<ESM::MagicEffect>,
            Store<ESM::Skill>,

            // Special entry which is hardcoded and not loaded from an ESM
            Store<ESM::Attribute >> mStores;

        std::map<ESM::RecNameInts, DynamicStore*>                   mRecNameToStore;
        std::unordered_map<const DynamicStore*, ESM::RecNameInts>   mStoreToRecName;

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
            auto esm3RecordType_find = stores.mStoreImp->mStoreToRecName.find(&stores.get<T>());

            if (esm3RecordType_find != stores.mStoreImp->mStoreToRecName.end())
            {
                stores.mStoreImp->mIds[ptr->mId] = esm3RecordType_find->second;
            }
            return ptr;
        }

        template <class T>
        static const T * esm3overrideRecord(ESMStore& stores, const T &x) {
            Store<T> &store = stores.getWritable<T>();

            T *ptr = store.insert(x);
            auto esm3RecordType_find = stores.mStoreImp->mStoreToRecName.find(&stores.get<T>());
            if (esm3RecordType_find != stores.mStoreImp->mStoreToRecName.end())
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
            auto esm3RecordType_find = stores.mStoreImp->mStoreToRecName.find(&stores.get<T>());
            if (esm3RecordType_find != stores.mStoreImp->mStoreToRecName.end())
            {
                stores.mStoreImp->mIds[ptr->mId] = esm3RecordType_find->second;
            }
            return ptr;
        }

        template<typename T>  
        static int AssignStoreToIndex(ESMStore& stores, Store<T>& store)
        {
            const std::size_t storeIndex = ESMStore::getTypeIndex<T>();
            if (stores.mStores.size() <= storeIndex)
                stores.mStores.resize(storeIndex + 1);

            assert(&store == &std::get<Store<T>>(stores.mStoreImp->mStores));

            stores.mStores[storeIndex] = &store;
            if constexpr (std::is_convertible<Store<T>*, DynamicStore*>::value)
            {
                stores.mDynamicStores.push_back(&store);
                constexpr ESM::RecNameInts recName = T::sRecordId;
                if constexpr (recName != ESM::REC_INTERNAL_PLAYER)
                {
                    stores.mStoreImp->mRecNameToStore[recName] = &store;
                }
            }
            return 0;
        }

        void SetupAfterStoresCreation(ESMStore& store)
        {
            for (const auto& recordStorePair : mRecNameToStore)
            {
                const DynamicStore* storePtr = recordStorePair.second;
                mStoreToRecName[storePtr] = recordStorePair.first;
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
        mStoreImp = std::make_unique<ESMStoreImp>(*this);
        std::apply([this](auto& ...x) {(ESMStoreImp::AssignStoreToIndex(*this, x), ...); }, mStoreImp->mStores);
        mDynamicCount = 0;
        mStoreImp->SetupAfterStoresCreation(*this);
        getWritable<ESM::Pathgrid>().setCells(getWritable<ESM::Cell>());
    }

    ESMStore::~ESMStore() //necessary for the destruction of of unique_ptr<ESMStoreImp>
    {
    }

    void ESMStore::clearDynamic()
    {
        for (const auto& store : mDynamicStores)
            store->clearDynamic();

        movePlayerRecord();
    }

static bool isCacheableRecord(int id)
{
    switch (id)
    {
        case ESM::REC_ACTI:
        case ESM::REC_ALCH:
        case ESM::REC_APPA:
        case ESM::REC_ARMO:
        case ESM::REC_BOOK:
        case ESM::REC_CLOT:
        case ESM::REC_CONT:
        case ESM::REC_CREA:
        case ESM::REC_DOOR:
        case ESM::REC_INGR:
        case ESM::REC_LEVC:
        case ESM::REC_LEVI:
        case ESM::REC_LIGH:
        case ESM::REC_LOCK:
        case ESM::REC_MISC:
        case ESM::REC_NPC_:
        case ESM::REC_PROB:
        case ESM::REC_REPA:
        case ESM::REC_STAT:
        case ESM::REC_WEAP:
        case ESM::REC_BODY:
            return true;
            break;
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
        const auto& it = mStoreImp->mRecNameToStore.find(recName);

        if (it == mStoreImp->mRecNameToStore.end()) {
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

static std::size_t sTypeIndexCounter = 0;

std::size_t& ESMStore::getTypeIndexCounter()
{
    return sTypeIndexCounter;
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

    std::map<ESM::RecNameInts, DynamicStore*>::iterator storeIt = mStoreImp->mRecNameToStore.begin();
    for (; storeIt != mStoreImp->mRecNameToStore.end(); ++storeIt) {
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
                mStoreImp->mRecNameToStore[type]->read (reader);
                return true;
            case ESM::REC_NPC_:
            case ESM::REC_CREA:
            case ESM::REC_CONT:
                mStoreImp->mRecNameToStore[type]->read (reader, true);
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


    template<> const ESM::Book* ESMStore::insert<ESM::Book>(const ESM::Book &toInsert) { return ESMStoreImp::esm3StoreInsert(*this, toInsert);}
    template<> const ESM::Armor* ESMStore::insert<ESM::Armor>(const ESM::Armor &toInsert) { return ESMStoreImp::esm3StoreInsert(*this, toInsert);}
    template<> const ESM::Class* ESMStore::insert<ESM::Class>(const ESM::Class &toInsert) { return ESMStoreImp::esm3StoreInsert(*this, toInsert);}
    template<> const ESM::Enchantment* ESMStore::insert<ESM::Enchantment>(const ESM::Enchantment &toInsert) { return ESMStoreImp::esm3StoreInsert(*this, toInsert);}
    template<> const ESM::Potion* ESMStore::insert<ESM::Potion>(const ESM::Potion &toInsert) { return ESMStoreImp::esm3StoreInsert(*this, toInsert);}
    template<> const ESM::Weapon* ESMStore::insert<ESM::Weapon>(const ESM::Weapon &toInsert) { return ESMStoreImp::esm3StoreInsert(*this, toInsert);}
    template<> const ESM::Clothing* ESMStore::insert<ESM::Clothing>(const ESM::Clothing &toInsert) { return ESMStoreImp::esm3StoreInsert(*this, toInsert);}
    template<> const ESM::Spell* ESMStore::insert<ESM::Spell>(const ESM::Spell &toInsert) { return ESMStoreImp::esm3StoreInsert(*this, toInsert);}


    template<> const ESM::GameSetting* ESMStore::insertStatic<ESM::GameSetting>(const ESM::GameSetting &toInsert) { return ESMStoreImp::esm3insertStatic(*this, toInsert);   }
    template<> const ESM::Static* ESMStore::insertStatic<ESM::Static>(const ESM::Static &toInsert) { return ESMStoreImp::esm3insertStatic(*this, toInsert);   }
    template<> const ESM::Door* ESMStore::insertStatic<ESM::Door>(const ESM::Door &toInsert) { return ESMStoreImp::esm3insertStatic(*this, toInsert);   }
    template<> const ESM::Global* ESMStore::insertStatic<ESM::Global>(const ESM::Global &toInsert) { return ESMStoreImp::esm3insertStatic(*this, toInsert);   }
    template<> const ESM::NPC* ESMStore::insertStatic<ESM::NPC>(const ESM::NPC &toInsert) { return ESMStoreImp::esm3insertStatic(*this, toInsert);   }


    template<> const ESM::Container* ESMStore::overrideRecord<ESM::Container>(const ESM::Container &toInsert) { return ESMStoreImp::esm3overrideRecord(*this, toInsert);   }
    template<> const ESM::Creature* ESMStore::overrideRecord<ESM::Creature>(const ESM::Creature &toInsert) { return ESMStoreImp::esm3overrideRecord(*this, toInsert);   }
    template<> const ESM::CreatureLevList* ESMStore::overrideRecord<ESM::CreatureLevList>(const ESM::CreatureLevList &toInsert) { return ESMStoreImp::esm3overrideRecord(*this, toInsert);   }
    template<> const ESM::Door* ESMStore::overrideRecord<ESM::Door>(const ESM::Door &toInsert) { return ESMStoreImp::esm3overrideRecord(*this, toInsert);   }
    template<> const ESM::ItemLevList* ESMStore::overrideRecord<ESM::ItemLevList>(const ESM::ItemLevList &toInsert) { return ESMStoreImp::esm3overrideRecord(*this, toInsert);   }
    template<> const ESM::NPC* ESMStore::overrideRecord<ESM::NPC>(const ESM::NPC &toInsert) { return ESMStoreImp::esm3overrideRecord(*this, toInsert);   }
} // end namespace
