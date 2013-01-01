
#include "dialoguemanagerimp.hpp"

#include <cctype>
#include <algorithm>
#include <iterator>

#include <components/esm/loaddial.hpp>
#include <components/esm/loadinfo.hpp>

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
#include "../mwbase/scriptmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/player.hpp"

#include "../mwgui/dialogue.hpp"

#include "../mwscript/compilercontext.hpp"
#include "../mwscript/interpretercontext.hpp"
#include "../mwscript/extensions.hpp"

#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/npcstats.hpp"

#include "filter.hpp"

namespace MWDialogue
{
    DialogueManager::DialogueManager (const Compiler::Extensions& extensions, bool scriptVerbose, Translation::Storage& translationDataStorage) :
      mCompilerContext (MWScript::CompilerContext::Type_Dialgoue),
        mErrorStream(std::cout.rdbuf()),mErrorHandler(mErrorStream)
      , mTemporaryDispositionChange(0.f)
      , mPermanentDispositionChange(0.f), mScriptVerbose (scriptVerbose)
      , mTranslationDataStorage(translationDataStorage)
    {
        mChoice = -1;
        mIsInChoice = false;
        mCompilerContext.setExtensions (&extensions);
        mDialogueMap.clear();
        mActorKnownTopics.clear();

        const MWWorld::Store<ESM::Dialogue> &dialogs =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::Dialogue>();

        MWWorld::Store<ESM::Dialogue>::iterator it = dialogs.begin();
        for (; it != dialogs.end(); ++it)
        {
            mDialogueMap[Misc::StringUtils::lowerCase(it->mId)] = *it;
        }
    }

    void DialogueManager::addTopic (const std::string& topic)
    {
        mKnownTopics[Misc::StringUtils::lowerCase(topic)] = true;
    }

    void DialogueManager::parseText (const std::string& text)
    {
        std::vector<HyperTextToken> hypertext = ParseHyperText(text);

        //calculation of standard form fir all hyperlinks
        for (size_t i = 0; i < hypertext.size(); ++i)
        {
            if (hypertext[i].mLink)
            {
                size_t asterisk_count = MWDialogue::RemovePseudoAsterisks(hypertext[i].mText);
                for(; asterisk_count > 0; --asterisk_count)
                    hypertext[i].mText.append("*");

                hypertext[i].mText = mTranslationDataStorage.topicStandardForm(hypertext[i].mText);
            }
        }

        for (size_t i = 0; i < hypertext.size(); ++i)
        {
            std::list<std::string>::iterator it;
            for(it = mActorKnownTopics.begin(); it != mActorKnownTopics.end(); ++it)
            {
                if (hypertext[i].mLink)
                {
                    if( hypertext[i].mText == *it )
                    {
                        mKnownTopics[hypertext[i].mText] = true;
                    }
                }
                else if( !mTranslationDataStorage.hasTranslation() )
                {
                    size_t pos = Misc::StringUtils::lowerCase(hypertext[i].mText).find(*it, 0);
                    if(pos !=std::string::npos)
                    {
                        mKnownTopics[*it] = true;
                    }
                }
            }
        }

        updateTopics();
    }

