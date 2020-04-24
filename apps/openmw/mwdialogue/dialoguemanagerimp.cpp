#include "dialoguemanagerimp.hpp"

#include <algorithm>
#include <list>

#include <components/debug/debuglog.hpp>

#include <components/esm/loaddial.hpp>
#include <components/esm/loadinfo.hpp>
#include <components/esm/dialoguestate.hpp>
#include <components/esm/esmwriter.hpp>

#include <components/compiler/exception.hpp>
#include <components/compiler/errorhandler.hpp>
#include <components/compiler/scanner.hpp>
#include <components/compiler/locals.hpp>
#include <components/compiler/output.hpp>
#include <components/compiler/scriptparser.hpp>

#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/defines.hpp>

#include <components/settings/settings.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/journal.hpp"
#include "../mwbase/scriptmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/soundmanager.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/esmstore.hpp"

#include "../mwscript/compilercontext.hpp"
#include "../mwscript/interpretercontext.hpp"
#include "../mwscript/extensions.hpp"

#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/actorutil.hpp"

#include "filter.hpp"
#include "hypertextparser.hpp"

namespace MWDialogue
{
    DialogueManager::DialogueManager (const Compiler::Extensions& extensions, Translation::Storage& translationDataStorage) :
      mTranslationDataStorage(translationDataStorage)
      , mCompilerContext (MWScript::CompilerContext::Type_Dialogue)
      , mErrorHandler()
      , mTalkedTo(false)
      , mTemporaryDispositionChange(0.f)
      , mPermanentDispositionChange(0.f)
    {
        mChoice = -1;
        mIsInChoice = false;
        mGoodbye = false;
        mCompilerContext.setExtensions (&extensions);
    }

    void DialogueManager::clear()
    {
        mKnownTopics.clear();
        mTalkedTo = false;
        mTemporaryDispositionChange = 0;
        mPermanentDispositionChange = 0;
    }

    void DialogueManager::addTopic (const std::string& topic)
    {
        mKnownTopics.insert( Misc::StringUtils::lowerCase(topic) );
    }

    void DialogueManager::parseText (const std::string& text)
    {
        updateActorKnownTopics();
        std::vector<HyperTextParser::Token> hypertext = HyperTextParser::parseHyperText(text);

        for (std::vector<HyperTextParser::Token>::iterator tok = hypertext.begin(); tok != hypertext.end(); ++tok)
        {
            std::string topicId = Misc::StringUtils::lowerCase(tok->mText);

            if (tok->isExplicitLink())
            {
                // calculation of standard form for all hyperlinks
                size_t asterisk_count = HyperTextParser::removePseudoAsterisks(topicId);
                for(; asterisk_count > 0; --asterisk_count)
                    topicId.append("*");

                topicId = mTranslationDataStorage.topicStandardForm(topicId);
            }

            if (mActorKnownTopics.count( topicId ))
                mKnownTopics.insert( topicId );
        }
    }

    bool DialogueManager::startDialogue (const MWWorld::Ptr& actor, ResponseCallback* callback)
    {
        updateGlobals();

        // Dialogue with dead actor (e.g. through script) should not be allowed.
        if (actor.getClass().getCreatureStats(actor).isDead())
            return false;

        mLastTopic = "";
        mPermanentDispositionChange = 0;
        mTemporaryDispositionChange = 0;

        mChoice = -1;
        mIsInChoice = false;
        mGoodbye = false;
        mChoices.clear();

        mActor = actor;

        MWMechanics::CreatureStats& creatureStats = actor.getClass().getCreatureStats (actor);
        mTalkedTo = creatureStats.hasTalkedToPlayer();

        mActorKnownTopics.clear();

        //greeting
        const MWWorld::Store<ESM::Dialogue> &dialogs =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::Dialogue>();

        Filter filter (actor, mChoice, mTalkedTo);

        for (MWWorld::Store<ESM::Dialogue>::iterator it = dialogs.begin(); it != dialogs.end(); ++it)
        {
            if(it->mType == ESM::Dialogue::Greeting)
            {
                // Search a response (we do not accept a fallback to "Info refusal" here)
                if (const ESM::DialInfo *info = filter.search (*it, false))
                {
                    creatureStats.talkedToPlayer();

                    if (!info->mSound.empty())
                    {
                        // TODO play sound
                    }

                    MWScript::InterpreterContext interpreterContext(&mActor.getRefData().getLocals(),mActor);
                    callback->addResponse("", Interpreter::fixDefinesDialog(info->mResponse, interpreterContext));
                    executeScript (info->mResultScript, mActor);
                    mLastTopic = it->mId;

                    parseText (info->mResponse);

                    return true;
                }
            }
        }
        return false;
    }

