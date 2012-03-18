
#include "dialoguemanager.hpp"

#include <cctype>
#include <algorithm>
#include <iterator>

#include <components/esm/loaddial.hpp>

#include <components/esm_store/store.hpp>


#include "../mwworld/class.hpp"
#include "../mwworld/environment.hpp"
#include "../mwworld/world.hpp"
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

namespace
{
    std::string toLower (const std::string& name)
    {
        std::string lowerCase;

        std::transform (name.begin(), name.end(), std::back_inserter (lowerCase),
            (int(*)(int)) std::tolower);

        return lowerCase;
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
    bool checkGlobal (char comp, const std::string& name, T value, MWWorld::World& world)
    {
        switch (world.getGlobalVariableType (name))
        {
        case 's':

            return selectCompare (comp, value, world.getGlobalVariable (name).mShort);

        case 'l':

            return selectCompare (comp, value, world.getGlobalVariable (name).mLong);

        case 'f':

            return selectCompare (comp, value, world.getGlobalVariable (name).mFloat);

        case ' ':

            world.getGlobalVariable (name); // trigger exception
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
        bool isAChoice = false;//is there any choice in the filters?
        bool isFunction = false;
        for (std::vector<ESM::DialInfo::SelectStruct>::const_iterator iter (info.selects.begin());
            iter != info.selects.end(); ++iter)
        {
            ESM::DialInfo::SelectStruct select = *iter;
            char type = select.selectRule[1];
            if(type == '1')
            {
                isFunction = true;
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
                    if(!selectCompare<int,int>(comp,0,select.i)) return false;
                    break;

                case 48://Detected
                    if(!selectCompare<int,int>(comp,1,select.i)) return false;
                    break;

                case 49://Alarmed
                    if(!selectCompare<int,int>(comp,0,select.i)) return false;
                    break;

                case 50://choice
                    isAChoice = true;
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
        char type = select.selectRule[1];

        if (type!='0')
        {
            char comp = select.selectRule[4];
            std::string name = select.selectRule.substr (5);
            std::string function = select.selectRule.substr(1,2);

            // TODO types 4, 5, 6, 7, 8, 9, A, B, C
            //new TOTO: 5,6,9
            switch (type)
            {
            case '1': // function

                return true; // TODO implement functions

            case '2': // global

                if (select.type==ESM::VT_Short || select.type==ESM::VT_Int ||
                    select.type==ESM::VT_Long)
                {
                    if (!checkGlobal (comp, toLower (name), select.i, *mEnvironment.mWorld))
                        return false;
                }
                else if (select.type==ESM::VT_Float)
                {
                    if (!checkGlobal (comp, toLower (name), select.f, *mEnvironment.mWorld))
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
                        mEnvironment.mWorld->getStore()))
                        return false;
                }
                else if (select.type==ESM::VT_Float)
                {
                    if (!checkLocal (comp, toLower (name), select.f, actor,
                        mEnvironment.mWorld->getStore()))
                        return false;
                }
                else
                    throw std::runtime_error (
                    "unsupported variable type in dialogue info select");

                return true;

            case '4'://journal
                if(select.type==ESM::VT_Int)
                {
                    //std::cout << "vtint: " << select.i << std::endl;
                    bool isInJournal;
                    if(mEnvironment.mJournal->begin()!=mEnvironment.mJournal->end())
                    {
                        for(std::deque<MWDialogue::StampedJournalEntry>::const_iterator it = mEnvironment.mJournal->begin();it!=mEnvironment.mJournal->end();it++)
                        {

                            if(it->mTopic == name) isInJournal = true;
                        }
                    }
                    else
                        isInJournal =  false;
                    if(!selectCompare<int,int>(comp,int(isInJournal),select.i)) return false;
                }
                else
                    throw std::runtime_error (
                    "unsupported variable type in dialogue info select");

                return true;

            case '5'://item
                {
                MWWorld::Ptr player = mEnvironment.mWorld->getPlayer().getPlayer();
                MWWorld::ContainerStore& store = MWWorld::Class::get (player).getContainerStore (player);

                int sum = 0;

                for (MWWorld::ContainerStoreIterator iter (store.begin()); iter!=store.end(); ++iter)
                    if (iter->getCellRef().refID==name)
                        sum += iter->getRefData().getCount();
                if(!selectCompare<int,int>(comp,sum,select.i)) return false;
                }

                return true;
            }

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
                if(select.type==ESM::VT_Int)
                {
                    ESMS::LiveCellRef<ESM::NPC, MWWorld::RefData>* npc = actor.get<ESM::NPC>();
                    int isFaction = int(toLower(npc->base->faction) == toLower(name));
                    if(selectCompare<int,int>(comp,!isFaction,select.i))
                        return false;
                }
                else
                    throw std::runtime_error (
                    "unsupported variable type in dialogue info select");

                return true;

            case '9':// not class
                if(select.type==ESM::VT_Int)
                {
                    ESMS::LiveCellRef<ESM::NPC, MWWorld::RefData>* npc = actor.get<ESM::NPC>();
                    int isClass = int(toLower(npc->base->cls) == toLower(name));
                    if(selectCompare<int,int>(comp,!isClass,select.i))
                        return false;
                }
                else
                    throw std::runtime_error (
                    "unsupported variable type in dialogue info select");

                return true;

            case 'A'://not Race
                if(select.type==ESM::VT_Int)
                {
                    ESMS::LiveCellRef<ESM::NPC, MWWorld::RefData>* npc = actor.get<ESM::NPC>();
                    int isRace = int(toLower(npc->base->race) == toLower(name));
                    //std::cout << "isRace"<<isRace; mEnvironment.mWorld
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
                        mEnvironment.mWorld->getStore()))
                        return false;
                }
                else if (select.type==ESM::VT_Float)
                {
                    if (checkLocal (comp, toLower (name), select.f, actor,
                        mEnvironment.mWorld->getStore()))
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
        // actor id
        if (!info.actor.empty())
            if (toLower (info.actor)!=MWWorld::Class::get (actor).getId (actor))
                return false;

        //PC Faction
        if(!info.pcFaction.empty()) return false;

        //NPC race
        if (!info.race.empty())
        {
            ESMS::LiveCellRef<ESM::NPC, MWWorld::RefData> *cellRef = actor.get<ESM::NPC>();

            if (!cellRef)
                return false;

            if (toLower (info.race)!=toLower (cellRef->base->race))
                return false;
        }

        //NPC class
        if (!info.clas.empty())
        {
            ESMS::LiveCellRef<ESM::NPC, MWWorld::RefData> *cellRef = actor.get<ESM::NPC>();

            if (!cellRef)
                return false;

            if (toLower (info.clas)!=toLower (cellRef->base->cls))
                return false;
        }

        //NPC faction
        if (!info.npcFaction.empty())
        {
            ESMS::LiveCellRef<ESM::NPC, MWWorld::RefData> *cellRef = actor.get<ESM::NPC>();

            if (!cellRef)
                return false;

            if (toLower (info.npcFaction)!=toLower (cellRef->base->faction))
                return false;

            //check NPC rank
            if(cellRef->base->npdt52.gold != -10)
            {
                if(cellRef->base->npdt52.rank < info.data.rank) return false;
            }
            else
            {
                if(cellRef->base->npdt12.rank < info.data.rank) return false;
            }
        }

        // TODO check player faction

        //check gender
        ESMS::LiveCellRef<ESM::NPC, MWWorld::RefData>* npc = actor.get<ESM::NPC>();
        if(npc->base->flags&npc->base->Female)
        {
            if(static_cast<int> (info.data.gender)==0)  return false;
        }
        else
        {
            if(static_cast<int> (info.data.gender)==1)  return false;
        }


        // check cell
        if (!info.cell.empty())
            if (mEnvironment.mWorld->getPlayer().getPlayer().getCell()->cell->name != info.cell)
                return false;

        // TODO check DATAstruct

        for (std::vector<ESM::DialInfo::SelectStruct>::const_iterator iter (info.selects.begin());
            iter != info.selects.end(); ++iter)
            if (!isMatching (actor, *iter))
                return false;

        /*std::cout
        << "unchecked entries:" << std::endl
        << "    player faction: " << info.pcFaction << std::endl
        << "    disposition: " << info.data.disposition << std::endl
        << "    NPC rank: " << static_cast<int> (info.data.rank) << std::endl
        << "    gender: " << static_cast<int> (info.data.gender) << std::endl
        << "    PC rank: " << static_cast<int> (info.data.PCrank) << std::endl;*/

        return true;
    }

    DialogueManager::DialogueManager (MWWorld::Environment& environment,const Compiler::Extensions& extensions) :
    mEnvironment (environment),mCompilerContext (MWScript::CompilerContext::Type_Dialgoue, environment),
        mErrorStream(std::cout.rdbuf()),mErrorHandler(mErrorStream)
    {
        mChoice = -1;
        mIsInChoice = false;
        mCompilerContext.setExtensions (&extensions);
    }

    void DialogueManager::addTopic(std::string topic)
    {
        knownTopics[toLower(topic)] = true;
    }

    void DialogueManager::parseText(std::string text)
    {
        std::map<std::string,std::list <ESM::DialInfo> >::iterator it;
        for(it = actorKnownTopics.begin();it != actorKnownTopics.end();it++)
        {
            MWGui::DialogueWindow* win = mEnvironment.mWindowManager->getDialogueWindow();
            size_t pos = find_str_ci(text,it->first,0);
            if(pos !=std::string::npos)
            {
                if(!it->second.empty())
                {
                    if(pos==0)
                    {
                        knownTopics[it->first] = true;
                        win->addKeyword(it->first);
                    }
                    else if(text.substr(pos -1,1) == " ")
                    {
                        knownTopics[it->first] = true;
                        win->addKeyword(it->first);
                    }
                }
            }

        }
    }

    void DialogueManager::startDialogue (const MWWorld::Ptr& actor)
    {
        mChoice = -1;
        mIsInChoice = false;
        std::cout << "talking with " << MWWorld::Class::get (actor).getName (actor) << std::endl;

        mActor = actor;

        //initialise the GUI
        mEnvironment.mInputManager->setGuiMode(MWGui::GM_Dialogue);
        MWGui::DialogueWindow* win = mEnvironment.mWindowManager->getDialogueWindow();
        win->startDialogue(MWWorld::Class::get (actor).getName (actor));

        //setup the list of topics known by the actor. Topics who are also on the knownTopics list will be added to the GUI
        actorKnownTopics.clear();
        ESMS::RecListT<ESM::Dialogue>::MapType dialogueList = mEnvironment.mWorld->getStore().dialogs.list;
        for(ESMS::RecListT<ESM::Dialogue>::MapType::iterator it = dialogueList.begin(); it!=dialogueList.end();it++)
        {
            ESM::Dialogue ndialogue = it->second;
            if(ndialogue.type == ESM::Dialogue::Topic)
            {
                for (std::vector<ESM::DialInfo>::const_iterator iter (it->second.mInfo.begin());
                    iter!=it->second.mInfo.end(); ++iter)
                {
                    if (isMatching (actor, *iter) && functionFilter(mActor,*iter,false))
                    {
                        actorKnownTopics[it->first].push_back(*iter);
                        //does the player know the topic?
                        if(knownTopics.find(toLower(it->first)) != knownTopics.end())
                        {
                            MWGui::DialogueWindow* win = mEnvironment.mWindowManager->getDialogueWindow();
                            win->addKeyword(it->first);
                        }
                    }
                }
            }
        }

        //greeting
        bool greetingFound = false;
        for(ESMS::RecListT<ESM::Dialogue>::MapType::iterator it = dialogueList.begin(); it!=dialogueList.end();it++)
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
        std::cout << cmd << std::endl;
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
                locals = mEnvironment.mScriptManager->getLocals (actorScript);
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
        catch (const Compiler::SourceException& error)
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
                MWScript::InterpreterContext interpreterContext(mEnvironment,&mActor.getRefData().getLocals(),mActor);
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

    void DialogueManager::keywordSelected(std::string keyword)
    {
        if(!mIsInChoice)
        {
            if(!actorKnownTopics[keyword].empty())
            {
                for(std::list<ESM::DialInfo>::iterator it = actorKnownTopics[keyword].begin(); it != actorKnownTopics[keyword].end();it++)
                {
                    ESM::DialInfo dial = *it;
                    if(functionFilter(mActor,dial,true))
                    {
                        std::string text = it->response;
                        std::string script = it->resultScript;

                        parseText(text);

                        MWGui::DialogueWindow* win = mEnvironment.mWindowManager->getDialogueWindow();
                        win->addTitle(keyword);
                        win->addText(it->response);

                        executeScript(script);

                        mLastTopic = keyword;
                        mLastDialogue = dial;
                        break;
                    }
                }
            }
        }
    }

    void DialogueManager::goodbyeSelected()
    {
        mEnvironment.mInputManager->setGuiMode(MWGui::GM_Game);
    }

    void DialogueManager::questionAnswered(std::string answere)
    {
        if(mChoiceMap.find(answere) != mChoiceMap.end())
        {
            mChoice = mChoiceMap[answere];
            std::list<ESM::DialInfo> dials = actorKnownTopics[mLastTopic];

            std::cout << actorKnownTopics[mLastTopic].size() << mLastTopic;
            std::list<ESM::DialInfo>::iterator iter;
            for(iter = actorKnownTopics[mLastTopic].begin(); iter->id != mLastDialogue.id;iter++)
            {
            }
            for(std::list<ESM::DialInfo>::iterator it = iter; it!=actorKnownTopics[mLastTopic].begin();)
            {
                it--;
                if(functionFilter(mActor,*it,true))
                {
                    mChoiceMap.clear();
                    mChoice = -1;
                    mIsInChoice = false;
                    MWGui::DialogueWindow* win = mEnvironment.mWindowManager->getDialogueWindow();
                    std::string text = it->response;
                    parseText(text);
                    win->addText(text);
                    executeScript(it->resultScript);
                    mLastTopic = mLastTopic;
                    mLastDialogue = *it;
                    break;
                }
            }
        }
    }

    void DialogueManager::printError(std::string error)
    {
        MWGui::DialogueWindow* win = mEnvironment.mWindowManager->getDialogueWindow();
        win->addText(error);
    }

    void DialogueManager::askQuestion(std::string question, int choice)
    {
        MWGui::DialogueWindow* win = mEnvironment.mWindowManager->getDialogueWindow();
        win->askQuestion(question);
        mChoiceMap[question] = choice;
        mIsInChoice = true;
    }
}
