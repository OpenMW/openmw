
#include "dialoguemanager.hpp"

#include <components/esm/loadinfo.hpp>
#include <components/esm/loaddial.hpp>

#include <components/esm_store/store.hpp>

#include "../mwworld/class.hpp"
#include "../mwworld/environment.hpp"
#include "../mwworld/world.hpp"

#include <iostream>

namespace MWDialogue
{
    bool DialogueManager::isMatching (const MWWorld::Ptr& actor, const ESM::DialInfo& info) const
    {
        // TODO check actor id
        // TODO check actor race
        // TODO check actor class
        // TODO check actor faction
        // TODO check player faction
        // TODO check cell
        // TODO check DATAstruct
        // TODO check select structures

        std::cout
            << "unchecked entries:" << std::endl
            << "    actor id: " << info.actor << std::endl
            << "    actor race: " << info.race << std::endl
            << "    actor class: " << info.clas << std::endl
            << "    actor faction: " << info.npcFaction << std::endl
            << "    player faction: " << info.pcFaction << std::endl
            << "    cell: " << info.cell << std::endl
            << "    DATAstruct" << std::endl;

        for (std::vector<ESM::DialInfo::SelectStruct>::const_iterator iter (info.selects.begin());
            iter != info.selects.end(); ++iter)
            std::cout << "    select: " << iter->selectRule << std::endl;

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
