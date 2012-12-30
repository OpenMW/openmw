
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

namespace
{

<<<<<<< HEAD
    template<typename T1, typename T2>
    bool selectCompare (char comp, T1 value1, T2 value2)
    {
        switch (comp)
        {
        case '0': return value1==value2;
//         case '1': return value1!=value2;
        case '2': return value1>value2;
        case '3': return value1>=value2;
        case '4': return value1<value2;
        case '5': return value1<=value2;
        }

        throw std::runtime_error ("unknown compare type in dialogue info select");
    }

    template<typename T>
    bool checkLocal (char comp, const std::string& name, T value, const MWWorld::Ptr& actor,
        const ESMS::ESMStore& store)
    {
        std::string scriptName = MWWorld::Class::get (actor).getScript (actor);

        if (scriptName.empty())
            return false; // no script

        const ESM::Script *script = store.scripts.find (scriptName);

        int i = 0;

        for (; i<static_cast<int> (script->mVarNames.size()); ++i)
            if (script->mVarNames[i]==name)
                break;

        if (i>=static_cast<int> (script->mVarNames.size()))
            return false; // script does not have a variable of this name

        const MWScript::Locals& locals = actor.getRefData().getLocals();

        if (i<script->mData.mNumShorts)
            return selectCompare (comp, locals.mShorts[i], value);
        else
            i -= script->mData.mNumShorts;

        if (i<script->mData.mNumLongs)
            return selectCompare (comp, locals.mLongs[i], value);
        else
            i -= script->mData.mNumShorts;

        return selectCompare (comp, locals.mFloats.at (i), value);
    }

    template<typename T>
    bool checkGlobal (char comp, const std::string& name, T value)
    {
        switch (MWBase::Environment::get().getWorld()->getGlobalVariableType (name))
        {
        case 's':
            return selectCompare (comp, MWBase::Environment::get().getWorld()->getGlobalVariable (name).mShort, value);

        case 'l':

            return selectCompare (comp, MWBase::Environment::get().getWorld()->getGlobalVariable (name).mLong, value);

        case 'f':

            return selectCompare (comp, MWBase::Environment::get().getWorld()->getGlobalVariable (name).mFloat, value);

        case ' ':

            MWBase::Environment::get().getWorld()->getGlobalVariable (name); // trigger exception
            break;

        default:

            throw std::runtime_error ("unsupported gobal variable type");
        }

        return false;
    }

=======
>>>>>>> 92623921add0d6e16a34973dcf6f2ee1f52dbbe7
    //helper function
    std::string::size_type find_str_ci(const std::string& str, const std::string& substr,size_t pos)
    {
        return Misc::toLower(str).find(Misc::toLower(substr),pos);
    }
}

namespace MWDialogue
{
<<<<<<< HEAD