    void DialogueManager::startDialogue (const MWWorld::Ptr& actor)
    {
        mChoice = -1;
        mIsInChoice = false;

        mActor = actor;

        MWMechanics::CreatureStats& creatureStats = MWWorld::Class::get (actor).getCreatureStats (actor);
        mTalkedTo = creatureStats.hasTalkedToPlayer();
        creatureStats.talkedToPlayer();

        mActorKnownTopics.clear();

        //initialise the GUI
        MWBase::Environment::get().getWindowManager()->pushGuiMode(MWGui::GM_Dialogue);
        MWGui::DialogueWindow* win = MWBase::Environment::get().getWindowManager()->getDialogueWindow();
        win->startDialogue(actor, MWWorld::Class::get (actor).getName (actor));

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
                if (const ESM::DialInfo *info = filter.search (*it))
                {
                    if (!info->mSound.empty())
                    {
                        // TODO play sound
                    }

                    parseText (info->mResponse);

                    MWScript::InterpreterContext interpreterContext(&mActor.getRefData().getLocals(),mActor);
                    win->addText (Interpreter::fixDefinesDialog(info->mResponse, interpreterContext));
                    executeScript (info->mResultScript);
                    mLastTopic = it->mId;
                    mLastDialogue = *info;
                    break;
                }
            }
        }
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

            std::string actorScript = MWWorld::Class::get (mActor).getScript (mActor);

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

        if (!success && mScriptVerbose)
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
                std::cerr << std::string ("Dialogue error: An exception has been thrown: ") + error.what();
            }
        }
    }

    void DialogueManager::executeTopic (const std::string& topic)
    {
        Filter filter (mActor, mChoice, mTalkedTo);

        const MWWorld::Store<ESM::Dialogue> &dialogues =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::Dialogue>();

        const ESM::Dialogue& dialogue = *dialogues.find (topic);

        if (const ESM::DialInfo *info = filter.search (dialogue))
        {
            parseText (info->mResponse);

            MWGui::DialogueWindow* win = MWBase::Environment::get().getWindowManager()->getDialogueWindow();

            if (dialogue.mType==ESM::Dialogue::Persuasion)
            {
                std::string modifiedTopic = "s" + topic;

                modifiedTopic.erase (std::remove (modifiedTopic.begin(), modifiedTopic.end(), ' '), modifiedTopic.end());

                const MWWorld::Store<ESM::GameSetting>& gmsts =
                    MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();

                win->addTitle (gmsts.find (modifiedTopic)->getString());
            }
            else
                win->addTitle (topic);

            MWScript::InterpreterContext interpreterContext(&mActor.getRefData().getLocals(),mActor);
            win->addText (Interpreter::fixDefinesDialog(info->mResponse, interpreterContext));

            executeScript (info->mResultScript);

            mLastTopic = topic;
            mLastDialogue = *info;
        }
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
                if (filter.search (*iter))
                {
                    std::string lower = Misc::StringUtils::lowerCase(iter->mId);
                    mActorKnownTopics.push_back (lower);

                    //does the player know the topic?
                    if (mKnownTopics.find (lower) != mKnownTopics.end())
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

        if(mActor.getTypeName() == typeid(ESM::NPC).name() && !mActor.get<ESM::NPC>()->mBase->mTransport.empty())
            windowServices |= MWGui::DialogueWindow::Service_Travel;

        if (services & ESM::NPC::Spells)
            windowServices |= MWGui::DialogueWindow::Service_BuySpells;

        if (services & ESM::NPC::Spellmaking)
            windowServices |= MWGui::DialogueWindow::Service_CreateSpells;

        if (services & ESM::NPC::Training)
            windowServices |= MWGui::DialogueWindow::Service_Training;

        if (services & ESM::NPC::Enchanting)
            windowServices |= MWGui::DialogueWindow::Service_Enchant;

        MWGui::DialogueWindow* win = MWBase::Environment::get().getWindowManager()->getDialogueWindow();

        win->setServices (windowServices);

        // sort again, because the previous sort was case-sensitive
        keywordList.sort(Misc::StringUtils::ciEqual);
        win->setKeywords(keywordList);

        mChoice = choice;
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

    void DialogueManager::goodbyeSelected()
    {
        MWBase::Environment::get().getWindowManager()->removeGuiMode(MWGui::GM_Dialogue);

        // Apply disposition change to NPC's base disposition
        if (mActor.getTypeName() == typeid(ESM::NPC).name())
        {
            MWMechanics::NpcStats& npcStats = MWWorld::Class::get(mActor).getNpcStats(mActor);
            npcStats.setBaseDisposition(npcStats.getBaseDisposition() + mPermanentDispositionChange);
        }
        mPermanentDispositionChange = 0;
        mTemporaryDispositionChange = 0;
    }

    void DialogueManager::questionAnswered (const std::string& answer)
    {
        if (mChoiceMap.find(answer) != mChoiceMap.end())
        {
            mChoice = mChoiceMap[answer];

            if (mDialogueMap.find(mLastTopic) != mDialogueMap.end())
            {
                if (mDialogueMap[mLastTopic].mType == ESM::Dialogue::Topic)
                {
                    Filter filter (mActor, mChoice, mTalkedTo);

                    if (const ESM::DialInfo *info = filter.search (mDialogueMap[mLastTopic]))
                    {
                        mChoiceMap.clear();
                        mChoice = -1;
                        mIsInChoice = false;
                        std::string text = info->mResponse;
                        parseText (text);

                        MWScript::InterpreterContext interpreterContext(&mActor.getRefData().getLocals(),mActor);
                        MWBase::Environment::get().getWindowManager()->getDialogueWindow()->addText (Interpreter::fixDefinesDialog(text, interpreterContext));
                        executeScript (info->mResultScript);
                        mLastTopic = mLastTopic;
                        mLastDialogue = *info;
                    }
                }
            }

            updateTopics();
        }
    }

    void DialogueManager::printError (const std::string& error)
    {
        MWGui::DialogueWindow* win = MWBase::Environment::get().getWindowManager()->getDialogueWindow();
        win->addText(error);
    }

    void DialogueManager::askQuestion (const std::string& question, int choice)
    {
        MWGui::DialogueWindow* win = MWBase::Environment::get().getWindowManager()->getDialogueWindow();
        win->askQuestion(question);
        mChoiceMap[Misc::StringUtils::lowerCase(question)] = choice;
        mIsInChoice = true;
    }

    MWWorld::Ptr DialogueManager::getActor() const
    {
        return mActor;
    }

    void DialogueManager::goodbye()
    {
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
        int curDisp = MWBase::Environment::get().getMechanicsManager()->getDerivedDisposition(mActor);
        if (curDisp + mTemporaryDispositionChange < 0)
            mTemporaryDispositionChange = -curDisp;
        else if (curDisp + mTemporaryDispositionChange > 100)
            mTemporaryDispositionChange = 100 - curDisp;

        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();
        MWWorld::Class::get(player).skillUsageSucceeded(player, ESM::Skill::Speechcraft, success ? 0 : 1);

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
        return mTemporaryDispositionChange;
    }

    void DialogueManager::applyTemporaryDispositionChange(int delta)
    {
        mTemporaryDispositionChange += delta;
    }

    std::vector<HyperTextToken> ParseHyperText(const std::string& text)
    {
        std::vector<HyperTextToken> result;

        MyGUI::UString utext(text);

        size_t pos_begin, pos_end, iteration_pos = 0;
        for(;;)
        {
            pos_begin = utext.find('@', iteration_pos);
            if (pos_begin != std::string::npos)
                pos_end = utext.find('#', pos_begin);

            if (pos_begin != std::string::npos && pos_end != std::string::npos)
            {
                result.push_back( HyperTextToken(utext.substr(iteration_pos, pos_begin - iteration_pos), false) );

                std::string link = utext.substr(pos_begin + 1, pos_end - pos_begin - 1);
                result.push_back( HyperTextToken(link, true) );

                iteration_pos = pos_end + 1;
            }
            else
            {
                result.push_back( HyperTextToken(utext.substr(iteration_pos), false) );
                break;
            }
        }

        return result;
    }

    size_t RemovePseudoAsterisks(std::string& phrase)
    {
        size_t pseudoAsterisksCount = 0;
        const char specialPseudoAsteriskCharacter = 127;

        if( !phrase.empty() )
        {
            std::string::reverse_iterator rit = phrase.rbegin();

            while( rit != phrase.rend() && *rit == specialPseudoAsteriskCharacter )
            {
                pseudoAsterisksCount++;
                ++rit;
            }
        }

        phrase = phrase.substr(0, phrase.length() - pseudoAsterisksCount);

        return pseudoAsterisksCount;
    }
}
