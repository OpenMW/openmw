
#include "dialoguemanager.hpp"

#include <cctype>
#include <algorithm>
#include <iterator>

#include <components/esm/loaddial.hpp>

#include <components/esm_store/store.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/refdata.hpp"
#include "../mwworld/player.hpp"
#include "../mwworld/containerstore.hpp"

#include "../mwinput/inputmanager.hpp"
#include "../mwgui/dialogue.hpp"
#include "../mwgui/window_manager.hpp"

#include "journal.hpp"

#include <iostream>

#include "../mwscript/extensions.hpp"
#include "../mwscript/scriptmanager.hpp"

#include <components/compiler/exception.hpp>
#include <components/compiler/errorhandler.hpp>
#include <components/compiler/scanner.hpp>
#include <components/compiler/locals.hpp>
#include <components/compiler/output.hpp>
#include <components/interpreter/interpreter.hpp>

#include "../mwscript/compilercontext.hpp"
#include "../mwscript/interpretercontext.hpp"
#include <components/compiler/scriptparser.hpp>

#include "../mwclass/npc.hpp"
#include "../mwmechanics/npcstats.hpp"

namespace
{
    std::string toLower (const std::string& name)
    {
        std::string lowerCase;

        std::transform (name.begin(), name.end(), std::back_inserter (lowerCase),
            (int(*)(int)) std::tolower);

        return lowerCase;
    }

    bool stringCompareNoCase (std::string first, std::string second)
    {
        unsigned int i=0;
        while ( (i<first.length()) && (i<second.length()) )
        {
            if (tolower(first[i])<tolower(second[i])) return true;
            else if (tolower(first[i])>tolower(second[i])) return false;
            ++i;
        }
        if (first.length()<second.length())
            return true;
        else
            return false;
    }

