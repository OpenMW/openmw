#include "Miscellaneous.hpp"
#include <apps/openmw-mp/Script/ScriptFunctions.hpp>
#include <apps/openmw-mp/Networking.hpp>

#include <iostream>
using namespace std;

unsigned int MiscellaneousFunctions::GetLastPlayerId() noexcept
{
    return Players::getLastPlayerId();
}
