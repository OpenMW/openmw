#include "../mwbase/environment.hpp"
#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/npcstats.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/worldimp.hpp"

#include "DedicatedActor.hpp"

using namespace mwmp;
using namespace std;

DedicatedActor::DedicatedActor()
{

}

DedicatedActor::~DedicatedActor()
{

}

void DedicatedActor::update()
{

}

void DedicatedActor::move()
{
    MWBase::World *world = MWBase::Environment::get().getWorld();

    world->moveObject(ptr, position.pos[0], position.pos[1], position.pos[2]);
    world->rotateObject(ptr, position.rot[0], position.rot[1], position.rot[2]);
}

MWWorld::Ptr DedicatedActor::getPtr()
{
    return ptr;
}

void DedicatedActor::setPtr(const MWWorld::Ptr& newPtr)
{
    ptr = newPtr;
}
