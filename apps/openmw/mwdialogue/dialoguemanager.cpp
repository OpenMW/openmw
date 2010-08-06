
#include "dialoguemanager.hpp"

#include "../mwworld/class.hpp"

#include <iostream>

namespace MWDialog
{
    DialogManager::DialogManager (MWWorld::Environment& environment) : mEnvironment (environment) {}

    void DialogManager::startDialog (const MWWorld::Ptr& actor)
    {
        std::cout << "talking with " << MWWorld::Class::get (actor).getName (actor) << std::endl;
    }

}
