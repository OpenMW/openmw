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
    drawState = 0;
    movementFlags = 0;
    animation.groupname = "";
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

    MWMechanics::Movement *move = &ptr.getClass().getMovementSettings(ptr);
    move->mPosition[0] = direction.pos[0];
    move->mPosition[1] = direction.pos[1];
    move->mPosition[2] = direction.pos[2];

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

    MWMechanics::NpcStats *ptrNpcStats = &ptr.getClass().getNpcStats(ptr);
    ptrNpcStats->setMovementFlag(CreatureStats::Flag_Run, (movementFlags & CreatureStats::Flag_Run) != 0);
    ptrNpcStats->setMovementFlag(CreatureStats::Flag_Sneak, (movementFlags & CreatureStats::Flag_Sneak) != 0);
    ptrNpcStats->setMovementFlag(CreatureStats::Flag_ForceJump, (movementFlags & CreatureStats::Flag_ForceJump) != 0);
    ptrNpcStats->setMovementFlag(CreatureStats::Flag_ForceMoveJump, (movementFlags & CreatureStats::Flag_ForceMoveJump) != 0);
}

void DedicatedActor::setAnimation()
{
    if (!animation.groupname.empty())
    {
        MWBase::Environment::get().getMechanicsManager()->playAnimationGroup(ptr,
            animation.groupname, animation.mode, animation.count, animation.persist);

        animation.groupname.clear();
    }
}

MWWorld::Ptr DedicatedActor::getPtr()
{
    return ptr;
}

void DedicatedActor::setPtr(const MWWorld::Ptr& newPtr)
{
    ptr = newPtr;

    refId = ptr.getCellRef().getRefId();
    refNumIndex = ptr.getCellRef().getRefNum().mIndex;
    mpNum = ptr.getCellRef().getMpNum();
}
