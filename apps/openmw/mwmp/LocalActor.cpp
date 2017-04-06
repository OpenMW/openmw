#include "../mwbase/environment.hpp"
#include "../mwmechanics/movement.hpp"
#include "../mwmechanics/npcstats.hpp"
#include "../mwrender/animation.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/worldimp.hpp"

#include "LocalActor.hpp"

using namespace mwmp;
using namespace std;

LocalActor::LocalActor()
{
    headPitch = -1;
    headYaw = -1;
}

LocalActor::~LocalActor()
{

}

void LocalActor::update()
{
    updatePosition();
    updateDrawState();
    updateMovementFlags();
    updateAnimation();
}

void LocalActor::updatePosition()
{
    position = ptr.getRefData().getPosition();

    MWMechanics::Movement &move = ptr.getClass().getMovementSettings(ptr);
    direction.pos[0] = move.mPosition[0];
    direction.pos[1] = move.mPosition[1];
    direction.pos[2] = move.mPosition[2];
}

void LocalActor::updateDrawState()
{
    drawState = ptr.getClass().getNpcStats(ptr).getDrawState();
}

void LocalActor::updateMovementFlags()
{
    using namespace MWMechanics;

    MWBase::World *world = MWBase::Environment::get().getWorld();
    MWMechanics::NpcStats ptrNpcStats = ptr.getClass().getNpcStats(ptr);

    bool run = ptrNpcStats.getMovementFlag(CreatureStats::Flag_Run);
    bool sneak = ptrNpcStats.getMovementFlag(CreatureStats::Flag_Sneak);
    bool forceJump = ptrNpcStats.getMovementFlag(CreatureStats::Flag_ForceJump);
    bool forceMoveJump = ptrNpcStats.getMovementFlag(CreatureStats::Flag_ForceMoveJump);

#define __SETFLAG(flag, value) (value) ? (movementFlags | flag) : (movementFlags & ~flag)

    movementFlags = __SETFLAG(CreatureStats::Flag_Sneak, sneak);
    movementFlags = __SETFLAG(CreatureStats::Flag_Run, run);
    movementFlags = __SETFLAG(CreatureStats::Flag_ForceJump, forceJump);
    movementFlags = __SETFLAG(CreatureStats::Flag_ForceMoveJump, forceMoveJump);

#undef __SETFLAG
}

void LocalActor::updateAnimation()
{
    MWBase::World *world = MWBase::Environment::get().getWorld();

    MWRender::Animation *animation = world->getAnimation(ptr);

    if (animation)
    {
        headPitch = animation->getHeadPitch();
        headYaw = animation->getHeadYaw();
    }
}

MWWorld::Ptr LocalActor::getPtr()
{
    return ptr;
}

void LocalActor::setPtr(const MWWorld::Ptr& newPtr)
{
    ptr = newPtr;
}
