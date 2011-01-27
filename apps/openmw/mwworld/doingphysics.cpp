
#include "doingphysics.hpp"

namespace MWWorld
{
    int DoingPhysics::sCounter = 0;

    DoingPhysics::DoingPhysics()
    {
        ++sCounter;
    }

    DoingPhysics::~DoingPhysics()
    {
        --sCounter;
    }

    bool DoingPhysics::isDoingPhysics()
    {
        return sCounter>0;
    }
}
