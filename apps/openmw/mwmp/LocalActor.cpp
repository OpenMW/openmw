#include <components/openmw-mp/Log.hpp>

#include "../mwbase/environment.hpp"
#include "../mwmechanics/movement.hpp"
#include "../mwrender/animation.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/worldimp.hpp"

#include "LocalActor.hpp"
#include "Main.hpp"
#include "Networking.hpp"
#include "ActorList.hpp"

using namespace mwmp;
using namespace std;

LocalActor::LocalActor()
{
    posWasChanged = false;

    wasRunning = false;
    wasSneaking = false;
    wasForceJumping = false;
    wasForceMoveJumping = false;

    wasJumping = false;
    wasFlying = false;
}

LocalActor::~LocalActor()
{

}

void LocalActor::update(bool forceUpdate)
{
    updatePosition(forceUpdate);
    updateAnimFlags(forceUpdate);
}

void LocalActor::updatePosition(bool forceUpdate)
{
    bool posIsChanging = (direction.pos[0] != 0 || direction.pos[1] != 0 || direction.pos[2] != 0 ||
        direction.rot[0] != 0 || direction.rot[1] != 0 || direction.rot[2] != 0);

    if (posIsChanging || posWasChanged || forceUpdate)
    {
        posWasChanged = posIsChanging;

        position = ptr.getRefData().getPosition();

        mwmp::Main::get().getNetworking()->getActorList()->addPositionActor(*this);
    }
}

void LocalActor::updateAnimFlags(bool forceUpdate)
{
    MWBase::World *world = MWBase::Environment::get().getWorld();
    MWMechanics::NpcStats ptrNpcStats = ptr.getClass().getNpcStats(ptr);

    using namespace MWMechanics;

    bool isRunning = ptrNpcStats.getMovementFlag(CreatureStats::Flag_Run);
    bool isSneaking = ptrNpcStats.getMovementFlag(CreatureStats::Flag_Sneak);
    bool isForceJumping = ptrNpcStats.getMovementFlag(CreatureStats::Flag_ForceJump);
    bool isForceMoveJumping = ptrNpcStats.getMovementFlag(CreatureStats::Flag_ForceMoveJump);

    isFlying = world->isFlying(ptr);
    bool isJumping = !world->isOnGround(ptr) && !isFlying;

    MWMechanics::DrawState_ currentDrawState = ptr.getClass().getNpcStats(ptr).getDrawState();

    if (wasRunning != isRunning
        || wasSneaking != isSneaking || wasForceJumping != isForceJumping
        || wasForceMoveJumping != isForceMoveJumping || lastDrawState != currentDrawState
        || wasJumping || isJumping || wasFlying || isFlying
        || forceUpdate)
    {
        wasRunning = isRunning;
        wasSneaking = isSneaking;
        wasForceJumping = isForceJumping;
        wasForceMoveJumping = isForceMoveJumping;
        lastDrawState = currentDrawState;

        wasFlying = isFlying;
        wasJumping = isJumping;

        movementFlags = 0;

#define __SETFLAG(flag, value) (value) ? (movementFlags | flag) : (movementFlags & ~flag)

        movementFlags = __SETFLAG(CreatureStats::Flag_Sneak, isSneaking);
        movementFlags = __SETFLAG(CreatureStats::Flag_Run, isRunning);
        movementFlags = __SETFLAG(CreatureStats::Flag_ForceJump, isForceJumping);
        movementFlags = __SETFLAG(CreatureStats::Flag_ForceJump, isJumping);
        movementFlags = __SETFLAG(CreatureStats::Flag_ForceMoveJump, isForceMoveJumping);

#undef __SETFLAG

        if (currentDrawState == MWMechanics::DrawState_Nothing)
            drawState = 0;
        else if (currentDrawState == MWMechanics::DrawState_Weapon)
            drawState = 1;
        else if (currentDrawState == MWMechanics::DrawState_Spell)
            drawState = 2;

        if (isJumping)
            updatePosition(true); // fix position after jump;

        mwmp::Main::get().getNetworking()->getActorList()->addDrawStateActor(*this);
    }
}

MWWorld::Ptr LocalActor::getPtr()
{
    return ptr;
}

void LocalActor::setPtr(const MWWorld::Ptr& newPtr)
{
    ptr = newPtr;

    refId = ptr.getCellRef().getRefId();
    refNumIndex = ptr.getCellRef().getRefNum().mIndex;
    mpNum = ptr.getCellRef().getMpNum();

    lastDrawState = ptr.getClass().getNpcStats(ptr).getDrawState();
}
