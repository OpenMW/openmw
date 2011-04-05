
#include "doingphysics.hpp"

namespace MWWorld
{
    int DoingPhysics::sCounter = 0;
    int DoingPhysics::sSuppress = 0;

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
        return sCounter>0 && sSuppress==0;
    }

    SuppressDoingPhysics::SuppressDoingPhysics()
    {
        ++DoingPhysics::sSuppress;
    }

    SuppressDoingPhysics::~SuppressDoingPhysics()
    {
        --DoingPhysics::sSuppress;
    }
}
