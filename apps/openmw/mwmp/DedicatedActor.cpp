#include <components/openmw-mp/Log.hpp>

#include "../mwbase/environment.hpp"
#include "../mwmechanics/mechanicsmanagerimp.hpp"
#include "../mwmechanics/movement.hpp"
#include "../mwmechanics/npcstats.hpp"
#include "../mwrender/animation.hpp"
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

void DedicatedActor::update(float dt)
{
    move(dt);
    setDrawState();
    setAnimation();
}

void DedicatedActor::move(float dt)
{
    MWBase::World *world = MWBase::Environment::get().getWorld();

    world->moveObject(ptr, position.pos[0], position.pos[1], position.pos[2]);

    world->rotateObject(ptr, position.rot[0], position.rot[1], position.rot[2]);
}

void DedicatedActor::setDrawState()
{
    using namespace MWMechanics;

    if (drawState == 0)
        ptr.getClass().getNpcStats(ptr).setDrawState(DrawState_Nothing);
    else if (drawState == 1)
        ptr.getClass().getNpcStats(ptr).setDrawState(DrawState_Weapon);
    else if (drawState == 2)
        ptr.getClass().getNpcStats(ptr).setDrawState(DrawState_Spell);
}

void DedicatedActor::setAnimation()
{
    if (hasAnimation)
    {
        MWBase::Environment::get().getMechanicsManager()->playAnimationGroup(ptr,
            animation.groupname, animation.mode, animation.count, animation.persist);
    }
}

MWWorld::Ptr DedicatedActor::getPtr()
{
    return ptr;
}

void DedicatedActor::setPtr(const MWWorld::Ptr& newPtr)
{
    ptr = newPtr;
}
