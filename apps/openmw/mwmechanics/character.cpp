/*
 * OpenMW - The completely unofficial reimplementation of Morrowind
 *
 * This file (character.cpp) is part of the OpenMW package.
 *
 * OpenMW is distributed as free software: you can redistribute it
 * and/or modify it under the terms of the GNU General Public License
 * version 3, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 3 along with this program. If not, see
 * http://www.gnu.org/licenses/ .
 */

#include "character.hpp"

#include <OgreStringConverter.h>

#include "movement.hpp"
#include "npcstats.hpp"

#include "../mwrender/animation.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/player.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/inventorystore.hpp"


namespace MWMechanics
{

static const struct {
    CharacterState state;
    const char groupname[32];
    MWRender::Animation::Priority priority;
    bool loops;
} sStateList[] = {
    { CharState_Idle, "idle", MWRender::Animation::Priority_Default, true },
    { CharState_Idle2, "idle2", MWRender::Animation::Priority_Default, true },
    { CharState_Idle3, "idle3", MWRender::Animation::Priority_Default, true },
    { CharState_Idle4, "idle4", MWRender::Animation::Priority_Default, true },
    { CharState_Idle5, "idle5", MWRender::Animation::Priority_Default, true },
    { CharState_Idle6, "idle6", MWRender::Animation::Priority_Default, true },
    { CharState_Idle7, "idle7", MWRender::Animation::Priority_Default, true },
    { CharState_Idle8, "idle8", MWRender::Animation::Priority_Default, true },
    { CharState_Idle9, "idle9", MWRender::Animation::Priority_Default, true },
    { CharState_IdleSwim, "idleswim", MWRender::Animation::Priority_Default, true },
    { CharState_IdleSneak, "idlesneak", MWRender::Animation::Priority_Default, true },

    { CharState_WalkForward, "walkforward", MWRender::Animation::Priority_Default, true },
    { CharState_WalkBack, "walkback", MWRender::Animation::Priority_Default, true },
    { CharState_WalkLeft, "walkleft", MWRender::Animation::Priority_Default, true },
    { CharState_WalkRight, "walkright", MWRender::Animation::Priority_Default, true },

    { CharState_SwimWalkForward, "swimwalkforward", MWRender::Animation::Priority_Default, true },
    { CharState_SwimWalkBack, "swimwalkback", MWRender::Animation::Priority_Default, true },
    { CharState_SwimWalkLeft, "swimwalkleft", MWRender::Animation::Priority_Default, true },
    { CharState_SwimWalkRight, "swimwalkright", MWRender::Animation::Priority_Default, true },

    { CharState_RunForward, "runforward", MWRender::Animation::Priority_Default, true },
    { CharState_RunBack, "runback", MWRender::Animation::Priority_Default, true },
    { CharState_RunLeft, "runleft", MWRender::Animation::Priority_Default, true },
    { CharState_RunRight, "runright", MWRender::Animation::Priority_Default, true },

    { CharState_SwimRunForward, "swimrunforward", MWRender::Animation::Priority_Default, true },
    { CharState_SwimRunBack, "swimrunback", MWRender::Animation::Priority_Default, true },
    { CharState_SwimRunLeft, "swimrunleft", MWRender::Animation::Priority_Default, true },
    { CharState_SwimRunRight, "swimrunright", MWRender::Animation::Priority_Default, true },

    { CharState_SneakForward, "sneakforward", MWRender::Animation::Priority_Default, true },
    { CharState_SneakBack, "sneakback", MWRender::Animation::Priority_Default, true },
    { CharState_SneakLeft, "sneakleft", MWRender::Animation::Priority_Default, true },
    { CharState_SneakRight, "sneakright", MWRender::Animation::Priority_Default, true },

    { CharState_TurnLeft, "turnleft", MWRender::Animation::Priority_Default, true },
    { CharState_TurnRight, "turnright", MWRender::Animation::Priority_Default, true },

    { CharState_Jump, "jump", MWRender::Animation::Priority_Default, true },

    { CharState_Death1, "death1", MWRender::Animation::Priority_Death, false },
    { CharState_Death2, "death2", MWRender::Animation::Priority_Death, false },
    { CharState_Death3, "death3", MWRender::Animation::Priority_Death, false },
    { CharState_Death4, "death4", MWRender::Animation::Priority_Death, false },
    { CharState_Death5, "death5", MWRender::Animation::Priority_Death, false },
};
static const size_t sStateListSize = sizeof(sStateList)/sizeof(sStateList[0]);

static const struct {
    WeaponType type;
    const char idlegroup[16];
    const char movementgroup[16];
    const char actiongroup[16];
} sWeaponTypeList[] = {
    { WeapType_HandToHand, "hh", "hh", "handtohand" },
    { WeapType_OneHand, "1h", "1h", "weapononehand" },
    { WeapType_TwoHand, "2c", "2c", "weapontwohand" },
    { WeapType_TwoWide, "2w", "2w", "weapontwowide" },
    { WeapType_BowAndArrow, "1h", "1h", "bowandarrow" },
    { WeapType_Crossbow, "crossbow", "2c", "crossbow" },
    { WeapType_ThowWeapon, "1h", "1h", "throwweapon" },
    { WeapType_Spell, "spell", "", "spellcast" },
};
static const size_t sWeaponTypeListSize = sizeof(sWeaponTypeList)/sizeof(sWeaponTypeList[0]);


void CharacterController::getCurrentGroup(std::string &group, MWRender::Animation::Priority &priority, bool &loops) const
{
    std::string name;
    for(size_t i = 0;i < sStateListSize;i++)
    {
        if(sStateList[i].state == mCharState)
        {
            name = sStateList[i].groupname;
            priority = sStateList[i].priority;
            loops = sStateList[i].loops;
            break;
        }
    }
    if(name.empty())
        throw std::runtime_error("Failed to find character state "+Ogre::StringConverter::toString(mCharState));

    if(!(mCharState >= CharState_Death1) && mWeaponType != WeapType_None)
    {
        for(size_t i = 0;i < sWeaponTypeListSize;i++)
        {
            if(sWeaponTypeList[i].type == mWeaponType)
            {
                if(mCharState == CharState_Idle)
                    (group=name) += sWeaponTypeList[i].idlegroup;
                else
                    (group=name) += sWeaponTypeList[i].movementgroup;
                break;
            }
        }
    }

    if(group.empty() || !mAnimation->hasAnimation(group))
        group = (mAnimation->hasAnimation(name) ? name : std::string());
}


CharacterController::CharacterController(const MWWorld::Ptr &ptr, MWRender::Animation *anim, CharacterState state)
    : mPtr(ptr)
    , mAnimation(anim)
    , mCharState(state)
    , mWeaponType(WeapType_None)
    , mSkipAnim(false)
    , mSecondsOfRunning(0)
    , mSecondsOfSwimming(0)
{
    if(!mAnimation)
        return;

    if(MWWorld::Class::get(mPtr).isActor())
    {
        /* Accumulate along X/Y only for now, until we can figure out how we should
         * handle knockout and death which moves the character down. */
        mAnimation->setAccumulation(Ogre::Vector3(1.0f, 1.0f, 0.0f));
    }
    else
    {
        /* Don't accumulate with non-actors. */
        mAnimation->setAccumulation(Ogre::Vector3(0.0f));
    }

    std::string group;
    MWRender::Animation::Priority prio;
    bool loops;
    getCurrentGroup(group, prio, loops);
    mAnimation->play(group, prio, MWRender::Animation::Group_All, false,
                     "start", "stop", 1.0f, loops ? (~(size_t)0) : 0);
}

CharacterController::~CharacterController()
{
}


void CharacterController::updatePtr(const MWWorld::Ptr &ptr)
{
    mPtr = ptr;
}


void CharacterController::update(float duration, Movement &movement)
{
    float speed = 0.0f;
    if(!(getState() >= CharState_Death1))
    {
        MWBase::World *world = MWBase::Environment::get().getWorld();
        const MWWorld::Class &cls = MWWorld::Class::get(mPtr);

        bool onground = world->isOnGround(mPtr);
        bool inwater = world->isSwimming(mPtr);
        bool isrunning = cls.getStance(mPtr, MWWorld::Class::Run);
        bool sneak = cls.getStance(mPtr, MWWorld::Class::Sneak);
        const Ogre::Vector3 &vec = cls.getMovementVector(mPtr);
        const Ogre::Vector3 &rot = cls.getRotationVector(mPtr);
        speed = cls.getSpeed(mPtr);

        // advance athletics
        if (vec.squaredLength() > 0 && mPtr == world->getPlayer().getPlayer())
        {
            if (inwater)
            {
                mSecondsOfSwimming += duration;
                while(mSecondsOfSwimming > 1)
                {
                    cls.skillUsageSucceeded(mPtr, ESM::Skill::Athletics, 1);
                    mSecondsOfSwimming -= 1;
                }
            }
            else if (isrunning)
            {
                mSecondsOfRunning += duration;
                while(mSecondsOfRunning > 1)
                {
                    cls.skillUsageSucceeded(mPtr, ESM::Skill::Athletics, 0);
                    mSecondsOfRunning -= 1;
                }
            }
        }

        if(mPtr.getTypeName() == typeid(ESM::NPC).name())
        {
            NpcStats &stats = cls.getNpcStats(mPtr);
            WeaponType weaptype = WeapType_None;

            if(stats.getDrawState() == DrawState_Spell)
                weaptype = WeapType_Spell;
            else if(stats.getDrawState() == MWMechanics::DrawState_Weapon)
            {
                MWWorld::InventoryStore &inv = cls.getInventoryStore(mPtr);
                MWWorld::ContainerStoreIterator weapon = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);
                if(weapon == inv.end())
                    weaptype = WeapType_HandToHand;
                else
                {
                    const std::string &type = weapon->getTypeName();
                    if(type == typeid(ESM::Lockpick).name() || type == typeid(ESM::Probe).name())
                        weaptype = WeapType_OneHand;
                    else if(type == typeid(ESM::Weapon).name())
                    {
                        MWWorld::LiveCellRef<ESM::Weapon> *ref = weapon->get<ESM::Weapon>();
                        ESM::Weapon::Type type = (ESM::Weapon::Type)ref->mBase->mData.mType;
                        switch(type)
                        {
                            case ESM::Weapon::ShortBladeOneHand:
                            case ESM::Weapon::LongBladeOneHand:
                            case ESM::Weapon::BluntOneHand:
                            case ESM::Weapon::AxeOneHand:
                            case ESM::Weapon::Arrow:
                            case ESM::Weapon::Bolt:
                                weaptype = WeapType_OneHand;
                                break;
                            case ESM::Weapon::LongBladeTwoHand:
                            case ESM::Weapon::BluntTwoClose:
                            case ESM::Weapon::AxeTwoHand:
                                weaptype = WeapType_TwoHand;
                                break;
                            case ESM::Weapon::BluntTwoWide:
                            case ESM::Weapon::SpearTwoWide:
                                weaptype = WeapType_TwoWide;
                                break;
                            case ESM::Weapon::MarksmanBow:
                                weaptype = WeapType_BowAndArrow;
                                break;
                            case ESM::Weapon::MarksmanCrossbow:
                                weaptype = WeapType_Crossbow;
                                break;
                            case ESM::Weapon::MarksmanThrown:
                                weaptype = WeapType_ThowWeapon;
                                break;
                        }
                    }
                }
            }

            if(weaptype != mWeaponType)
            {
                mWeaponType = weaptype;
                forceStateUpdate();
            }
        }

        /* FIXME: The state should be set to Jump, and X/Y movement should be disallowed except
         * for the initial thrust (which would be carried by "physics" until landing). */
        if(onground && vec.z > 0.0f)
        {
            float x = cls.getJump(mPtr);

            if(vec.x == 0 && vec.y == 0)
                movement.mPosition[2] += x*duration;
            else
            {
                /* FIXME: this would be more correct if we were going into a jumping state,
                 * rather than normal walking/idle states. */
                //Ogre::Vector3 lat = Ogre::Vector3(vec.x, vec.y, 0.0f).normalisedCopy();
                //movement += Ogre::Vector3(lat.x, lat.y, 1.0f) * x * 0.707f * duration;
                movement.mPosition[2] += x * 0.707f * duration;
            }

            //decrease fatigue by fFatigueJumpBase + (1 - normalizedEncumbrance) * fFatigueJumpMult;
        }

        if(std::abs(vec.x/2.0f) > std::abs(vec.y) && speed > 0.0f)
        {
            if(vec.x > 0.0f)
                setState(inwater ? (isrunning ? CharState_SwimRunRight : CharState_SwimWalkRight)
                                 : (sneak ? CharState_SneakRight : (isrunning ? CharState_RunRight : CharState_WalkRight)));
            else if(vec.x < 0.0f)
                setState(inwater ? (isrunning ? CharState_SwimRunLeft : CharState_SwimWalkLeft)
                                 : (sneak ? CharState_SneakLeft : (isrunning ? CharState_RunLeft : CharState_WalkLeft)));

            movement.mPosition[0] += vec.x * (speed*duration);
            movement.mPosition[1] += vec.y * (speed*duration);
        }
        else if(vec.y != 0.0f && speed > 0.0f)
        {
            if(vec.y > 0.0f)
                setState(inwater ? (isrunning ? CharState_SwimRunForward : CharState_SwimWalkForward)
                                 : (sneak ? CharState_SneakForward : (isrunning ? CharState_RunForward : CharState_WalkForward)));
            else if(vec.y < 0.0f)
                setState(inwater ? (isrunning ? CharState_SwimRunBack : CharState_SwimWalkBack)
                                 : (sneak ? CharState_SneakBack : (isrunning ? CharState_RunBack : CharState_WalkBack)));

            movement.mPosition[0] += vec.x * (speed*duration);
            movement.mPosition[1] += vec.y * (speed*duration);
        }
        else if(rot.z != 0.0f && !inwater && !sneak)
        {
            if(rot.z > 0.0f)
                setState(CharState_TurnRight);
            else if(rot.z < 0.0f)
                setState(CharState_TurnLeft);
        }
        else if(mAnimQueue.size() > 0)
        {
            if(mAnimation->isPlaying(mAnimQueue.front().first) == false)
            {
                mAnimQueue.pop_front();
                if(mAnimQueue.size() > 0)
                    mAnimation->play(mAnimQueue.front().first,
                                     MWRender::Animation::Priority_Default,
                                     MWRender::Animation::Group_All, false,
                                     "start", "stop", 0.0f, mAnimQueue.front().second);
            }
        }
        else if(getState() != CharState_SpecialIdle)
            setState((inwater ? CharState_IdleSwim : (sneak ? CharState_IdleSneak : CharState_Idle)));

        movement.mRotation[0] += rot.x * duration;
        movement.mRotation[1] += rot.y * duration;
        movement.mRotation[2] += rot.z * duration;
    }

