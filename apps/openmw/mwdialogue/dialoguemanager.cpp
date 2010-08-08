
#include "dialoguemanager.hpp"

#include <components/esm/loaddial.hpp>

#include <components/esm_store/store.hpp>

#include "../mwworld/class.hpp"
#include "../mwworld/environment.hpp"
#include "../mwworld/world.hpp"
#include "../mwworld/refdata.hpp"

#include <iostream>

namespace
{
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
}

namespace MWDialogue
{
    bool DialogueManager::isMatching (const MWWorld::Ptr& actor,
        const ESM::DialInfo::SelectStruct& select) const
    {
        char type = select.selectRule[1];

        if (type!='0')
        {
            char comp = select.selectRule[4];
            std::string name = select.selectRule.substr (5);

            // TODO types 1, 2, 4, 5, 6, 7, 8, 9, A, B, C

            switch (type)
            {
                case '3': // local

                    if (select.type==ESM::VT_Short || select.type==ESM::VT_Int ||
                        select.type==ESM::VT_Long)
                    {
                        if (!checkLocal (comp, name, select.i, actor,
                            mEnvironment.mWorld->getStore()))
                            return false;
                    }
                    else if (select.type==ESM::VT_Float)
                    {
                        if (!checkLocal (comp, name, select.f, actor,
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
        // TODO check actor id
        // TODO check actor race
        // TODO check actor class
        // TODO check actor faction
        // TODO check player faction

        // check cell
        if (!info.cell.empty())
            if (mEnvironment.mWorld->getPlayerPos().getPlayer().getCell()->cell->name != info.cell)
                return false;

        // TODO check DATAstruct

        for (std::vector<ESM::DialInfo::SelectStruct>::const_iterator iter (info.selects.begin());
            iter != info.selects.end(); ++iter)
            if (!isMatching (actor, *iter))
                return false;

        std::cout
            << "unchecked entries:" << std::endl
            << "    actor id: " << info.actor << std::endl
            << "    actor race: " << info.race << std::endl
            << "    actor class: " << info.clas << std::endl
            << "    actor faction: " << info.npcFaction << std::endl
            << "    player faction: " << info.pcFaction << std::endl
            << "    DATAstruct" << std::endl;

        return true;
    }

    DialogueManager::DialogueManager (MWWorld::Environment& environment) : mEnvironment (environment) {}

    void DialogueManager::startDialogue (const MWWorld::Ptr& actor)
    {
        std::cout << "talking with " << MWWorld::Class::get (actor).getName (actor) << std::endl;

        const ESM::Dialogue *dialogue = mEnvironment.mWorld->getStore().dialogs.find ("hello");

        for (std::vector<ESM::DialInfo>::const_iterator iter (dialogue->mInfo.begin());
            iter!=dialogue->mInfo.end(); ++iter)
        {
            if (isMatching (actor, *iter))
            {
                // start dialogue
                std::cout << "found matching info record" << std::endl;

                std::cout << "response: " << iter->response << std::endl;

                if (!iter->sound.empty())
                {
                    // TODO play sound
                }

                if (!iter->resultScript.empty())
                {
                    // TODO execute script
                }

                break;
            }
        }
    }

}