    bool DialogueManager::compile (const std::string& cmd, std::vector<Interpreter::Type_Code>& code, const MWWorld::Ptr& actor)
    {
        bool success = true;

        try
        {
            mErrorHandler.reset();

            mErrorHandler.setContext("[dialogue script]");

            std::istringstream input (cmd + "\n");

            Compiler::Scanner scanner (mErrorHandler, input, mCompilerContext.getExtensions());

            Compiler::Locals locals;

            std::string actorScript = actor.getClass().getScript (actor);

            if (!actorScript.empty())
            {
                // grab local variables from actor's script, if available.
                locals = MWBase::Environment::get().getScriptManager()->getLocals (actorScript);
            }

            Compiler::ScriptParser parser(mErrorHandler,mCompilerContext, locals, false);

            scanner.scan (parser);

            if (!mErrorHandler.isGood())
                success = false;

            if (success)
                parser.getCode (code);
        }
        catch (const Compiler::SourceException& /* error */)
        {
            // error has already been reported via error handler
            success = false;
        }
        catch (const std::exception& error)
        {
            Log(Debug::Error) << std::string ("Dialogue error: An exception has been thrown: ") + error.what();
            success = false;
        }

        if (!success)
        {
            Log(Debug::Error) << "Error: compiling failed (dialogue script): \n" << cmd << "\n";
        }

        return success;
    }

    void DialogueManager::executeScript (const std::string& script, const MWWorld::Ptr& actor)
    {
        std::vector<Interpreter::Type_Code> code;
        if(compile(script, code, actor))
        {
            try
            {
                MWScript::InterpreterContext interpreterContext(&actor.getRefData().getLocals(), actor);
                Interpreter::Interpreter interpreter;
                MWScript::installOpcodes (interpreter);
                interpreter.run (&code[0], code.size(), interpreterContext);
            }
            catch (const std::exception& error)
            {
               Log(Debug::Error) << std::string ("Dialogue error: An exception has been thrown: ") + error.what();
            }
        }
    }

    void DialogueManager::executeTopic (const std::string& topic, ResponseCallback* callback)
    {
        Filter filter (mActor, mChoice, mTalkedTo);

        const MWWorld::Store<ESM::Dialogue> &dialogues =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::Dialogue>();

        const ESM::Dialogue& dialogue = *dialogues.find (topic);

        const ESM::DialInfo* info = filter.search(dialogue, true);
        if (info)
        {
            std::string title;
            if (dialogue.mType==ESM::Dialogue::Persuasion)
            {
                // Determine GMST from dialogue topic. GMSTs are:
                // sAdmireSuccess, sAdmireFail, sIntimidateSuccess, sIntimidateFail,
                // sTauntSuccess, sTauntFail, sBribeSuccess, sBribeFail
                std::string modifiedTopic = "s" + topic;

                modifiedTopic.erase (std::remove (modifiedTopic.begin(), modifiedTopic.end(), ' '), modifiedTopic.end());

                const MWWorld::Store<ESM::GameSetting>& gmsts =
                    MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();

                title = gmsts.find (modifiedTopic)->mValue.getString();
            }
            else
                title = topic;

            MWScript::InterpreterContext interpreterContext(&mActor.getRefData().getLocals(),mActor);
            callback->addResponse(title, Interpreter::fixDefinesDialog(info->mResponse, interpreterContext));

            if (dialogue.mType == ESM::Dialogue::Topic)
            {
                // Make sure the returned DialInfo is from the Dialogue we supplied. If could also be from the Info refusal group,
                // in which case it should not be added to the journal.
                for (ESM::Dialogue::InfoContainer::const_iterator iter = dialogue.mInfo.begin();
                    iter!=dialogue.mInfo.end(); ++iter)
                {
                    if (iter->mId == info->mId)
                    {
                        MWBase::Environment::get().getJournal()->addTopic (Misc::StringUtils::lowerCase(topic), info->mId, mActor);
                        break;
                    }
                }
            }

            executeScript (info->mResultScript, mActor);

            parseText (info->mResponse);

            mLastTopic = topic;
        }
    }

