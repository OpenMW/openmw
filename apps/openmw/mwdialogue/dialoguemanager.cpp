
#include "dialoguemanager.hpp"

#include "../mwworld/class.hpp"

#include <iostream>

namespace MWDialogue
{
    DialogueManager::DialogueManager (MWWorld::Environment& environment) : mEnvironment (environment) {}

    void DialogueManager::startDialogue (const MWWorld::Ptr& actor)
    {
        std::cout << "talking with " << MWWorld::Class::get (actor).getName (actor) << std::endl;
    }

}