    bool DialogueManager::functionFilter(const MWWorld::Ptr& actor, const ESM::DialInfo& info,bool choice)
    {
        bool isCreature = (actor.getTypeName() != typeid(ESM::NPC).name());

        for (std::vector<ESM::DialInfo::SelectStruct>::const_iterator iter (info.mSelects.begin());
            iter != info.mSelects.end(); ++iter)
        {
            ESM::DialInfo::SelectStruct select = *iter;
            char type = select.mSelectRule[1];
            if(type == '1')
            {
                char comp = select.mSelectRule[4];
                std::string name = select.mSelectRule.substr (5);
                std::string function = select.mSelectRule.substr(2,2);

                int ifunction;
                std::istringstream iss(function);
                iss >> ifunction;
                switch(ifunction)
                {
                case 39://PC Expelled
                    if(!selectCompare<int,int>(comp,0,select.mI)) return false;
                    break;

                case 40://PC Common Disease
                    if(!selectCompare<int,int>(comp,0,select.mI)) return false;
                    break;

                case 41://PC Blight Disease
                    if(!selectCompare<int,int>(comp,0,select.mI)) return false;
                    break;

                case 43://PC Crime level
                    if(!selectCompare<int,int>(comp,0,select.mI)) return false;
                    break;

                case 46://Same faction
                    {
                    if (isCreature)
                        return false;

                    MWMechanics::NpcStats PCstats = MWWorld::Class::get(MWBase::Environment::get().getWorld()->getPlayer().getPlayer()).getNpcStats(MWBase::Environment::get().getWorld()->getPlayer().getPlayer());
                    MWMechanics::NpcStats NPCstats = MWWorld::Class::get(actor).getNpcStats(actor);
                    int sameFaction = 0;
                    if(!NPCstats.getFactionRanks().empty())
                    {
                        std::string NPCFaction = NPCstats.getFactionRanks().begin()->first;
                        if(PCstats.getFactionRanks().find(Misc::toLower(NPCFaction)) != PCstats.getFactionRanks().end()) sameFaction = 1;
                    }
                    if(!selectCompare<int,int>(comp,sameFaction,select.mI)) return false;
                    }
                    break;

                case 48://Detected
                    if(!selectCompare<int,int>(comp,1,select.mI)) return false;
                    break;

                case 49://Alarmed
                    if(!selectCompare<int,int>(comp,0,select.mI)) return false;
                    break;

                case 50://choice
                    if(choice)
                    {
                        if(!selectCompare<int,int>(comp,mChoice,select.mI)) return false;
                    }
                    break;

                case 60://PC Vampire
                    if(!selectCompare<int,int>(comp,0,select.mI)) return false;
                    break;

                case 61://Level
                    if(!selectCompare<int,int>(comp,1,select.mI)) return false;
                    break;

                case 62://Attacked
                    if(!selectCompare<int,int>(comp,0,select.mI)) return false;
                    break;

                case 63://Talked to PC
                    if(!selectCompare<int,int>(comp,0,select.mI)) return false;
                    break;

                case 64://PC Health
                    if(!selectCompare<int,int>(comp,50,select.mI)) return false;
                    break;

                case 65://Creature target
                    if(!selectCompare<int,int>(comp,0,select.mI)) return false;
                    break;

                case 66://Friend hit
                    if(!selectCompare<int,int>(comp,0,select.mI)) return false;
                    break;

                case 67://Fight
                    if(!selectCompare<int,int>(comp,0,select.mI)) return false;
                    break;

                case 68://Hello????
                    if(!selectCompare<int,int>(comp,0,select.mI)) return false;
                    break;

                case 69://Alarm
                    if(!selectCompare<int,int>(comp,0,select.mI)) return false;
                    break;

                case 70://Flee
                    if(!selectCompare<int,int>(comp,0,select.mI)) return false;
                    break;

                case 71://Should Attack
                    if(!selectCompare<int,int>(comp,0,select.mI)) return false;
                    break;

                default:
                    break;

                }
            }
        }

        return true;
    }

