#include "dialoguebindings.hpp"
#include "context.hpp"
#include "recordstore.hpp"
#include "apps/openmw/mwworld/store.hpp"
#include <components/esm3/loaddial.hpp>
#include <components/lua/luastate.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/vfs/pathutil.hpp>

namespace
{
    sol::table prepareJournalRecord(sol::state_view& lua, const ESM::Dialogue& mwDialogue)
    {
        const auto dialogueRecordId = mwDialogue.mId.serializeText();
        sol::table result(lua, sol::create);
        result["text"] = mwDialogue.mStringId;

        sol::table preparedInfos(lua, sol::create);
        unsigned index = 1;
        for (const auto& mwDialogueInfo : mwDialogue.mInfo)
        {
            if (mwDialogueInfo.mQuestStatus == ESM::DialInfo::QuestStatus::QS_Name)
            {
                result["questName"] = mwDialogueInfo.mResponse;
                continue;
            }
            sol::table infoElement(lua, sol::create);
            infoElement["id"] = (dialogueRecordId + '#' + mwDialogueInfo.mId.serializeText());
            infoElement["text"] = mwDialogueInfo.mResponse;
            infoElement["questStage"] = mwDialogueInfo.mData.mJournalIndex;
            infoElement["questFinished"] = (mwDialogueInfo.mQuestStatus == ESM::DialInfo::QuestStatus::QS_Finished);
            infoElement["questRestart"] = (mwDialogueInfo.mQuestStatus == ESM::DialInfo::QuestStatus::QS_Restart);
            preparedInfos[index++] = infoElement;
        }

        result["infos"] = LuaUtil::makeStrictReadOnly(preparedInfos);
        return result;
    }

    sol::table prepareNonJournalRecord(sol::state_view& lua, const ESM::Dialogue& mwDialogue)
    {
        const auto dialogueRecordId = mwDialogue.mId.serializeText();
        sol::table result(lua, sol::create);
        result["text"] = mwDialogue.mStringId;

        sol::table preparedInfos(lua, sol::create);
        unsigned index = 1;
        for (const auto& mwDialogueInfo : mwDialogue.mInfo)
        {
            sol::table infoElement(lua, sol::create);
            infoElement["id"] = (dialogueRecordId + '#' + mwDialogueInfo.mId.serializeText());
            infoElement["text"] = mwDialogueInfo.mResponse;
            infoElement["actorId"] = mwDialogueInfo.mActor.serializeText();
            infoElement["actorRace"] = mwDialogueInfo.mRace.serializeText();
            infoElement["actorClass"] = mwDialogueInfo.mClass.serializeText();
            infoElement["actorFaction"] = mwDialogueInfo.mFaction.serializeText();
            if (mwDialogueInfo.mData.mRank != -1)
            {
                infoElement["actorFactionRank"] = mwDialogueInfo.mData.mRank;
            }
            infoElement["actorCell"] = mwDialogueInfo.mClass.serializeText();
            infoElement["actorDisposition"] = mwDialogueInfo.mData.mDisposition;
            if (mwDialogueInfo.mData.mGender != ESM::DialInfo::Gender::NA)
            {
                infoElement["actorGender"] = mwDialogueInfo.mData.mGender;
            }
            infoElement["playerFaction"] = mwDialogueInfo.mPcFaction.serializeText();
            if (mwDialogueInfo.mData.mPCrank != -1)
            {
                infoElement["playerFactionRank"] = mwDialogueInfo.mData.mPCrank;
            }
            if (not mwDialogueInfo.mSound.empty())
            {
                infoElement["sound"] = Misc::ResourceHelpers::correctSoundPath(VFS::Path::Normalized(mwDialogueInfo.mSound)).value();
            }
            //mResultScript TODO
            //mSelects TODO
            preparedInfos[index++] = infoElement;
        }

        result["infos"] = LuaUtil::makeStrictReadOnly(preparedInfos);
        return result;
    }
}

namespace MWLua
{

    sol::table initCoreDialogueBindings(const Context& context)
    {
        sol::state_view& lua = context.mLua->sol();
        sol::table api(lua, sol::create);

        const MWWorld::Store<ESM::Dialogue>& mwDialogueStore = MWBase::Environment::get().getESMStore()->get<ESM::Dialogue>();

        sol::table journalRecordsByQuestId(lua, sol::create);
        sol::table topicRecordsByTopicId(lua, sol::create);
        sol::table voiceRecordsById(lua, sol::create);
        sol::table greetingRecordsById(lua, sol::create);
        sol::table persuasionRecordsById(lua, sol::create);
        for (const auto& mwDialogue : mwDialogueStore)
        {
            const auto dialogueRecordId = mwDialogue.mId.serializeText();
            if (mwDialogue.mType == ESM::Dialogue::Type::Journal)
            {
                journalRecordsByQuestId[dialogueRecordId] = prepareJournalRecord(lua, mwDialogue);
            }
            else if (mwDialogue.mType == ESM::Dialogue::Type::Topic)
            {
                topicRecordsByTopicId[dialogueRecordId] = prepareNonJournalRecord(lua, mwDialogue);
            }
            else if (mwDialogue.mType == ESM::Dialogue::Type::Voice)
            {
                voiceRecordsById[dialogueRecordId] = prepareNonJournalRecord(lua, mwDialogue);
            }
            else if (mwDialogue.mType == ESM::Dialogue::Type::Greeting)
            {
                greetingRecordsById[dialogueRecordId] = prepareNonJournalRecord(lua, mwDialogue);
            }
            else if (mwDialogue.mType == ESM::Dialogue::Type::Persuasion)
            {
                persuasionRecordsById[dialogueRecordId] = prepareNonJournalRecord(lua, mwDialogue);
            }
        }
        api["journalRecords"] = LuaUtil::makeStrictReadOnly(journalRecordsByQuestId);
        api["topicRecords"] = LuaUtil::makeStrictReadOnly(topicRecordsByTopicId);
        api["voiceRecords"] = LuaUtil::makeStrictReadOnly(voiceRecordsById);
        api["greetingRecords"] = LuaUtil::makeStrictReadOnly(greetingRecordsById);
        api["persuasionRecords"] = LuaUtil::makeStrictReadOnly(persuasionRecordsById);

        return LuaUtil::makeReadOnly(api);
    }
}