    const ESM::Dialogue *DialogueManager::searchDialogue(const std::string& id)
    {
        return MWBase::Environment::get().getWorld()->getStore().get<ESM::Dialogue>().search(id);
    }

    void DialogueManager::updateGlobals()
    {
        MWBase::Environment::get().getWorld()->updateDialogueGlobals();
    }

    void DialogueManager::updateActorKnownTopics()
    {
        updateGlobals();

        mActorKnownTopics.clear();

        const MWWorld::Store<ESM::Dialogue> &dialogs =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::Dialogue>();

        Filter filter (mActor, -1, mTalkedTo);

        for (MWWorld::Store<ESM::Dialogue>::iterator iter = dialogs.begin(); iter != dialogs.end(); ++iter)
        {
            if (iter->mType == ESM::Dialogue::Topic)
            {
                if (filter.responseAvailable (*iter))
                {
                    mActorKnownTopics.insert (iter->mId);
                }
            }
        }

    }

    std::list<std::string> DialogueManager::getAvailableTopics()
    {
        updateActorKnownTopics();

        std::list<std::string> keywordList;

        for (const std::string& topic : mActorKnownTopics)
        {
            //does the player know the topic?
            if (mKnownTopics.count(topic))
                keywordList.push_back(topic);
        }

        // sort again, because the previous sort was case-sensitive
        keywordList.sort(Misc::StringUtils::ciLess);
        return keywordList;
    }

    void DialogueManager::keywordSelected (const std::string& keyword, ResponseCallback* callback)
    {
        if(!mIsInChoice)
        {
            const ESM::Dialogue* dialogue = searchDialogue(keyword);
            if (dialogue && dialogue->mType == ESM::Dialogue::Topic)
            {
                executeTopic (keyword, callback);
            }
        }
    }

    bool DialogueManager::isInChoice() const
    {
        return mIsInChoice;
    }

    void DialogueManager::goodbyeSelected()
    {
        // Apply disposition change to NPC's base disposition
        if (mActor.getClass().isNpc())
        {
            // Clamp permanent disposition change so that final disposition doesn't go below 0 (could happen with intimidate)       
            float curDisp = static_cast<float>(MWBase::Environment::get().getMechanicsManager()->getDerivedDisposition(mActor, false));
            if (curDisp + mPermanentDispositionChange < 0)
                mPermanentDispositionChange = -curDisp;

            MWMechanics::NpcStats& npcStats = mActor.getClass().getNpcStats(mActor);
            npcStats.setBaseDisposition(static_cast<int>(npcStats.getBaseDisposition() + mPermanentDispositionChange));
        }
        mPermanentDispositionChange = 0;
        mTemporaryDispositionChange = 0;
    }

    void DialogueManager::questionAnswered (int answer, ResponseCallback* callback)
    {
        mChoice = answer;

        const ESM::Dialogue* dialogue = searchDialogue(mLastTopic);
        if (dialogue)
        {
            Filter filter (mActor, mChoice, mTalkedTo);

            if (dialogue->mType == ESM::Dialogue::Topic || dialogue->mType == ESM::Dialogue::Greeting)
            {
                if (const ESM::DialInfo *info = filter.search (*dialogue, true))
                {
                    std::string text = info->mResponse;
                    parseText (text);

                    mChoice = -1;
                    mIsInChoice = false;
                    mChoices.clear();

                    MWScript::InterpreterContext interpreterContext(&mActor.getRefData().getLocals(),mActor);
                    callback->addResponse("", Interpreter::fixDefinesDialog(text, interpreterContext));

                    if (dialogue->mType == ESM::Dialogue::Topic)
                    {
                        // Make sure the returned DialInfo is from the Dialogue we supplied. If could also be from the Info refusal group,
                        // in which case it should not be added to the journal
                        for (ESM::Dialogue::InfoContainer::const_iterator iter = dialogue->mInfo.begin();
                            iter!=dialogue->mInfo.end(); ++iter)
                        {
                            if (iter->mId == info->mId)
                            {
                                MWBase::Environment::get().getJournal()->addTopic (Misc::StringUtils::lowerCase(mLastTopic), info->mId, mActor);
                                break;
                            }
                        }
                    }

                    executeScript (info->mResultScript, mActor);
                }
                else
                {
                    mChoice = -1;
                    mIsInChoice = false;
                    mChoices.clear();
                }
            }
        }

        updateActorKnownTopics();
    }

