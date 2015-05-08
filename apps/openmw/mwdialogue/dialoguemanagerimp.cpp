
#include "dialoguemanagerimp.hpp"

#include <cctype>
#include <cstdlib>
#include <algorithm>
#include <iterator>
#include <list>

#include <components/esm/loaddial.hpp>
#include <components/esm/loadinfo.hpp>
#include <components/esm/dialoguestate.hpp>

#include <components/compiler/exception.hpp>
#include <components/compiler/errorhandler.hpp>
#include <components/compiler/scanner.hpp>
#include <components/compiler/locals.hpp>
#include <components/compiler/output.hpp>
#include <components/compiler/scriptparser.hpp>

#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/defines.hpp>

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

#include "../mwgui/dialogue.hpp"

#include "../mwscript/compilercontext.hpp"
#include "../mwscript/interpretercontext.hpp"
#include "../mwscript/extensions.hpp"

#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/npcstats.hpp"

#include "filter.hpp"
#include "hypertextparser.hpp"

namespace MWDialogue
{
    DialogueManager::DialogueManager (const Compiler::Extensions& extensions, bool scriptVerbose, Translation::Storage& translationDataStorage) :
      mCompilerContext (MWScript::CompilerContext::Type_Dialogue),
        mErrorStream(std::cout.rdbuf()),mErrorHandler(mErrorStream)
      , mTemporaryDispositionChange(0.f)
      , mPermanentDispositionChange(0.f), mScriptVerbose (scriptVerbose)
      , mTranslationDataStorage(translationDataStorage)
      , mTalkedTo(false)
    {
        mChoice = -1;
        mIsInChoice = false;
        mCompilerContext.setExtensions (&extensions);

        const MWWorld::Store<ESM::Dialogue> &dialogs =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::Dialogue>();

        MWWorld::Store<ESM::Dialogue>::iterator it = dialogs.begin();
        for (; it != dialogs.end(); ++it)
        {
            mDialogueMap[Misc::StringUtils::lowerCase(it->mId)] = *it;
        }
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

            if (tok->isImplicitKeyword() && mTranslationDataStorage.hasTranslation())
                continue;

            if (mActorKnownTopics.count( topicId ))
                mKnownTopics.insert( topicId );
        }

