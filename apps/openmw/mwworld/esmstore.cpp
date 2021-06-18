#include "esmstore.hpp"

#include <algorithm>
#include <set>

#include <boost/filesystem/operations.hpp>

#include <components/debug/debuglog.hpp>
#include <components/loadinglistener/loadinglistener.hpp>
#include <components/esm/esmreader.hpp>
#include <components/esm/esmwriter.hpp>
#include <components/misc/algorithm.hpp>

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

    void readRefs(const ESM::Cell& cell, std::vector<Ref>& refs, std::vector<std::string>& refIDs, std::vector<ESM::ESMReader>& readers)
    {
        for (size_t i = 0; i < cell.mContextList.size(); i++)
        {
            size_t index = cell.mContextList[i].index;
            if (readers.size() <= index)
                readers.resize(index + 1);
            cell.restore(readers[index], i);
            ESM::CellRef ref;
            ref.mRefNum.mContentFile = ESM::RefNum::RefNum_NoContentFile;
            bool deleted = false;
            while(cell.getNextRef(readers[index], ref, deleted))
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

    std::vector<ESM::NPC> getNPCsToReplace(const MWWorld::Store<ESM::Faction>& factions, const MWWorld::Store<ESM::Class>& classes, const std::map<std::string, ESM::NPC>& npcs)
    {
        // Cache first class from store - we will use it if current class is not found
        std::string defaultCls;
        auto it = classes.begin();
        if (it != classes.end())
            defaultCls = it->mId;
        else
            throw std::runtime_error("List of NPC classes is empty!");

        // Validate NPCs for non-existing class and faction.
        // We will replace invalid entries by fixed ones
        std::vector<ESM::NPC> npcsToReplace;

        for (const auto& npcIter : npcs)
        {
            ESM::NPC npc = npcIter.second;
            bool changed = false;

            const std::string npcFaction = npc.mFaction;
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

            std::string npcClass = npc.mClass;
            if (!npcClass.empty())
            {
                const ESM::Class *cls = classes.search(npcClass);
                if (!cls)
                {
                    Log(Debug::Verbose) << "NPC '" << npc.mId << "' (" << npc.mName << ") has nonexistent class '" << npc.mClass << "', using '" << defaultCls << "' class as replacement.";
                    npc.mClass = defaultCls;
                    changed = true;
                }
            }

            if (changed)
                npcsToReplace.push_back(npc);
        }

        return npcsToReplace;
    }
}

namespace MWWorld
{

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

void ESMStore::load(ESM::ESMReader &esm, Loading::Listener* listener)
{
    listener->setProgressRange(1000);

    ESM::Dialogue *dialogue = nullptr;

    // Land texture loading needs to use a separate internal store for each plugin.
    // We set the number of plugins here to avoid continual resizes during loading,
    // and so we can properly verify if valid plugin indices are being passed to the
    // LandTexture Store retrieval methods.
    mLandTextures.resize(esm.getGlobalReaderList()->size());

    /// \todo Move this to somewhere else. ESMReader?
    // Cache parent esX files by tracking their indices in the global list of
    //  all files/readers used by the engine. This will greaty accelerate
    //  refnumber mangling, as required for handling moved references.
    const std::vector<ESM::Header::MasterData> &masters = esm.getGameFiles();
    std::vector<ESM::ESMReader> *allPlugins = esm.getGlobalReaderList();
    for (size_t j = 0; j < masters.size(); j++) {
        const ESM::Header::MasterData &mast = masters[j];
        std::string fname = mast.name;
        int index = ~0;
        for (int i = 0; i < esm.getIndex(); i++) {
            const std::string candidate = allPlugins->at(i).getContext().filename;
            std::string fnamecandidate = boost::filesystem::path(candidate).filename().string();
            if (Misc::StringUtils::ciEqual(fname, fnamecandidate)) {
                index = i;
                break;
            }
        }
        if (index == (int)~0) {
            // Tried to load a parent file that has not been loaded yet. This is bad,
            //  the launcher should have taken care of this.
            std::string fstring = "File " + esm.getName() + " asks for parent file " + masters[j].name
                + ", but it has not been loaded yet. Please check your load order.";
            esm.fail(fstring);
        }
        esm.addParentFileIndex(index);
    }

    // Loop through all records
    while(esm.hasMoreRecs())
    {
        ESM::NAME n = esm.getRecName();
        esm.getRecHeader();

        // Look up the record type.
        std::map<int, StoreBase *>::iterator it = mStores.find(n.intval);

        if (it == mStores.end()) {
            if (n.intval == ESM::REC_INFO) {
                if (dialogue)
                {
                    dialogue->readInfo(esm, esm.getIndex() != 0);
                }
                else
                {
                    Log(Debug::Error) << "Error: info record without dialog";
                    esm.skipRecord();
                }
            } else if (n.intval == ESM::REC_MGEF) {
                mMagicEffects.load (esm);
            } else if (n.intval == ESM::REC_SKIL) {
                mSkills.load (esm);
            }
            else if (n.intval==ESM::REC_FILT || n.intval == ESM::REC_DBGP)
            {
                // ignore project file only records
                esm.skipRecord();
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

            if (n.intval==ESM::REC_DIAL) {
                dialogue = const_cast<ESM::Dialogue*>(mDialogs.find(id.mId));
            } else {
                dialogue = nullptr;
            }
        }
        listener->setProgress(static_cast<size_t>(esm.getFileOffset() / (float)esm.getFileSize() * 1000));
    }
}

void ESMStore::setUp(bool validateRecords)
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
        mStaticIds = mIds;

    mSkills.setUp();
    mMagicEffects.setUp();
    mAttributes.setUp();
    mDialogs.setUp();

    if (validateRecords)
    {
        validate();
        countRecords();
    }
}

void ESMStore::countRecords()
{
    if(!mRefCount.empty())
        return;
    std::vector<Ref> refs;
    std::vector<std::string> refIDs;
    std::vector<ESM::ESMReader> readers;
    for(auto it = mCells.intBegin(); it != mCells.intEnd(); it++)
        readRefs(*it, refs, refIDs, readers);
    for(auto it = mCells.extBegin(); it != mCells.extEnd(); it++)
        readRefs(*it, refs, refIDs, readers);
    const auto lessByRefNum = [] (const Ref& l, const Ref& r) { return l.mRefNum < r.mRefNum; };
    std::stable_sort(refs.begin(), refs.end(), lessByRefNum);
    const auto equalByRefNum = [] (const Ref& l, const Ref& r) { return l.mRefNum == r.mRefNum; };
    const auto incrementRefCount = [&] (const Ref& value)
    {
        if (value.mRefID != deletedRefID)
        {
            std::string& refId = refIDs[value.mRefID];
            Misc::StringUtils::lowerCaseInPlace(refId);
            ++mRefCount[std::move(refId)];
        }
    };
    Misc::forEachUnique(refs.rbegin(), refs.rend(), equalByRefNum, incrementRefCount);
}

int ESMStore::getRefCount(const std::string& id) const
{
    const std::string lowerId = Misc::StringUtils::lowerCase(id);
    auto it = mRefCount.find(lowerId);
    if(it == mRefCount.end())
        return 0;
    return it->second;
}

void ESMStore::validate()
{
    std::vector<ESM::NPC> npcsToReplace = getNPCsToReplace(mFactions, mClasses, mNpcs.mStatic);

    for (const ESM::NPC &npc : npcsToReplace)
    {
        mNpcs.eraseStatic(npc.mId);
        mNpcs.insertStatic(npc);
    }

    // Validate spell effects for invalid arguments
    std::vector<ESM::Spell> spellsToReplace;
    for (ESM::Spell spell : mSpells)
    {
        if (spell.mEffects.mList.empty())
            continue;

        bool changed = false;
        auto iter = spell.mEffects.mList.begin();
        while (iter != spell.mEffects.mList.end())
        {
            const ESM::MagicEffect* mgef = mMagicEffects.search(iter->mEffectID);
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
        mSpells.eraseStatic(spell.mId);
        mSpells.insertStatic(spell);
    }
}

void ESMStore::validateDynamic()
{
    std::vector<ESM::NPC> npcsToReplace = getNPCsToReplace(mFactions, mClasses, mNpcs.mDynamic);

    for (const ESM::NPC &npc : npcsToReplace)
        mNpcs.insert(npc);
}

    int ESMStore::countSavedGameRecords() const
    {
        return 1 // DYNA (dynamic name counter)
            +mPotions.getDynamicSize()
            +mArmors.getDynamicSize()
            +mBooks.getDynamicSize()
            +mClasses.getDynamicSize()
            +mClothes.getDynamicSize()
            +mEnchants.getDynamicSize()
            +mNpcs.getDynamicSize()
            +mSpells.getDynamicSize()
            +mWeapons.getDynamicSize()
            +mCreatureLists.getDynamicSize()
            +mItemLists.getDynamicSize()
            +mCreatures.getDynamicSize()
            +mContainers.getDynamicSize();
    }

    void ESMStore::write (ESM::ESMWriter& writer, Loading::Listener& progress) const
    {
        writer.startRecord(ESM::REC_DYNA);
        writer.startSubRecord("COUN");
        writer.writeT(mDynamicCount);
        writer.endRecord("COUN");
        writer.endRecord(ESM::REC_DYNA);

        mPotions.write (writer, progress);
        mArmors.write (writer, progress);
        mBooks.write (writer, progress);
        mClasses.write (writer, progress);
        mClothes.write (writer, progress);
        mEnchants.write (writer, progress);
        mSpells.write (writer, progress);
        mWeapons.write (writer, progress);
        mNpcs.write (writer, progress);
        mItemLists.write (writer, progress);
        mCreatureLists.write (writer, progress);
        mCreatures.write (writer, progress);
        mContainers.write (writer, progress);
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

        const ESM::NPC *player = mNpcs.find ("player");

        if (!mRaces.find (player->mRace) ||
            !mClasses.find (player->mClass))
            throw std::runtime_error ("Invalid player record (race or class unavailable");
    }

    std::pair<std::shared_ptr<MWMechanics::SpellList>, bool> ESMStore::getSpellList(const std::string& originalId) const
    {
        const std::string id = Misc::StringUtils::lowerCase(originalId);
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
} // end namespace