    template<typename T1, typename T2>
    bool selectCompare (char comp, T1 value1, T2 value2)
    {
        switch (comp)
        {
        case '0': return value1==value2;
        case '1': return value1!=value2;
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

        for (; i<static_cast<int> (script->varNames.size()); ++i)
            if (script->varNames[i]==name)
                break;

        if (i>=static_cast<int> (script->varNames.size()))
            return false; // script does not have a variable of this name

        const MWScript::Locals& locals = actor.getRefData().getLocals();

        if (i<script->data.numShorts)
            return selectCompare (comp, locals.mShorts[i], value);
        else
            i -= script->data.numShorts;

        if (i<script->data.numLongs)
            return selectCompare (comp, locals.mLongs[i], value);
        else
            i -= script->data.numShorts;

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
}

namespace MWDialogue
{

    //helper function
    std::string::size_type find_str_ci(const std::string& str, const std::string& substr,size_t pos)
    {
        return toLower(str).find(toLower(substr),pos);
    }

    bool DialogueManager::functionFilter(const MWWorld::Ptr& actor, const ESM::DialInfo& info,bool choice)
    {
        bool isCreature = (actor.getTypeName() != typeid(ESM::NPC).name());

        for (std::vector<ESM::DialInfo::SelectStruct>::const_iterator iter (info.selects.begin());
            iter != info.selects.end(); ++iter)
        {
            ESM::DialInfo::SelectStruct select = *iter;
            char type = select.selectRule[1];
            if(type == '1')
            {
                char comp = select.selectRule[4];
                std::string name = select.selectRule.substr (5);
                std::string function = select.selectRule.substr(2,2);

                int ifunction;
                std::istringstream iss(function);
                iss >> ifunction;
                switch(ifunction)
                {
                case 39://PC Expelled
                    if(!selectCompare<int,int>(comp,0,select.i)) return false;
                    break;

                case 40://PC Common Disease
                    if(!selectCompare<int,int>(comp,0,select.i)) return false;
                    break;

                case 41://PC Blight Disease
                    if(!selectCompare<int,int>(comp,0,select.i)) return false;
                    break;

                case 43://PC Crime level
                    if(!selectCompare<int,int>(comp,0,select.i)) return false;
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
                        if(PCstats.getFactionRanks().find(toLower(NPCFaction)) != PCstats.getFactionRanks().end()) sameFaction = 1;
                    }
                    if(!selectCompare<int,int>(comp,sameFaction,select.i)) return false;
                    }
                    break;

                case 48://Detected
                    if(!selectCompare<int,int>(comp,1,select.i)) return false;
                    break;

                case 49://Alarmed
                    if(!selectCompare<int,int>(comp,0,select.i)) return false;
                    break;

                case 50://choice
                    if(choice)
                    {
                        if(!selectCompare<int,int>(comp,mChoice,select.i)) return false;
                    }
                    break;

                case 60://PC Vampire
                    if(!selectCompare<int,int>(comp,0,select.i)) return false;
                    break;

                case 61://Level
                    if(!selectCompare<int,int>(comp,1,select.i)) return false;
                    break;

                case 62://Attacked
                    if(!selectCompare<int,int>(comp,0,select.i)) return false;
                    break;

                case 63://Talked to PC
                    if(!selectCompare<int,int>(comp,0,select.i)) return false;
                    break;

                case 64://PC Health
                    if(!selectCompare<int,int>(comp,50,select.i)) return false;
                    break;

                case 65://Creature target
                    if(!selectCompare<int,int>(comp,0,select.i)) return false;
                    break;

                case 66://Friend hit
                    if(!selectCompare<int,int>(comp,0,select.i)) return false;
                    break;

                case 67://Fight
                    if(!selectCompare<int,int>(comp,0,select.i)) return false;
                    break;

                case 68://Hello????
                    if(!selectCompare<int,int>(comp,0,select.i)) return false;
                    break;

                case 69://Alarm
                    if(!selectCompare<int,int>(comp,0,select.i)) return false;
                    break;

                case 70://Flee
                    if(!selectCompare<int,int>(comp,0,select.i)) return false;
                    break;

                case 71://Should Attack
                    if(!selectCompare<int,int>(comp,0,select.i)) return false;
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

        char type = select.selectRule[1];

        if (type!='0')
        {
            char comp = select.selectRule[4];
            std::string name = select.selectRule.substr (5);
            std::string function = select.selectRule.substr(1,2);

            switch (type)
            {
            case '1': // function

                return true; // Done elsewhere.

            case '2': // global

                if (select.type==ESM::VT_Short || select.type==ESM::VT_Int ||
                    select.type==ESM::VT_Long)
                {
                    if (!checkGlobal (comp, toLower (name), select.i))
                        return false;
                }
                else if (select.type==ESM::VT_Float)
                {
                    if (!checkGlobal (comp, toLower (name), select.f))
                        return false;
                }
                else
                    throw std::runtime_error (
                    "unsupported variable type in dialogue info select");

                return true;

            case '3': // local

                if (select.type==ESM::VT_Short || select.type==ESM::VT_Int ||
                    select.type==ESM::VT_Long)
                {
                    if (!checkLocal (comp, toLower (name), select.i, actor,
                        MWBase::Environment::get().getWorld()->getStore()))
                        return false;
                }
                else if (select.type==ESM::VT_Float)
                {
                    if (!checkLocal (comp, toLower (name), select.f, actor,
                        MWBase::Environment::get().getWorld()->getStore()))
                        return false;
                }
                else
                    throw std::runtime_error (
                    "unsupported variable type in dialogue info select");

                return true;

            case '4'://journal
                if(select.type==ESM::VT_Int)
                {
                    if(!selectCompare<int,int>(comp,MWBase::Environment::get().getJournal()->getJournalIndex(toLower(name)),select.i)) return false;
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
                    if (toLower(iter->getCellRef().refID) == toLower(name))
                        sum += iter->getRefData().getCount();
                if(!selectCompare<int,int>(comp,sum,select.i)) return false;
                }

                return true;


            case '6'://dead
                if(!selectCompare<int,int>(comp,0,select.i)) return false;

            case '7':// not ID
                if(select.type==ESM::VT_String ||select.type==ESM::VT_Int)//bug in morrowind here? it's not a short, it's a string
                {
                    int isID = int(toLower(name)==toLower(MWWorld::Class::get (actor).getId (actor)));
                    if (selectCompare<int,int>(comp,!isID,select.i)) return false;
                }
                else
                    throw std::runtime_error (
                    "unsupported variable type in dialogue info select");

                return true;

            case '8':// not faction
                if (isCreature)
                    return false;

                if(select.type==ESM::VT_Int)
                {
                    MWWorld::LiveCellRef<ESM::NPC>* npc = actor.get<ESM::NPC>();
                    int isFaction = int(toLower(npc->base->faction) == toLower(name));
                    if(selectCompare<int,int>(comp,!isFaction,select.i))
                        return false;
                }
                else
                    throw std::runtime_error (
                    "unsupported variable type in dialogue info select");

                return true;

            case '9':// not class
                if (isCreature)
                    return false;

                if(select.type==ESM::VT_Int)
                {
                    MWWorld::LiveCellRef<ESM::NPC>* npc = actor.get<ESM::NPC>();
                    int isClass = int(toLower(npc->base->cls) == toLower(name));
                    if(selectCompare<int,int>(comp,!isClass,select.i))
                        return false;
                }
                else
                    throw std::runtime_error (
                    "unsupported variable type in dialogue info select");

                return true;

            case 'A'://not Race
                if (isCreature)
                    return false;

                if(select.type==ESM::VT_Int)
                {
                    MWWorld::LiveCellRef<ESM::NPC>* npc = actor.get<ESM::NPC>();
                    int isRace = int(toLower(npc->base->race) == toLower(name));
                    if(selectCompare<int,int>(comp,!isRace,select.i))
                        return false;
                }
                else
                    throw std::runtime_error (
                    "unsupported variable type in dialogue info select");

                return true;

            case 'B'://not Cell
                if(select.type==ESM::VT_Int)
                {
                    int isCell = int(toLower(actor.getCell()->cell->name) == toLower(name));
                    if(selectCompare<int,int>(comp,!isCell,select.i))
                        return false;
                }
                else
                    throw std::runtime_error (
                    "unsupported variable type in dialogue info select");
                return true;

            case 'C'://not local
                if (select.type==ESM::VT_Short || select.type==ESM::VT_Int ||
                    select.type==ESM::VT_Long)
                {
                    if (checkLocal (comp, toLower (name), select.i, actor,
                        MWBase::Environment::get().getWorld()->getStore()))
                        return false;
                }
                else if (select.type==ESM::VT_Float)
                {
                    if (checkLocal (comp, toLower (name), select.f, actor,
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
        if (!info.actor.empty())
            if (toLower (info.actor)!=MWWorld::Class::get (actor).getId (actor))
                return false;

        //NPC race
        if (!info.race.empty())
        {
            if (isCreature)
                return false;

            MWWorld::LiveCellRef<ESM::NPC> *cellRef = actor.get<ESM::NPC>();

            if (!cellRef)
                return false;

            if (toLower (info.race)!=toLower (cellRef->base->race))
                return false;
        }

        //NPC class
        if (!info.clas.empty())
        {
            if (isCreature)
                return false;

            MWWorld::LiveCellRef<ESM::NPC> *cellRef = actor.get<ESM::NPC>();

            if (!cellRef)
                return false;

            if (toLower (info.clas)!=toLower (cellRef->base->cls))
                return false;
        }

        //NPC faction
        if (!info.npcFaction.empty())
        {
            if (isCreature)
                return false;

            //MWWorld::Class npcClass = MWWorld::Class::get(actor);
            MWMechanics::NpcStats stats = MWWorld::Class::get(actor).getNpcStats(actor);
            std::map<std::string,int>::iterator it = stats.getFactionRanks().find(toLower(info.npcFaction));
            if(it!=stats.getFactionRanks().end())
            {
                //check rank
                if(it->second < (int)info.data.rank) return false;
            }
            else
            {
                //not in the faction
                return false;
            }
        }

        // TODO check player faction
        if(!info.pcFaction.empty())
        {
            MWMechanics::NpcStats stats = MWWorld::Class::get(MWBase::Environment::get().getWorld()->getPlayer().getPlayer()).getNpcStats(MWBase::Environment::get().getWorld()->getPlayer().getPlayer());
            std::map<std::string,int>::iterator it = stats.getFactionRanks().find(toLower(info.pcFaction));
            if(it!=stats.getFactionRanks().end())
            {
                //check rank
                if(it->second < (int)info.data.PCrank) return false;
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
            if(npc->base->flags&npc->base->Female)
            {
                if(static_cast<int> (info.data.gender)==0)  return false;
            }
            else
            {
                if(static_cast<int> (info.data.gender)==1)  return false;
            }
        }

        // check cell
        if (!info.cell.empty())
            if (MWBase::Environment::get().getWorld()->getPlayer().getPlayer().getCell()->cell->name != info.cell)
                return false;

        // TODO check DATAstruct
        for (std::vector<ESM::DialInfo::SelectStruct>::const_iterator iter (info.selects.begin());
            iter != info.selects.end(); ++iter)
            if (!isMatching (actor, *iter))
                return false;

        return true;
    }

    DialogueManager::DialogueManager (const Compiler::Extensions& extensions) :
      mCompilerContext (MWScript::CompilerContext::Type_Dialgoue),
        mErrorStream(std::cout.rdbuf()),mErrorHandler(mErrorStream)
    {
        mChoice = -1;
        mIsInChoice = false;
        mCompilerContext.setExtensions (&extensions);
        mDialogueMap.clear();
        mActorKnownTopics.clear();
        ESMS::RecListCaseT<ESM::Dialogue>::MapType dialogueList = MWBase::Environment::get().getWorld()->getStore().dialogs.list;
        for(ESMS::RecListCaseT<ESM::Dialogue>::MapType::iterator it = dialogueList.begin(); it!=dialogueList.end();it++)
        {
            mDialogueMap[toLower(it->first)] = it->second;
        }
    }

    void DialogueManager::addTopic(std::string topic)
    {
        mKnownTopics[toLower(topic)] = true;
    }

    void DialogueManager::parseText(std::string text)
    {
        std::list<std::string>::iterator it;
        for(it = mActorKnownTopics.begin();it != mActorKnownTopics.end();++it)
        {
            size_t pos = find_str_ci(text,*it,0);
            if(pos !=std::string::npos)
            {
                if(pos==0)
                {
                    mKnownTopics[*it] = true;
                }
                else if(text.substr(pos -1,1) == " ")
                {
                    mKnownTopics[*it] = true;
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

        mActorKnownTopics.clear();

        //initialise the GUI
        MWBase::Environment::get().getWindowManager()->pushGuiMode(MWGui::GM_Dialogue);
        MWGui::DialogueWindow* win = MWBase::Environment::get().getWindowManager()->getDialogueWindow();
        win->startDialogue(actor, MWWorld::Class::get (actor).getName (actor));

        //setup the list of topics known by the actor. Topics who are also on the knownTopics list will be added to the GUI
        updateTopics();

        //greeting
        bool greetingFound = false;
        //ESMS::RecListT<ESM::Dialogue>::MapType dialogueList = MWBase::Environment::get().getWorld()->getStore().dialogs.list;
        ESMS::RecListCaseT<ESM::Dialogue>::MapType dialogueList = MWBase::Environment::get().getWorld()->getStore().dialogs.list;
        for(ESMS::RecListCaseT<ESM::Dialogue>::MapType::iterator it = dialogueList.begin(); it!=dialogueList.end();it++)
        {
            ESM::Dialogue ndialogue = it->second;
            if(ndialogue.type == ESM::Dialogue::Greeting)
            {
                if (greetingFound) break;
                for (std::vector<ESM::DialInfo>::const_iterator iter (it->second.mInfo.begin());
                    iter!=it->second.mInfo.end(); ++iter)
                {
                    if (isMatching (actor, *iter) && functionFilter(mActor,*iter,true))
                    {
                        if (!iter->sound.empty())
                        {
                            // TODO play sound
                        }

                        std::string text = iter->response;
                        parseText(text);
                        win->addText(iter->response);
                        executeScript(iter->resultScript);
                        greetingFound = true;
                        mLastTopic = it->first;
                        mLastDialogue = *iter;
                        break;
                    }
                }
            }
        }
    }

    bool DialogueManager::compile (const std::string& cmd,std::vector<Interpreter::Type_Code>& code)
    {
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
            if(mErrorHandler.isGood())
            {
                parser.getCode(code);
                return true;
            }
            return false;
        }
        catch (const Compiler::SourceException& /* error */)
        {
            // error has already been reported via error handler
        }
        catch (const std::exception& error)
        {
            printError (std::string ("An exception has been thrown: ") + error.what());
        }

        return false;
    }

    void DialogueManager::executeScript(std::string script)
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
                printError (std::string ("An exception has been thrown: ") + error.what());
            }
        }
    }

    void DialogueManager::updateTopics()
    {
        std::list<std::string> keywordList;
        int choice = mChoice;
        mChoice = -1;
        mActorKnownTopics.clear();
        MWGui::DialogueWindow* win = MWBase::Environment::get().getWindowManager()->getDialogueWindow();
        ESMS::RecListCaseT<ESM::Dialogue>::MapType dialogueList = MWBase::Environment::get().getWorld()->getStore().dialogs.list;
        for(ESMS::RecListCaseT<ESM::Dialogue>::MapType::iterator it = dialogueList.begin(); it!=dialogueList.end();it++)
        {
            ESM::Dialogue ndialogue = it->second;
            if(ndialogue.type == ESM::Dialogue::Topic)
            {
                for (std::vector<ESM::DialInfo>::const_iterator iter (it->second.mInfo.begin());
                    iter!=it->second.mInfo.end(); ++iter)
                {
                    if (isMatching (mActor, *iter) && functionFilter(mActor,*iter,true))
                    {
                        mActorKnownTopics.push_back(toLower(it->first));
                        //does the player know the topic?
                        if(mKnownTopics.find(toLower(it->first)) != mKnownTopics.end())
                        {
                            keywordList.push_back(it->first);
                            break;
                        }
                    }
                }
            }
        }

        // check the available services of this actor
        int services = 0;
        if (mActor.getTypeName() == typeid(ESM::NPC).name())
        {
            MWWorld::LiveCellRef<ESM::NPC>* ref = mActor.get<ESM::NPC>();
            if (ref->base->hasAI)
                services = ref->base->AI.services;
        }
        else if (mActor.getTypeName() == typeid(ESM::Creature).name())
        {
            MWWorld::LiveCellRef<ESM::Creature>* ref = mActor.get<ESM::Creature>();
            if (ref->base->hasAI)
                services = ref->base->AI.services;
        }

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
            win->setShowTrade(true);
        else
            win->setShowTrade(false);

        // sort again, because the previous sort was case-sensitive
        keywordList.sort(stringCompareNoCase);
        win->setKeywords(keywordList);

        mChoice = choice;
    }

    void DialogueManager::keywordSelected(std::string keyword)
    {
        if(!mIsInChoice)
        {
            if(mDialogueMap.find(keyword) != mDialogueMap.end())
            {
                ESM::Dialogue ndialogue = mDialogueMap[keyword];
                if(ndialogue.type == ESM::Dialogue::Topic)
                {
                    for (std::vector<ESM::DialInfo>::const_iterator iter  = ndialogue.mInfo.begin();
                        iter!=ndialogue.mInfo.end(); ++iter)
                    {
                        if (isMatching (mActor, *iter) && functionFilter(mActor,*iter,true))
                        {
                            std::string text = iter->response;
                            std::string script = iter->resultScript;

                            parseText(text);

                            MWGui::DialogueWindow* win = MWBase::Environment::get().getWindowManager()->getDialogueWindow();
                            win->addTitle(keyword);
                            win->addText(iter->response);

                            executeScript(script);

                            mLastTopic = keyword;
                            mLastDialogue = *iter;
                            break;
                        }
                    }
                }
            }
        }

        updateTopics();
    }

    void DialogueManager::goodbyeSelected()
    {
        MWBase::Environment::get().getWindowManager()->removeGuiMode(MWGui::GM_Dialogue);
    }

    void DialogueManager::questionAnswered(std::string answere)
    {
        if(mChoiceMap.find(answere) != mChoiceMap.end())
        {
            mChoice = mChoiceMap[answere];

            std::vector<ESM::DialInfo>::const_iterator iter;
            if(mDialogueMap.find(mLastTopic) != mDialogueMap.end())
            {
                ESM::Dialogue ndialogue = mDialogueMap[mLastTopic];
                if(ndialogue.type == ESM::Dialogue::Topic)
                {
                    for (std::vector<ESM::DialInfo>::const_iterator iter = ndialogue.mInfo.begin();
                        iter!=ndialogue.mInfo.end(); ++iter)
                    {
                        if (isMatching (mActor, *iter) && functionFilter(mActor,*iter,true))
                        {
                            mChoiceMap.clear();
                            mChoice = -1;
                            mIsInChoice = false;
                            MWGui::DialogueWindow* win = MWBase::Environment::get().getWindowManager()->getDialogueWindow();
                            std::string text = iter->response;
                            parseText(text);
                            win->addText(text);
                            executeScript(iter->resultScript);
                            mLastTopic = mLastTopic;
                            mLastDialogue = *iter;
                            break;
                        }
                    }
                }
            }
            updateTopics();
        }
    }

    void DialogueManager::printError(std::string error)
    {
        MWGui::DialogueWindow* win = MWBase::Environment::get().getWindowManager()->getDialogueWindow();
        win->addText(error);
    }

    void DialogueManager::askQuestion(std::string question, int choice)
    {
        MWGui::DialogueWindow* win = MWBase::Environment::get().getWindowManager()->getDialogueWindow();
        win->askQuestion(question);
        mChoiceMap[toLower(question)] = choice;
        mIsInChoice = true;
    }

    std::string DialogueManager::getFaction()
    {
        if (mActor.getTypeName() != typeid(ESM::NPC).name())
            return "";

        std::string factionID("");
        MWMechanics::NpcStats stats = MWWorld::Class::get(mActor).getNpcStats(mActor);
        if(stats.getFactionRanks().empty())
        {
            std::cout << "No faction for this actor!";
        }
        else
        {
            factionID = stats.getFactionRanks().begin()->first;
        }
        return factionID;
    }

    void DialogueManager::goodbye()
    {
        MWGui::DialogueWindow* win = MWBase::Environment::get().getWindowManager()->getDialogueWindow();

        win->goodbye();
    }
}