    bool DialogueManager::isMatching (const MWWorld::Ptr& actor,
        const ESM::DialInfo::SelectStruct& select) const
    {
        bool isCreature = (actor.getTypeName() != typeid(ESM::NPC).name());

        char type = select.mSelectRule[1];

        if (type!='0')
        {
            char comp = select.mSelectRule[4];
            std::string name = select.mSelectRule.substr (5);
            std::string function = select.mSelectRule.substr(1,2);

            switch (type)
            {
            case '1': // function

                return true; // Done elsewhere.

            case '2': // global

                if (select.mType==ESM::VT_Short || select.mType==ESM::VT_Int ||
                    select.mType==ESM::VT_Long)
                {
                    if (!checkGlobal (comp, Misc::toLower (name), select.mI))
                        return false;
                }
                else if (select.mType==ESM::VT_Float)
                {
                    if (!checkGlobal (comp, Misc::toLower (name), select.mF))
                        return false;
                }
                else
                    throw std::runtime_error (
                    "unsupported variable type in dialogue info select");

                return true;

            case '3': // local

                if (select.mType==ESM::VT_Short || select.mType==ESM::VT_Int ||
                    select.mType==ESM::VT_Long)
                {
                    if (!checkLocal (comp, Misc::toLower (name), select.mI, actor,
                        MWBase::Environment::get().getWorld()->getStore()))
                        return false;
                }
                else if (select.mType==ESM::VT_Float)
                {
                    if (!checkLocal (comp, Misc::toLower (name), select.mF, actor,
                        MWBase::Environment::get().getWorld()->getStore()))
                        return false;
                }
                else
                    throw std::runtime_error (
                    "unsupported variable type in dialogue info select");

                return true;

            case '4'://journal
                if(select.mType==ESM::VT_Int)
                {
                    if(!selectCompare<int,int>(comp,MWBase::Environment::get().getJournal()->getJournalIndex(Misc::toLower(name)),select.mI)) return false;
                }
                else
                    throw std::runtime_error (
                    "unsupported variable type in dialogue info select");

                return true;

            case '5'://item
                {
                MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();
                MWWorld::ContainerStore& store = MWWorld::Class::get (player).getContainerStore (player);

                int sum = 0;

                for (MWWorld::ContainerStoreIterator iter (store.begin()); iter!=store.end(); ++iter)
                    if (Misc::toLower(iter->getCellRef().mRefID) == Misc::toLower(name))
                        sum += iter->getRefData().getCount();
                if(!selectCompare<int,int>(comp,sum,select.mI)) return false;
                }

                return true;


            case '6'://dead
                if(!selectCompare<int,int>(comp,0,select.mI)) return false;

            case '7':// not ID
                if(select.mType==ESM::VT_String ||select.mType==ESM::VT_Int)//bug in morrowind here? it's not a short, it's a string
                {
                    int isID = int(Misc::toLower(name)==Misc::toLower(MWWorld::Class::get (actor).getId (actor)));
                    if (selectCompare<int,int>(comp,!isID,select.mI)) return false;
                }
                else
                    throw std::runtime_error (
                    "unsupported variable type in dialogue info select");

                return true;

            case '8':// not faction
                if (isCreature)
                    return false;

                if(select.mType==ESM::VT_Int)
                {
                    MWWorld::LiveCellRef<ESM::NPC>* npc = actor.get<ESM::NPC>();
                    int isFaction = int(Misc::toLower(npc->base->mFaction) == Misc::toLower(name));
                    if(selectCompare<int,int>(comp,!isFaction,select.mI))
                        return false;
                }
                else
                    throw std::runtime_error (
                    "unsupported variable type in dialogue info select");

                return true;

            case '9':// not class
                if (isCreature)
                    return false;

                if(select.mType==ESM::VT_Int)
                {
                    MWWorld::LiveCellRef<ESM::NPC>* npc = actor.get<ESM::NPC>();
                    int isClass = int(Misc::toLower(npc->base->mClass) == Misc::toLower(name));
                    if(selectCompare<int,int>(comp,!isClass,select.mI))
                        return false;
                }
                else
                    throw std::runtime_error (
                    "unsupported variable type in dialogue info select");

                return true;

            case 'A'://not Race
                if (isCreature)
                    return false;

                if(select.mType==ESM::VT_Int)
                {
                    MWWorld::LiveCellRef<ESM::NPC>* npc = actor.get<ESM::NPC>();
                    int isRace = int(Misc::toLower(npc->base->mRace) == Misc::toLower(name));
                    if(selectCompare<int,int>(comp,!isRace,select.mI))
                        return false;
                }
                else
                    throw std::runtime_error (
                    "unsupported variable type in dialogue info select");

                return true;

            case 'B'://not Cell
                if(select.mType==ESM::VT_Int)
                {
                    int isCell = int(Misc::toLower(actor.getCell()->cell->mName) == Misc::toLower(name));
                    if(selectCompare<int,int>(comp,!isCell,select.mI))
                        return false;
                }
                else
                    throw std::runtime_error (
                    "unsupported variable type in dialogue info select");
                return true;

            case 'C'://not local
                if (select.mType==ESM::VT_Short || select.mType==ESM::VT_Int ||
                    select.mType==ESM::VT_Long)
                {
                    if (checkLocal (comp, Misc::toLower (name), select.mI, actor,
                        MWBase::Environment::get().getWorld()->getStore()))
                        return false;
                }
                else if (select.mType==ESM::VT_Float)
                {
                    if (checkLocal (comp, Misc::toLower (name), select.mF, actor,
                        MWBase::Environment::get().getWorld()->getStore()))
                        return false;
                }
                else
                    throw std::runtime_error (
                    "unsupported variable type in dialogue info select");
                return true;


            default:

                std::cout << "unchecked select: " << type << " " << comp << " " << name << std::endl;
            }
        }

        return true;
    }

