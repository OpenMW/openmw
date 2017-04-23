#include <components/openmw-mp/Log.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwmechanics/mechanicsmanagerimp.hpp"
#include "../mwmechanics/movement.hpp"
#include "../mwmechanics/npcstats.hpp"

#include "../mwrender/animation.hpp"

#include "../mwworld/cellstore.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/worldimp.hpp"

#include "DedicatedActor.hpp"
#include "Main.hpp"
#include "CellController.hpp"
#include "MechanicsHelper.hpp"

using namespace mwmp;
using namespace std;

DedicatedActor::DedicatedActor()
{
    drawState = 0;
    movementFlags = 0;
    animation.groupname = "";
    sound = "";

    creatureStats = new ESM::CreatureStats();
    creatureStats->blank();
    creatureStats->mDynamic[0].mBase = -1;

    hasChangedCell = true;
}

DedicatedActor::~DedicatedActor()
{

}

void DedicatedActor::update(float dt)
{
    move(dt);
    setAnimFlags();
    playAnimation();
    playSound();
    setStatsDynamic();
}

void DedicatedActor::setCell(MWWorld::CellStore *cellStore)
{
    MWBase::World *world = MWBase::Environment::get().getWorld();

    ptr = world->moveObject(ptr, cellStore, position.pos[0], position.pos[1], position.pos[2]);

    hasChangedCell = true;
}

void DedicatedActor::move(float dt)
{
    ESM::Position refPos = ptr.getRefData().getPosition();
    MWBase::World *world = MWBase::Environment::get().getWorld();

    // Don't apply linear interpolation if the DedicatedActor has just gone through a cell change, because
    // the interpolated position will be invalid, causing a slight hopping glitch
    if (!hasChangedCell)
    {
        static const int timeMultiplier = 15;
        osg::Vec3f lerp = Main::get().getMechanicsHelper()->getLinearInterpolation(refPos.asVec3(), position.asVec3(), dt * timeMultiplier);
        refPos.pos[0] = lerp.x();
        refPos.pos[1] = lerp.y();
        refPos.pos[2] = lerp.z();

        world->moveObject(ptr, refPos.pos[0], refPos.pos[1], refPos.pos[2]);
    }
    else
    {
        world->moveObject(ptr, position.pos[0], position.pos[1], position.pos[2]);
        hasChangedCell = false;
    }

    MWMechanics::Movement *move = &ptr.getClass().getMovementSettings(ptr);
    move->mPosition[0] = direction.pos[0];
    move->mPosition[1] = direction.pos[1];
    move->mPosition[2] = direction.pos[2];

    world->rotateObject(ptr, position.rot[0], position.rot[1], position.rot[2]);
}

void DedicatedActor::setAnimFlags()
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

void DedicatedActor::playAnimation()
{
    if (!animation.groupname.empty())
    {
        MWBase::Environment::get().getMechanicsManager()->playAnimationGroup(ptr,
            animation.groupname, animation.mode, animation.count, animation.persist);

        animation.groupname.clear();
    }
}

void DedicatedActor::playSound()
{
    if (!sound.empty())
    {
        MWBase::Environment::get().getSoundManager()->say(ptr, sound);

        MWBase::WindowManager *winMgr = MWBase::Environment::get().getWindowManager();
        if (winMgr->getSubtitlesEnabled())
            winMgr->messageBox(response);

        sound.clear();
    }
}

void DedicatedActor::setStatsDynamic()
{
    // Only set dynamic stats if they have valid values
    if (creatureStats->mDynamic[0].mBase == -1) return;

    MWMechanics::NpcStats *ptrNpcStats = &ptr.getClass().getNpcStats(ptr);
    MWMechanics::DynamicStat<float> value;

    for (int i = 0; i < 3; ++i)
    {
        value.readState(creatureStats->mDynamic[i]);
        ptrNpcStats->setDynamic(i, value);
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