    if(mAnimation && !mSkipAnim)
    {
        mAnimation->setSpeed(speed);

        Ogre::Vector3 moved = mAnimation->runAnimation(duration);
        // Ensure we're moving in generally the right direction
        if((movement.mPosition[0] < 0.0f && movement.mPosition[0] < moved.x*2.0f) ||
           (movement.mPosition[0] > 0.0f && movement.mPosition[0] > moved.x*2.0f))
            moved.x = movement.mPosition[0];
        if((movement.mPosition[1] < 0.0f && movement.mPosition[1] < moved.y*2.0f) ||
           (movement.mPosition[1] > 0.0f && movement.mPosition[1] > moved.y*2.0f))
            moved.y = movement.mPosition[1];
        if((movement.mPosition[2] < 0.0f && movement.mPosition[2] < moved.z*2.0f) ||
           (movement.mPosition[2] > 0.0f && movement.mPosition[2] > moved.z*2.0f))
            moved.z = movement.mPosition[2];

        movement.mPosition[0] = moved.x;
        movement.mPosition[1] = moved.y;
        movement.mPosition[2] = moved.z;
    }
    mSkipAnim = false;
}


void CharacterController::playGroup(const std::string &groupname, int mode, int count)
{
    if(!mAnimation || !mAnimation->hasAnimation(groupname))
        std::cerr<< "Animation "<<groupname<<" not found" <<std::endl;
    else
    {
        count = std::max(count, 1);
        if(mode != 0 || mAnimQueue.size() == 0)
        {
            mAnimQueue.clear();
            mAnimQueue.push_back(std::make_pair(groupname, count-1));

            mCharState = CharState_SpecialIdle;
            mAnimation->play(groupname, MWRender::Animation::Priority_Default,
                             MWRender::Animation::Group_All, false,
                             ((mode==2) ? "loop start" : "start"), "stop", 0.0f, count-1);
        }
        else if(mode == 0)
        {
            mAnimQueue.resize(1);
            mAnimQueue.push_back(std::make_pair(groupname, count-1));
        }
    }
}

void CharacterController::skipAnim()
{
    mSkipAnim = true;
}


void CharacterController::setState(CharacterState state)
{
    if(mCharState == state)
        return;
    mCharState = state;

    forceStateUpdate();
}

void CharacterController::forceStateUpdate()
{
    if(!mAnimation)
        return;
    mAnimQueue.clear();

    std::string group;
    MWRender::Animation::Priority prio;
    bool loops;
    getCurrentGroup(group, prio, loops);
    mAnimation->play(group, prio, MWRender::Animation::Group_All, false,
                     "start", "stop", 0.0f, loops ? (~(size_t)0) : 0);

    mAnimation->showWeapons(mWeaponType != WeapType_None && mWeaponType != WeapType_HandToHand &&
                            mWeaponType != WeapType_Spell);
}

}