    bool DialogueManager::isMatching (const MWWorld::Ptr& actor, const ESM::DialInfo& info) const
    {
        bool isCreature = (actor.getTypeName() != typeid(ESM::NPC).name());

        // actor id
        if (!info.mActor.empty())
            if (Misc::toLower (info.mActor)!=MWWorld::Class::get (actor).getId (actor))
                return false;

        //NPC race
        if (!info.mRace.empty())
        {
            if (isCreature)
                return false;

            MWWorld::LiveCellRef<ESM::NPC> *cellRef = actor.get<ESM::NPC>();

            if (!cellRef)
                return false;

            if (Misc::toLower (info.mRace)!=Misc::toLower (cellRef->base->mRace))
                return false;
        }

        //NPC class
        if (!info.mClass.empty())
        {
            if (isCreature)
                return false;

            MWWorld::LiveCellRef<ESM::NPC> *cellRef = actor.get<ESM::NPC>();

            if (!cellRef)
                return false;

            if (Misc::toLower (info.mClass)!=Misc::toLower (cellRef->base->mClass))
                return false;
        }

        //NPC faction
        if (!info.mNpcFaction.empty())
        {
            if (isCreature)
                return false;

            //MWWorld::Class npcClass = MWWorld::Class::get(actor);
            MWMechanics::NpcStats stats = MWWorld::Class::get(actor).getNpcStats(actor);
            std::map<std::string,int>::iterator it = stats.getFactionRanks().find(Misc::toLower(info.mNpcFaction));
            if(it!=stats.getFactionRanks().end())
            {
                //check rank
                if(it->second < (int)info.mData.mRank) return false;
            }
            else
            {
                //not in the faction
                return false;
            }
        }

        // TODO check player faction
        if(!info.mPcFaction.empty())
        {
            MWMechanics::NpcStats stats = MWWorld::Class::get(MWBase::Environment::get().getWorld()->getPlayer().getPlayer()).getNpcStats(MWBase::Environment::get().getWorld()->getPlayer().getPlayer());
            std::map<std::string,int>::iterator it = stats.getFactionRanks().find(Misc::toLower(info.mPcFaction));
            if(it!=stats.getFactionRanks().end())
            {
                //check rank
                if(it->second < (int)info.mData.mPCrank) return false;
            }
            else
            {
                //not in the faction
                return false;
            }
        }

        //check gender
        if (!isCreature)
        {
            MWWorld::LiveCellRef<ESM::NPC>* npc = actor.get<ESM::NPC>();
            if(npc->base->mFlags & npc->base->Female)
            {
                if(static_cast<int> (info.mData.mGender)==0)  return false;
            }
            else
            {
                if(static_cast<int> (info.mData.mGender)==1)  return false;
            }
        }

        // check cell
        if (!info.mCell.empty())
            if (MWBase::Environment::get().getWorld()->getPlayer().getPlayer().getCell()->cell->mName != info.mCell)
                return false;

        // TODO check DATAstruct
        for (std::vector<ESM::DialInfo::SelectStruct>::const_iterator iter (info.mSelects.begin());
            iter != info.mSelects.end(); ++iter)
            if (!isMatching (actor, *iter))
                return false;

        return true;
    }

    DialogueManager::DialogueManager (const Compiler::Extensions& extensions) :
=======
    DialogueManager::DialogueManager (const Compiler::Extensions& extensions, bool scriptVerbose) :
>>>>>>> 92623921add0d6e16a34973dcf6f2ee1f52dbbe7
      mCompilerContext (MWScript::CompilerContext::Type_Dialgoue),
        mErrorStream(std::cout.rdbuf()),mErrorHandler(mErrorStream)
      , mTemporaryDispositionChange(0.f)
      , mPermanentDispositionChange(0.f), mScriptVerbose (scriptVerbose)
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
<<<<<<< HEAD
            mDialogueMap[Misc::toLower(it->first)] = it->second;
=======
            mDialogueMap[toLower(it->mId)] = *it;
>>>>>>> 92623921add0d6e16a34973dcf6f2ee1f52dbbe7
        }
    }

    void DialogueManager::addTopic (const std::string& topic)
    {
        mKnownTopics[Misc::toLower(topic)] = true;
    }

    void DialogueManager::parseText (const std::string& text)
    {
        std::list<std::string>::iterator it;
        for(it = mActorKnownTopics.begin();it != mActorKnownTopics.end();++it)
        {
            size_t pos = find_str_ci(text,*it,0);
            if(pos !=std::string::npos)
            {
                mKnownTopics[*it] = true;
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
                    mActorKnownTopics.push_back (toLower (iter->mId));

                    //does the player know the topic?
                    if (mKnownTopics.find (toLower (iter->mId)) != mKnownTopics.end())
                    {
<<<<<<< HEAD
                        mActorKnownTopics.push_back(Misc::toLower(it->first));
                        //does the player know the topic?
                        if(mKnownTopics.find(Misc::toLower(it->first)) != mKnownTopics.end())
                        {
                            keywordList.push_back(it->first);
                            break;
                        }
=======
                        keywordList.push_back (iter->mId);
>>>>>>> 92623921add0d6e16a34973dcf6f2ee1f52dbbe7
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
        keywordList.sort(Misc::stringCompareNoCase);
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
        mChoiceMap[Misc::toLower(question)] = choice;
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
}