        updateTopics();
    }

    void DialogueManager::startDialogue (const MWWorld::Ptr& actor)
    {
        // Dialogue with dead actor (e.g. through script) should not be allowed.
        if (actor.getClass().getCreatureStats(actor).isDead())
            return;

        mLastTopic = "";
        mPermanentDispositionChange = 0;
        mTemporaryDispositionChange = 0;

        mChoice = -1;
        mIsInChoice = false;

        mActor = actor;

        MWMechanics::CreatureStats& creatureStats = actor.getClass().getCreatureStats (actor);
        mTalkedTo = creatureStats.hasTalkedToPlayer();

        mActorKnownTopics.clear();

        MWGui::DialogueWindow* win = MWBase::Environment::get().getWindowManager()->getDialogueWindow();

        // If the dialogue window was already open, keep the existing history
        bool resetHistory = (!MWBase::Environment::get().getWindowManager()->containsMode(MWGui::GM_Dialogue));

        win->startDialogue(actor, actor.getClass().getName (actor), resetHistory);

        //setup the list of topics known by the actor. Topics who are also on the knownTopics list will be added to the GUI
        updateTopics();

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
                    //initialise the GUI
                    MWBase::Environment::get().getWindowManager()->pushGuiMode(MWGui::GM_Dialogue);

                    creatureStats.talkedToPlayer();

                    if (!info->mSound.empty())
                    {
                        // TODO play sound
                    }

                    parseText (info->mResponse);

                    MWScript::InterpreterContext interpreterContext(&mActor.getRefData().getLocals(),mActor);
                    win->addResponse (Interpreter::fixDefinesDialog(info->mResponse, interpreterContext));
                    executeScript (info->mResultScript);
                    mLastTopic = Misc::StringUtils::lowerCase(it->mId);
                    return;
                }
            }
        }

        // No greetings found. The dialogue window should not be shown.
        // If this is a companion, we must show the companion window directly (used by BM_bear_be_unique).
        bool isCompanion = !mActor.getClass().getScript(mActor).empty()
                && mActor.getRefData().getLocals().getIntVar(mActor.getClass().getScript(mActor), "companion");
        if (isCompanion)
            MWBase::Environment::get().getWindowManager()->showCompanionWindow(mActor);
    }

    bool DialogueManager::compile (const std::string& cmd,std::vector<Interpreter::Type_Code>& code)
    {
        bool success = true;

        try
        {
            mErrorHandler.reset();

            std::istringstream input (cmd + "\n");

            Compiler::Scanner scanner (mErrorHandler, input, mCompilerContext.getExtensions());

            Compiler::Locals locals;

            std::string actorScript = mActor.getClass().getScript (mActor);

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
            std::cerr << std::string ("Dialogue error: An exception has been thrown: ") + error.what() << std::endl;
            success = false;
        }

        if (!success)
        {
            std::cerr
                << "compiling failed (dialogue script)" << std::endl
                << cmd
                << std::endl << std::endl;
        }

        return success;
    }

    void DialogueManager::executeScript (const std::string& script)
    {
        std::vector<Interpreter::Type_Code> code;
        if(compile(script,code))
        {
            try
            {
                MWScript::InterpreterContext interpreterContext(&mActor.getRefData().getLocals(),mActor);
                Interpreter::Interpreter interpreter;
                MWScript::installOpcodes (interpreter);
                interpreter.run (&code[0], code.size(), interpreterContext);
            }
            catch (const std::exception& error)
            {
                std::cerr << std::string ("Dialogue error: An exception has been thrown: ") + error.what() << std::endl;
            }
        }
    }

    void DialogueManager::executeTopic (const std::string& topic)
    {
        Filter filter (mActor, mChoice, mTalkedTo);

        const MWWorld::Store<ESM::Dialogue> &dialogues =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::Dialogue>();

        const ESM::Dialogue& dialogue = *dialogues.find (topic);

        MWGui::DialogueWindow* win = MWBase::Environment::get().getWindowManager()->getDialogueWindow();

        const ESM::DialInfo* info = filter.search(dialogue, true);
        if (info)
        {
            parseText (info->mResponse);

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

                title = gmsts.find (modifiedTopic)->getString();
            }
            else
                title = topic;

            MWScript::InterpreterContext interpreterContext(&mActor.getRefData().getLocals(),mActor);
            win->addResponse (Interpreter::fixDefinesDialog(info->mResponse, interpreterContext), title);

            // Make sure the returned DialInfo is from the Dialogue we supplied. If could also be from the Info refusal group,
            // in which case it should not be added to the journal.
            for (ESM::Dialogue::InfoContainer::const_iterator iter = dialogue.mInfo.begin();
                iter!=dialogue.mInfo.end(); ++iter)
            {
                if (iter->mId == info->mId)
                {
                    MWBase::Environment::get().getJournal()->addTopic (topic, info->mId, mActor);
                    break;
                }
            }

            executeScript (info->mResultScript);

            mLastTopic = topic;
        }
        else
        {
            // no response found, print a fallback text
            win->addResponse ("…", topic);
        }
    }

    void DialogueManager::updateGlobals()
    {
        MWBase::Environment::get().getWorld()->updateDialogueGlobals();
    }

    void DialogueManager::updateTopics()
    {
        std::list<std::string> keywordList;
        int choice = mChoice;
        mChoice = -1;
        mActorKnownTopics.clear();

        const MWWorld::Store<ESM::Dialogue> &dialogs =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::Dialogue>();

        Filter filter (mActor, mChoice, mTalkedTo);

        for (MWWorld::Store<ESM::Dialogue>::iterator iter = dialogs.begin(); iter != dialogs.end(); ++iter)
        {
            if (iter->mType == ESM::Dialogue::Topic)
            {
                if (filter.responseAvailable (*iter))
                {
                    std::string lower = Misc::StringUtils::lowerCase(iter->mId);
                    mActorKnownTopics.insert (lower);

                    //does the player know the topic?
                    if (mKnownTopics.count(lower))
                    {
                        keywordList.push_back (iter->mId);
                    }
                }
            }
        }

        // check the available services of this actor
        int services = 0;
        if (mActor.getTypeName() == typeid(ESM::NPC).name())
        {
            MWWorld::LiveCellRef<ESM::NPC>* ref = mActor.get<ESM::NPC>();
            if (ref->mBase->mHasAI)
                services = ref->mBase->mAiData.mServices;
        }
        else if (mActor.getTypeName() == typeid(ESM::Creature).name())
        {
            MWWorld::LiveCellRef<ESM::Creature>* ref = mActor.get<ESM::Creature>();
            if (ref->mBase->mHasAI)
                services = ref->mBase->mAiData.mServices;
        }

        int windowServices = 0;

        if (services & ESM::NPC::Weapon
            || services & ESM::NPC::Armor
            || services & ESM::NPC::Clothing
            || services & ESM::NPC::Books
            || services & ESM::NPC::Ingredients
            || services & ESM::NPC::Picks
            || services & ESM::NPC::Probes
            || services & ESM::NPC::Lights
            || services & ESM::NPC::Apparatus
            || services & ESM::NPC::RepairItem
            || services & ESM::NPC::Misc)
            windowServices |= MWGui::DialogueWindow::Service_Trade;

        if((mActor.getTypeName() == typeid(ESM::NPC).name() && !mActor.get<ESM::NPC>()->mBase->getTransport().empty())
                || (mActor.getTypeName() == typeid(ESM::Creature).name() && !mActor.get<ESM::Creature>()->mBase->getTransport().empty()))
            windowServices |= MWGui::DialogueWindow::Service_Travel;

        if (services & ESM::NPC::Spells)
            windowServices |= MWGui::DialogueWindow::Service_BuySpells;

        if (services & ESM::NPC::Spellmaking)
            windowServices |= MWGui::DialogueWindow::Service_CreateSpells;

        if (services & ESM::NPC::Training)
            windowServices |= MWGui::DialogueWindow::Service_Training;

        if (services & ESM::NPC::Enchanting)
            windowServices |= MWGui::DialogueWindow::Service_Enchant;

        if (services & ESM::NPC::Repair)
            windowServices |= MWGui::DialogueWindow::Service_Repair;

        MWGui::DialogueWindow* win = MWBase::Environment::get().getWindowManager()->getDialogueWindow();

        win->setServices (windowServices);

        // sort again, because the previous sort was case-sensitive
        keywordList.sort(Misc::StringUtils::ciLess);
        win->setKeywords(keywordList);

        mChoice = choice;

        updateGlobals();
    }

    void DialogueManager::keywordSelected (const std::string& keyword)
    {
        if(!mIsInChoice)
        {
            if(mDialogueMap.find(keyword) != mDialogueMap.end())
            {
                ESM::Dialogue ndialogue = mDialogueMap[keyword];
                if (mDialogueMap[keyword].mType == ESM::Dialogue::Topic)
                {
                    executeTopic (keyword);
                }
            }
        }

        updateTopics();
    }

    bool DialogueManager::isInChoice() const
    {
        return mIsInChoice;
    }

    void DialogueManager::goodbyeSelected()
    {
        MWBase::Environment::get().getWindowManager()->removeGuiMode(MWGui::GM_Dialogue);

        // Apply disposition change to NPC's base disposition
        if (mActor.getClass().isNpc())
        {
            MWMechanics::NpcStats& npcStats = mActor.getClass().getNpcStats(mActor);
            npcStats.setBaseDisposition(static_cast<int>(npcStats.getBaseDisposition() + mPermanentDispositionChange));
        }
        mPermanentDispositionChange = 0;
        mTemporaryDispositionChange = 0;
    }

    void DialogueManager::questionAnswered (int answer)
    {
        mChoice = answer;

        if (mDialogueMap.find(mLastTopic) != mDialogueMap.end())
        {
            Filter filter (mActor, mChoice, mTalkedTo);

            if (mDialogueMap[mLastTopic].mType == ESM::Dialogue::Topic
                    || mDialogueMap[mLastTopic].mType == ESM::Dialogue::Greeting)
            {
                if (const ESM::DialInfo *info = filter.search (mDialogueMap[mLastTopic], true))
                {
                    std::string text = info->mResponse;
                    parseText (text);

                    mChoice = -1;
                    mIsInChoice = false;
                    MWBase::Environment::get().getWindowManager()->getDialogueWindow()->clearChoices();

                    MWScript::InterpreterContext interpreterContext(&mActor.getRefData().getLocals(),mActor);
                    MWBase::Environment::get().getWindowManager()->getDialogueWindow()->addResponse (Interpreter::fixDefinesDialog(text, interpreterContext));

                    // Make sure the returned DialInfo is from the Dialogue we supplied. If could also be from the Info refusal group,
                    // in which case it should not be added to the journal.
                    for (ESM::Dialogue::InfoContainer::const_iterator iter = mDialogueMap[mLastTopic].mInfo.begin();
                        iter!=mDialogueMap[mLastTopic].mInfo.end(); ++iter)
                    {
                        if (iter->mId == info->mId)
                        {
                            MWBase::Environment::get().getJournal()->addTopic (mLastTopic, info->mId, mActor);
                            break;
                        }
                    }

                    executeScript (info->mResultScript);
                }
                else
                {
                    mChoice = -1;
                    mIsInChoice = false;
                    MWBase::Environment::get().getWindowManager()->getDialogueWindow()->clearChoices();
                }
            }
        }

        updateTopics();
    }

    void DialogueManager::askQuestion (const std::string& question, int choice)
    {
        mIsInChoice = true;

        MWGui::DialogueWindow* win = MWBase::Environment::get().getWindowManager()->getDialogueWindow();
        win->addChoice(question, choice);
    }

    void DialogueManager::goodbye()
    {
        mIsInChoice = true;

        MWGui::DialogueWindow* win = MWBase::Environment::get().getWindowManager()->getDialogueWindow();

        win->goodbye();
    }

    void DialogueManager::persuade(int type)
    {
        bool success;
        float temp, perm;
        MWBase::Environment::get().getMechanicsManager()->getPersuasionDispositionChange(
                    mActor, MWBase::MechanicsManager::PersuasionType(type), mTemporaryDispositionChange,
                    success, temp, perm);
        mTemporaryDispositionChange += temp;
        mPermanentDispositionChange += perm;

        // change temp disposition so that final disposition is between 0...100
        float curDisp = static_cast<float>(MWBase::Environment::get().getMechanicsManager()->getDerivedDisposition(mActor));
        if (curDisp + mTemporaryDispositionChange < 0)
            mTemporaryDispositionChange = -curDisp;
        else if (curDisp + mTemporaryDispositionChange > 100)
            mTemporaryDispositionChange = 100 - curDisp;

        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
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

        executeTopic (text + (success ? " Success" : " Fail"));
    }

    int DialogueManager::getTemporaryDispositionChange() const
    {
        return static_cast<int>(mTemporaryDispositionChange);
    }

    void DialogueManager::applyDispositionChange(int delta)
    {
        mTemporaryDispositionChange += delta;
    }

    bool DialogueManager::checkServiceRefused()
    {
        Filter filter (mActor, mChoice, mTalkedTo);

        const MWWorld::Store<ESM::Dialogue> &dialogues =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::Dialogue>();

        const ESM::Dialogue& dialogue = *dialogues.find ("Service Refusal");
        MWGui::DialogueWindow* win = MWBase::Environment::get().getWindowManager()->getDialogueWindow();

        std::vector<const ESM::DialInfo *> infos = filter.list (dialogue, false, false, true);
        if (!infos.empty())
        {
            const ESM::DialInfo* info = infos[0];

            parseText (info->mResponse);

            const MWWorld::Store<ESM::GameSetting>& gmsts =
                MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();

            MWScript::InterpreterContext interpreterContext(&mActor.getRefData().getLocals(),mActor);

            win->addResponse (Interpreter::fixDefinesDialog(info->mResponse, interpreterContext),
                              gmsts.find ("sServiceRefusal")->getString());

            executeScript (info->mResultScript);
            return true;
        }
        return false;
    }

    void DialogueManager::say(const MWWorld::Ptr &actor, const std::string &topic) const
    {
        MWBase::SoundManager *sndMgr = MWBase::Environment::get().getSoundManager();
        if(!sndMgr->sayDone(actor))
        {
            // Actor is already saying something.
            return;
        }

        const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
        const ESM::Dialogue *dial = store.get<ESM::Dialogue>().find(topic);

        Filter filter(actor, 0, false);
        const ESM::DialInfo *info = filter.search(*dial, false);
        if(info != NULL)
        {
            MWBase::WindowManager *winMgr = MWBase::Environment::get().getWindowManager();
            if(winMgr->getSubtitlesEnabled())
                winMgr->messageBox(info->mResponse);
            if (!info->mSound.empty())
                sndMgr->say(actor, info->mSound);
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
                        mLastTopic, actor.getClass().getName(actor));
        }
    }
}