    void DialogueManager::addChoice (const std::string& text, int choice)
    {
        mIsInChoice = true;
        mChoices.push_back(std::make_pair(text, choice));
    }

    const std::vector<std::pair<std::string, int> >& DialogueManager::getChoices()
    {
        return mChoices;
    }

    bool DialogueManager::isGoodbye()
    {
        return mGoodbye;
    }

    void DialogueManager::goodbye()
    {
        mIsInChoice = false;
        mGoodbye = true;
    }

    void DialogueManager::persuade(int type, ResponseCallback* callback)
    {
        bool success;
        float temp, perm;
        MWBase::Environment::get().getMechanicsManager()->getPersuasionDispositionChange(
                    mActor, MWBase::MechanicsManager::PersuasionType(type),
                    success, temp, perm);
        mTemporaryDispositionChange += temp;
        mPermanentDispositionChange += perm;

        // change temp disposition so that final disposition is between 0...100
        float curDisp = static_cast<float>(MWBase::Environment::get().getMechanicsManager()->getDerivedDisposition(mActor, false));
        if (curDisp + mTemporaryDispositionChange < 0)
            mTemporaryDispositionChange = -curDisp;
        else if (curDisp + mTemporaryDispositionChange > 100)
            mTemporaryDispositionChange = 100 - curDisp;

        MWWorld::Ptr player = MWMechanics::getPlayer();
        player.getClass().skillUsageSucceeded(player, ESM::Skill::Speechcraft, success ? 0 : 1);

        if (success)
        {
            int gold=0;
            if (type == MWBase::MechanicsManager::PT_Bribe10)
                gold = 10;
            else if (type == MWBase::MechanicsManager::PT_Bribe100)
                gold = 100;
            else if (type == MWBase::MechanicsManager::PT_Bribe1000)
                gold = 1000;

            if (gold)
            {
                player.getClass().getContainerStore(player).remove(MWWorld::ContainerStore::sGoldId, gold, player);
                mActor.getClass().getContainerStore(mActor).add(MWWorld::ContainerStore::sGoldId, gold, mActor);
            }
        }

        std::string text;

        if (type == MWBase::MechanicsManager::PT_Admire)
            text = "Admire";
        else if (type == MWBase::MechanicsManager::PT_Taunt)
            text = "Taunt";
        else if (type == MWBase::MechanicsManager::PT_Intimidate)
            text = "Intimidate";
        else{
            text = "Bribe";
        }

        executeTopic (text + (success ? " Success" : " Fail"), callback);
    }

    int DialogueManager::getTemporaryDispositionChange() const
    {
        return static_cast<int>(mTemporaryDispositionChange);
    }

    void DialogueManager::applyBarterDispositionChange(int delta)
    {
        mTemporaryDispositionChange += delta;
        if (Settings::Manager::getBool("barter disposition change is permanent", "Game"))
            mPermanentDispositionChange += delta;
    }

    bool DialogueManager::checkServiceRefused(ResponseCallback* callback)
    {
        Filter filter (mActor, mChoice, mTalkedTo);

        const MWWorld::Store<ESM::Dialogue> &dialogues =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::Dialogue>();

        const ESM::Dialogue& dialogue = *dialogues.find ("Service Refusal");

        std::vector<const ESM::DialInfo *> infos = filter.list (dialogue, false, false, true);
        if (!infos.empty())
        {
            const ESM::DialInfo* info = infos[0];

            parseText (info->mResponse);

            const MWWorld::Store<ESM::GameSetting>& gmsts =
                MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();

            MWScript::InterpreterContext interpreterContext(&mActor.getRefData().getLocals(),mActor);

            callback->addResponse(gmsts.find ("sServiceRefusal")->mValue.getString(), Interpreter::fixDefinesDialog(info->mResponse, interpreterContext));

            executeScript (info->mResultScript, mActor);
            return true;
        }
        return false;
    }

