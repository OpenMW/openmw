#include <components/openmw-mp/Log.hpp>

#include "../mwbase/environment.hpp"

#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/movement.hpp"

#include "../mwrender/animation.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/worldimp.hpp"

#include "LocalActor.hpp"
#include "Main.hpp"
#include "Networking.hpp"
#include "ActorList.hpp"
#include "MechanicsHelper.hpp"

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

    statTimer = 0;

    attack.type = Attack::MELEE;
    attack.shouldSend = false;

    creatureStats = new ESM::CreatureStats();
}

LocalActor::~LocalActor()
{

}

void LocalActor::update(bool forceUpdate)
{
    updatePosition(forceUpdate);
    updateAnimFlags(forceUpdate);
    updateAnimPlay();
    updateSpeech();
    updateStatsDynamic(forceUpdate);
    updateAttack();
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

        mwmp::Main::get().getNetworking()->getActorList()->addAnimFlagsActor(*this);
    }
}

void LocalActor::updateAnimPlay()
{
    if (!animation.groupname.empty())
    {
        mwmp::Main::get().getNetworking()->getActorList()->addAnimPlayActor(*this);
        animation.groupname.clear();
    }
}

void LocalActor::updateSpeech()
{
    if (!sound.empty())
    {
        mwmp::Main::get().getNetworking()->getActorList()->addSpeechActor(*this);
        sound.clear();
    }
}

void LocalActor::updateStatsDynamic(bool forceUpdate)
{
    MWMechanics::CreatureStats *ptrCreatureStats = &ptr.getClass().getCreatureStats(ptr);
    MWMechanics::DynamicStat<float> health(ptrCreatureStats->getHealth());
    MWMechanics::DynamicStat<float> magicka(ptrCreatureStats->getMagicka());
    MWMechanics::DynamicStat<float> fatigue(ptrCreatureStats->getFatigue());

    const float timeoutSec = 0.5;

    if ((statTimer += MWBase::Environment::get().getFrameDuration()) >= timeoutSec || forceUpdate)
    {
        if (oldHealth != health || oldMagicka != magicka || oldFatigue != fatigue || forceUpdate)
        {
            oldHealth = health;
            oldMagicka = magicka;
            oldFatigue = fatigue;

            health.writeState(creatureStats->mDynamic[0]);
            magicka.writeState(creatureStats->mDynamic[1]);
            fatigue.writeState(creatureStats->mDynamic[2]);

            statTimer = 0;

            mwmp::Main::get().getNetworking()->getActorList()->addStatsDynamicActor(*this);
        }
    }
}

void LocalActor::updateAttack()
{
    if (attack.shouldSend)
    {
        if (attack.type == Attack::MAGIC)
        {
            MWMechanics::CreatureStats &attackerStats = ptr.getClass().getNpcStats(ptr);
            attack.spellId = attackerStats.getSpells().getSelectedSpell();
            attack.success = mwmp::Main::get().getMechanicsHelper()->getSpellSuccess(attack.spellId, ptr);
        }

        mwmp::Main::get().getNetworking()->getActorList()->addAttackActor(*this);
        attack.shouldSend = false;
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
    oldHealth = ptr.getClass().getCreatureStats(ptr).getHealth();
    oldMagicka = ptr.getClass().getCreatureStats(ptr).getMagicka();
    oldFatigue = ptr.getClass().getCreatureStats(ptr).getFatigue();
}