    void DialogueManager::say(const MWWorld::Ptr &actor, const std::string &topic)
    {
        MWBase::SoundManager *sndMgr = MWBase::Environment::get().getSoundManager();
        if(sndMgr->sayActive(actor))
        {
            // Actor is already saying something.
            return;
        }

        if (actor.getClass().isNpc() && MWBase::Environment::get().getWorld()->isSwimming(actor))
        {
            // NPCs don't talk while submerged
            return;
        }

        if (actor.getClass().getCreatureStats(actor).getKnockedDown())
        {
            // Unconscious actors can not speak
            return;
        }

        const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
        const ESM::Dialogue *dial = store.get<ESM::Dialogue>().find(topic);

        const MWMechanics::CreatureStats& creatureStats = actor.getClass().getCreatureStats(actor);
        Filter filter(actor, 0, creatureStats.hasTalkedToPlayer());
        const ESM::DialInfo *info = filter.search(*dial, false);
        if(info != nullptr)
        {
            MWBase::WindowManager *winMgr = MWBase::Environment::get().getWindowManager();
            if(winMgr->getSubtitlesEnabled())
                winMgr->messageBox(info->mResponse);
            if (!info->mSound.empty())
                sndMgr->say(actor, info->mSound);
            if (!info->mResultScript.empty())
                executeScript(info->mResultScript, actor);
        }
    }

    int DialogueManager::countSavedGameRecords() const
    {
        return 1; // known topics
    }

    void DialogueManager::write (ESM::ESMWriter& writer, Loading::Listener& progress) const
    {
        ESM::DialogueState state;

        for (std::set<std::string>::const_iterator iter (mKnownTopics.begin());
            iter!=mKnownTopics.end(); ++iter)
        {
            state.mKnownTopics.push_back (*iter);
        }

        state.mChangedFactionReaction = mChangedFactionReaction;

        writer.startRecord (ESM::REC_DIAS);
        state.save (writer);
        writer.endRecord (ESM::REC_DIAS);
    }

    void DialogueManager::readRecord (ESM::ESMReader& reader, uint32_t type)
    {
        if (type==ESM::REC_DIAS)
        {
            const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();

            ESM::DialogueState state;
            state.load (reader);

            for (std::vector<std::string>::const_iterator iter (state.mKnownTopics.begin());
                iter!=state.mKnownTopics.end(); ++iter)
                if (store.get<ESM::Dialogue>().search (*iter))
                    mKnownTopics.insert (*iter);

            mChangedFactionReaction = state.mChangedFactionReaction;
        }
    }

    void DialogueManager::modFactionReaction(const std::string &faction1, const std::string &faction2, int diff)
    {
        std::string fact1 = Misc::StringUtils::lowerCase(faction1);
        std::string fact2 = Misc::StringUtils::lowerCase(faction2);

        // Make sure the factions exist
        MWBase::Environment::get().getWorld()->getStore().get<ESM::Faction>().find(fact1);
        MWBase::Environment::get().getWorld()->getStore().get<ESM::Faction>().find(fact2);

        int newValue = getFactionReaction(faction1, faction2) + diff;

        std::map<std::string, int>& map = mChangedFactionReaction[fact1];
        map[fact2] = newValue;
    }

    void DialogueManager::setFactionReaction(const std::string &faction1, const std::string &faction2, int absolute)
    {
        std::string fact1 = Misc::StringUtils::lowerCase(faction1);
        std::string fact2 = Misc::StringUtils::lowerCase(faction2);

        // Make sure the factions exist
        MWBase::Environment::get().getWorld()->getStore().get<ESM::Faction>().find(fact1);
        MWBase::Environment::get().getWorld()->getStore().get<ESM::Faction>().find(fact2);

        std::map<std::string, int>& map = mChangedFactionReaction[fact1];
        map[fact2] = absolute;
    }

    int DialogueManager::getFactionReaction(const std::string &faction1, const std::string &faction2) const
    {
        std::string fact1 = Misc::StringUtils::lowerCase(faction1);
        std::string fact2 = Misc::StringUtils::lowerCase(faction2);

        ModFactionReactionMap::const_iterator map = mChangedFactionReaction.find(fact1);
        if (map != mChangedFactionReaction.end() && map->second.find(fact2) != map->second.end())
            return map->second.at(fact2);

        const ESM::Faction* faction = MWBase::Environment::get().getWorld()->getStore().get<ESM::Faction>().find(fact1);

        std::map<std::string, int>::const_iterator it = faction->mReactions.begin();
        for (; it != faction->mReactions.end(); ++it)
        {
            if (Misc::StringUtils::ciEqual(it->first, fact2))
                    return it->second;
        }
        return 0;
    }

    void DialogueManager::clearInfoActor(const MWWorld::Ptr &actor) const
    {
        if (actor == mActor && !mLastTopic.empty())
        {
            MWBase::Environment::get().getJournal()->removeLastAddedTopicResponse(
                        Misc::StringUtils::lowerCase(mLastTopic), actor.getClass().getName(actor));
        }
    }
}
