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
#include "creaturestats.hpp"

#include "../mwrender/animation.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/soundmanager.hpp"

#include "../mwworld/player.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/inventorystore.hpp"

namespace
{

int getBestAttack (const ESM::Weapon* weapon)
{
    int slash = (weapon->mData.mSlash[0] + weapon->mData.mSlash[1])/2;
    int chop = (weapon->mData.mChop[0] + weapon->mData.mChop[1])/2;
    int thrust = (weapon->mData.mThrust[0] + weapon->mData.mThrust[1])/2;
    if (slash >= chop && slash >= thrust)
        return MWMechanics::CreatureStats::AT_Slash;
    else if (chop >= slash && chop >= thrust)
        return MWMechanics::CreatureStats::AT_Chop;
    else
        return MWMechanics::CreatureStats::AT_Thrust;
}

}

namespace MWMechanics
{

struct StateInfo {
    CharacterState state;
    const char groupname[32];
};

static const StateInfo sStateList[] = {
    { CharState_Idle, "idle" },
    { CharState_Idle2, "idle2" },
    { CharState_Idle3, "idle3" },
    { CharState_Idle4, "idle4" },
    { CharState_Idle5, "idle5" },
    { CharState_Idle6, "idle6" },
    { CharState_Idle7, "idle7" },
    { CharState_Idle8, "idle8" },
    { CharState_Idle9, "idle9" },
    { CharState_IdleSwim, "idleswim" },
    { CharState_IdleSneak, "idlesneak" },

    { CharState_Death1, "death1" },
    { CharState_Death2, "death2" },
    { CharState_Death3, "death3" },
    { CharState_Death4, "death4" },
    { CharState_Death5, "death5" },
};
static const StateInfo *sStateListEnd = &sStateList[sizeof(sStateList)/sizeof(sStateList[0])];


static const StateInfo sMovementList[] = {
    { CharState_WalkForward, "walkforward" },
    { CharState_WalkBack, "walkback" },
    { CharState_WalkLeft, "walkleft" },
    { CharState_WalkRight, "walkright" },

    { CharState_SwimWalkForward, "swimwalkforward" },
    { CharState_SwimWalkBack, "swimwalkback" },
    { CharState_SwimWalkLeft, "swimwalkleft" },
    { CharState_SwimWalkRight, "swimwalkright" },

    { CharState_RunForward, "runforward" },
    { CharState_RunBack, "runback" },
    { CharState_RunLeft, "runleft" },
    { CharState_RunRight, "runright" },

    { CharState_SwimRunForward, "swimrunforward" },
    { CharState_SwimRunBack, "swimrunback" },
    { CharState_SwimRunLeft, "swimrunleft" },
    { CharState_SwimRunRight, "swimrunright" },

    { CharState_SneakForward, "sneakforward" },
    { CharState_SneakBack, "sneakback" },
    { CharState_SneakLeft, "sneakleft" },
    { CharState_SneakRight, "sneakright" },

    { CharState_TurnLeft, "turnleft" },
    { CharState_TurnRight, "turnright" },
};
static const StateInfo *sMovementListEnd = &sMovementList[sizeof(sMovementList)/sizeof(sMovementList[0])];


class FindCharState {
    CharacterState state;

public:
    FindCharState(CharacterState _state) : state(_state) { }

    bool operator()(const StateInfo &info) const
    { return info.state == state; }
};


static const struct WeaponInfo {
    WeaponType type;
    const char shortgroup[16];
    const char longgroup[16];
} sWeaponTypeList[] = {
    { WeapType_HandToHand, "hh", "handtohand" },
    { WeapType_OneHand, "1h", "weapononehand" },
    { WeapType_TwoHand, "2c", "weapontwohand" },
    { WeapType_TwoWide, "2w", "weapontwowide" },
    { WeapType_BowAndArrow, "1h", "bowandarrow" },
    { WeapType_Crossbow, "crossbow", "crossbow" },
    { WeapType_ThowWeapon, "1h", "throwweapon" },
    { WeapType_PickProbe, "1h", "pickprobe" },
    { WeapType_Spell, "spell", "spellcast" },
};
static const WeaponInfo *sWeaponTypeListEnd = &sWeaponTypeList[sizeof(sWeaponTypeList)/sizeof(sWeaponTypeList[0])];

class FindWeaponType {
    WeaponType type;

public:
    FindWeaponType(WeaponType _type) : type(_type) { }

    bool operator()(const WeaponInfo &weap) const
    { return weap.type == type; }
};


void CharacterController::refreshCurrentAnims(CharacterState idle, CharacterState movement, bool force)
{
    const WeaponInfo *weap = std::find_if(sWeaponTypeList, sWeaponTypeListEnd, FindWeaponType(mWeaponType));

    if(force || idle != mIdleState)
    {
        mIdleState = idle;

        std::string idle;
        // Only play "idleswim" or "idlesneak" if they exist. Otherwise, fallback to
        // "idle"+weapon or "idle".
        if(mIdleState == CharState_IdleSwim && mAnimation->hasAnimation("idleswim"))
            idle = "idleswim";
        else if(mIdleState == CharState_IdleSneak && mAnimation->hasAnimation("idlesneak"))
            idle = "idlesneak";
        else
        {
            idle = "idle";
            if(weap != sWeaponTypeListEnd)
            {
                idle += weap->shortgroup;
                if(!mAnimation->hasAnimation(idle))
                    idle = "idle";
            }
        }

        mAnimation->disable(mCurrentIdle);
        mCurrentIdle = idle;
        mAnimation->play(mCurrentIdle, Priority_Default, MWRender::Animation::Group_All, false,
                         1.0f, "start", "stop", 0.0f, ~0ul);
    }

    if(force || movement != mMovementState)
    {
        mMovementState = movement;

        std::string movement;
        MWRender::Animation::Group movegroup = MWRender::Animation::Group_All;
        const StateInfo *movestate = std::find_if(sMovementList, sMovementListEnd, FindCharState(mMovementState));
        if(movestate != sMovementListEnd)
        {
            movement = movestate->groupname;
            if(weap != sWeaponTypeListEnd && movement.find("swim") == std::string::npos)
            {
                movement += weap->shortgroup;
                if(!mAnimation->hasAnimation(movement))
                {
                    movegroup = MWRender::Animation::Group_LowerBody;
                    movement = movestate->groupname;
                }
            }

            if(!mAnimation->hasAnimation(movement))
            {
                std::string::size_type sneakpos = movement.find("sneak");
                if(sneakpos == std::string::npos)
                    movement.clear();
                else
                {
                    movegroup = MWRender::Animation::Group_LowerBody;
                    movement.erase(sneakpos, 5);
                    if(!mAnimation->hasAnimation(movement))
                        movement.clear();
                }
            }
        }

        mAnimation->disable(mCurrentMovement);
        mCurrentMovement = movement;
        if(!mCurrentMovement.empty())
        {
            float vel, speed = 0.0f;
            if(mMovementSpeed > 0.0f && (vel=mAnimation->getVelocity(mCurrentMovement)) > 1.0f)
                speed = mMovementSpeed / vel;
            mAnimation->play(mCurrentMovement, Priority_Movement, movegroup, false,
                             speed, "start", "stop", 0.0f, ~0ul);
        }
    }
}


void CharacterController::getWeaponGroup(WeaponType weaptype, std::string &group)
{
    const WeaponInfo *info = std::find_if(sWeaponTypeList, sWeaponTypeListEnd, FindWeaponType(weaptype));
    if(info != sWeaponTypeListEnd)
        group = info->longgroup;
}


CharacterController::CharacterController(const MWWorld::Ptr &ptr, MWRender::Animation *anim)
    : mPtr(ptr)
    , mAnimation(anim)
    , mIdleState(CharState_Idle)
    , mMovementState(CharState_None)
    , mMovementSpeed(0.0f)
    , mDeathState(CharState_None)
    , mUpperBodyState(UpperCharState_Nothing)
    , mWeaponType(WeapType_None)
    , mSkipAnim(false)
    , mSecondsOfRunning(0)
    , mSecondsOfSwimming(0)
    , mUpdateWeapon(true)
{
    if(!mAnimation)
        return;

    if(MWWorld::Class::get(mPtr).isActor())
    {
        /* Accumulate along X/Y only for now, until we can figure out how we should
         * handle knockout and death which moves the character down. */
        mAnimation->setAccumulation(Ogre::Vector3(1.0f, 1.0f, 0.0f));

        if(MWWorld::Class::get(mPtr).getCreatureStats(mPtr).isDead())
        {
            /* FIXME: Get the actual death state used. */
            mDeathState = CharState_Death1;
        }
    }
    else
    {
        /* Don't accumulate with non-actors. */
        mAnimation->setAccumulation(Ogre::Vector3(0.0f));
    }

    refreshCurrentAnims(mIdleState, mMovementState, true);
    if(mDeathState != CharState_None)
    {
        const StateInfo *state = std::find_if(sStateList, sStateListEnd, FindCharState(mDeathState));
        if(state == sStateListEnd)
            throw std::runtime_error("Failed to find character state "+Ogre::StringConverter::toString(mDeathState));

        mCurrentDeath = state->groupname;
        mAnimation->play(mCurrentDeath, Priority_Death, MWRender::Animation::Group_All,
                         false, 1.0f, "start", "stop", 1.0f, 0);
    }
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
    const MWWorld::Class &cls = MWWorld::Class::get(mPtr);

    if(!cls.isActor())
    {
        if(mAnimQueue.size() > 1)
        {
            if(mAnimation->isPlaying(mAnimQueue.front().first) == false)
            {
                mAnimation->disable(mAnimQueue.front().first);
                mAnimQueue.pop_front();

                mAnimation->play(mAnimQueue.front().first, Priority_Default,
                                 MWRender::Animation::Group_All, false,
                                 1.0f, "start", "stop", 0.0f, mAnimQueue.front().second);
            }
        }
    }
    else if(!cls.getCreatureStats(mPtr).isDead())
    {
        MWBase::World *world = MWBase::Environment::get().getWorld();

        bool onground = world->isOnGround(mPtr);
        bool inwater = world->isSwimming(mPtr);
        bool isrunning = cls.getStance(mPtr, MWWorld::Class::Run);
        bool sneak = cls.getStance(mPtr, MWWorld::Class::Sneak);
        Ogre::Vector3 vec = cls.getMovementVector(mPtr);
        Ogre::Vector3 rot = cls.getRotationVector(mPtr);
        mMovementSpeed = cls.getSpeed(mPtr);

        // advance athletics
        if(vec.squaredLength() > 0 && mPtr.getRefData().getHandle() == "player")
        {
            if(inwater)
            {
                mSecondsOfSwimming += duration;
                while(mSecondsOfSwimming > 1)
                {
                    cls.skillUsageSucceeded(mPtr, ESM::Skill::Athletics, 1);
                    mSecondsOfSwimming -= 1;
                }
            }
            else if(isrunning)
            {
                mSecondsOfRunning += duration;
                while(mSecondsOfRunning > 1)
                {
                    cls.skillUsageSucceeded(mPtr, ESM::Skill::Athletics, 0);
                    mSecondsOfRunning -= 1;
                }
            }
        }

        /* FIXME: The state should be set to Jump, and X/Y movement should be disallowed except
         * for the initial thrust (which would be carried by "physics" until landing). */
        if(!onground)
            vec.z = 0.0f;
        else if(vec.z > 0.0f)
        {
            float z = cls.getJump(mPtr);

            if(vec.x == 0 && vec.y == 0)
                vec.z *= z;
            else
            {
                /* FIXME: this would be more correct if we were going into a jumping state,
                 * rather than normal walking/idle states. */
                //Ogre::Vector3 lat = Ogre::Vector3(vec.x, vec.y, 0.0f).normalisedCopy();
                //vec *= Ogre::Vector3(lat.x, lat.y, 1.0f) * z * 0.707f;
                vec.z *= z * 0.707f;
            }

            //decrease fatigue by fFatigueJumpBase + (1 - normalizedEncumbrance) * fFatigueJumpMult;
        }

        vec.x *= mMovementSpeed;
        vec.y *= mMovementSpeed;

        CharacterState movestate = CharState_None;
        CharacterState idlestate = CharState_SpecialIdle;
        bool forcestateupdate = false;

        if(std::abs(vec.x/2.0f) > std::abs(vec.y))
        {
            if(vec.x > 0.0f)
                movestate = (inwater ? (isrunning ? CharState_SwimRunRight : CharState_SwimWalkRight)
                                     : (sneak ? CharState_SneakRight
                                              : (isrunning ? CharState_RunRight : CharState_WalkRight)));
            else if(vec.x < 0.0f)
                movestate = (inwater ? (isrunning ? CharState_SwimRunLeft : CharState_SwimWalkLeft)
                                     : (sneak ? CharState_SneakLeft
                                              : (isrunning ? CharState_RunLeft : CharState_WalkLeft)));
        }
        else if(vec.y != 0.0f)
        {
            if(vec.y > 0.0f)
                movestate = (inwater ? (isrunning ? CharState_SwimRunForward : CharState_SwimWalkForward)
                                     : (sneak ? CharState_SneakForward
                                              : (isrunning ? CharState_RunForward : CharState_WalkForward)));
            else if(vec.y < 0.0f)
                movestate = (inwater ? (isrunning ? CharState_SwimRunBack : CharState_SwimWalkBack)
                                     : (sneak ? CharState_SneakBack
                                              : (isrunning ? CharState_RunBack : CharState_WalkBack)));
        }
        else if(rot.z != 0.0f && !inwater && !sneak)
        {
            if(rot.z > 0.0f)
                movestate = CharState_TurnRight;
            else if(rot.z < 0.0f)
                movestate = CharState_TurnLeft;
        }

        if(movestate != CharState_None)
            clearAnimQueue();

        if(mAnimQueue.size() == 0)
            idlestate = (inwater ? CharState_IdleSwim : (sneak ? CharState_IdleSneak : CharState_Idle));
        else if(mAnimQueue.size() > 1)
        {
            if(mAnimation->isPlaying(mAnimQueue.front().first) == false)
            {
                mAnimation->disable(mAnimQueue.front().first);
                mAnimQueue.pop_front();

                mAnimation->play(mAnimQueue.front().first, Priority_Default,
                                 MWRender::Animation::Group_All, false,
                                 1.0f, "start", "stop", 0.0f, mAnimQueue.front().second);
            }
        }

        vec *= duration;
        movement.mPosition[0] += vec.x;
        movement.mPosition[1] += vec.y;
        movement.mPosition[2] += vec.z;
        rot *= duration;
        movement.mRotation[0] += rot.x;
        movement.mRotation[1] += rot.y;
        movement.mRotation[2] += rot.z;

        if(mPtr.getTypeName() == typeid(ESM::NPC).name())
        {
            NpcStats &stats = cls.getNpcStats(mPtr);
            WeaponType weaptype = WeapType_None;
            MWWorld::InventoryStore &inv = cls.getInventoryStore(mPtr);
            MWWorld::ContainerStoreIterator weapon = inv.end();

            if(stats.getDrawState() == DrawState_Spell)
                weaptype = WeapType_Spell;
            else if(stats.getDrawState() == MWMechanics::DrawState_Weapon)
            {
                weapon = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);
                if(weapon == inv.end())
                    weaptype = WeapType_HandToHand;
                else
                {
                    const std::string &type = weapon->getTypeName();
                    if(type == typeid(ESM::Lockpick).name() || type == typeid(ESM::Probe).name())
                        weaptype = WeapType_PickProbe;
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
            else
                weapon = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);

            if(mUpdateWeapon)
            {
                forcestateupdate = (mWeaponType != weaptype);
                mWeaponType = weaptype;
                mUpdateWeapon = false;
            }

            if(weaptype != mWeaponType)
            {
                forcestateupdate = true;

                std::string weapgroup;
                if(weaptype == WeapType_None)
                {
                    getWeaponGroup(mWeaponType, weapgroup);
                    mAnimation->play(weapgroup, Priority_Weapon,
                                     MWRender::Animation::Group_UpperBody, true,
                                     1.0f, "unequip start", "unequip stop", 0.0f, 0);
                    mUpperBodyState = UpperCharState_UnEquipingWeap;
                }
                else
                {
                    getWeaponGroup(weaptype, weapgroup);
                    mAnimation->showWeapons(false);
                    mAnimation->play(weapgroup, Priority_Weapon,
                                     MWRender::Animation::Group_UpperBody, true,
                                     1.0f, "equip start", "equip stop", 0.0f, 0);
                    mUpperBodyState = UpperCharState_EquipingWeap;
                }

                mWeaponType = weaptype;

                if(weapon != inv.end())
                {
                    std::string soundid = (mWeaponType == WeapType_None) ?
                                          MWWorld::Class::get(*weapon).getDownSoundId(*weapon) :
                                          MWWorld::Class::get(*weapon).getUpSoundId(*weapon);
                    if(!soundid.empty())
                    {
                        MWBase::SoundManager *sndMgr = MWBase::Environment::get().getSoundManager();
                        sndMgr->playSound3D(mPtr, soundid, 1.0f, 1.0f);
                    }
                }
            }

            if(weaptype != WeapType_PickProbe && weaptype != WeapType_BowAndArrow
                && weaptype != WeapType_Crossbow && weaptype != WeapType_ThowWeapon)
            {
                std::string weapgroup;
                getWeaponGroup(mWeaponType, weapgroup);
                float weapSpeed = 1;
                if(weapon != inv.end()) weapSpeed = weapon->get<ESM::Weapon>()->mBase->mData.mSpeed;
                std::string start;
                std::string stop;
                float complete;
                float speedMult;
                bool animPlaying = mAnimation->getInfo(weapgroup,&complete,&speedMult,&start,&stop);

                if(cls.getCreatureStats(mPtr).getAttackingOrSpell())
                {
                    if(mUpperBodyState == UpperCharState_WeapEquiped)
                    {
                        int attackType = cls.getCreatureStats(mPtr).getAttackType();
                        if (Settings::Manager::getBool("best attack", "Game") && weapon != inv.end())
                            attackType = getBestAttack(weapon->get<ESM::Weapon>()->mBase);

                        if (attackType == MWMechanics::CreatureStats::AT_Chop)
                            mAttackType = "chop";
                        else if (attackType == MWMechanics::CreatureStats::AT_Slash)
                            mAttackType = "slash";
                        else
                            mAttackType = "thrust";

                        mAnimation->play(weapgroup, Priority_Weapon,
                            MWRender::Animation::Group_UpperBody, false,
                            weapSpeed, mAttackType+" start", mAttackType+" min attack", 0.0f, 0);
                        mUpperBodyState = UpperCharState_StartToMinAttack;
                    }
                }
                else if(mUpperBodyState == UpperCharState_MinAttackToMaxAttack)
                {
                    if(animPlaying)
                    {
                        mAnimation->disable(weapgroup);
                        mAnimation->play(weapgroup, Priority_Weapon,
                            MWRender::Animation::Group_UpperBody, false,
                            weapSpeed, mAttackType+" max attack", mAttackType+" min hit", 1-complete, 0);
                    }
                    else
                    {
                        mAnimation->play(weapgroup, Priority_Weapon,
                            MWRender::Animation::Group_UpperBody, false,
                            weapSpeed, mAttackType+" max attack", mAttackType+" min hit", 0, 0);
                    }
                    mUpperBodyState = UpperCharState_MaxAttackToMinHit;
                }

                if(mUpperBodyState == UpperCharState_EquipingWeap && !animPlaying) mUpperBodyState = UpperCharState_WeapEquiped;
                if(mUpperBodyState == UpperCharState_UnEquipingWeap && !animPlaying) mUpperBodyState = UpperCharState_Nothing;
                if(animPlaying)
                {
                    if(mUpperBodyState == UpperCharState_StartToMinAttack && complete == 1)
                    {
                        mAnimation->disable(weapgroup);
                        mAnimation->play(weapgroup, Priority_Weapon,
                            MWRender::Animation::Group_UpperBody, false,
                            weapSpeed, mAttackType+" min attack", mAttackType+" max attack",0, 0);
                        mUpperBodyState = UpperCharState_MinAttackToMaxAttack;
                    }
                    else if(mUpperBodyState == UpperCharState_MaxAttackToMinHit && complete == 1)
                    {
                        mAnimation->disable(weapgroup);
                        mAnimation->play(weapgroup, Priority_Weapon,
                            MWRender::Animation::Group_UpperBody, false,
                            weapSpeed, mAttackType+" min hit", mAttackType+" hit",0, 0);
                        mUpperBodyState = UpperCharState_MinHitToHit;
                    }
                    else if(mUpperBodyState == UpperCharState_MinHitToHit && complete == 1)
                    {
                        mAnimation->disable(weapgroup);
                        mAnimation->play(weapgroup, Priority_Weapon,
                            MWRender::Animation::Group_UpperBody, false,
                            weapSpeed, mAttackType+" large follow start", mAttackType+" large follow stop",0, 0);
                        mUpperBodyState = UpperCharState_LargeFollowStartToLargeFollowStop;
                    }
                    else if(mUpperBodyState == UpperCharState_LargeFollowStartToLargeFollowStop && complete == 1)
                    {
                        mUpperBodyState = UpperCharState_WeapEquiped;
                    }
                }
            }

            MWWorld::ContainerStoreIterator torch = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedLeft);
            if(torch != inv.end() && torch->getTypeName() == typeid(ESM::Light).name())
            {
                if(!mAnimation->isPlaying("torch"))
                    mAnimation->play("torch", Priority_Torch,
                                     MWRender::Animation::Group_LeftArm, false,
                                     1.0f, "start", "stop", 0.0f, (~(size_t)0));
            }
            else if(mAnimation->isPlaying("torch"))
                mAnimation->disable("torch");
        }

        refreshCurrentAnims(idlestate, movestate, forcestateupdate);
    }
    else if(cls.getCreatureStats(mPtr).isDead())
    {
        MWBase::Environment::get().getWorld()->enableActorCollision(mPtr, false);
    }

    if(mAnimation && !mSkipAnim)
    {
        Ogre::Vector3 moved = mAnimation->runAnimation(duration);
        // Ensure we're moving in generally the right direction
        if(mMovementSpeed > 0.f)
        {
            if((movement.mPosition[0] < 0.0f && movement.mPosition[0] < moved.x*2.0f) ||
               (movement.mPosition[0] > 0.0f && movement.mPosition[0] > moved.x*2.0f))
                moved.x = movement.mPosition[0];
            if((movement.mPosition[1] < 0.0f && movement.mPosition[1] < moved.y*2.0f) ||
               (movement.mPosition[1] > 0.0f && movement.mPosition[1] > moved.y*2.0f))
                moved.y = movement.mPosition[1];
            if((movement.mPosition[2] < 0.0f && movement.mPosition[2] < moved.z*2.0f) ||
               (movement.mPosition[2] > 0.0f && movement.mPosition[2] > moved.z*2.0f))
                moved.z = movement.mPosition[2];
        }

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
            clearAnimQueue();
            mAnimQueue.push_back(std::make_pair(groupname, count-1));

            mAnimation->disable(mCurrentIdle);
            mCurrentIdle.clear();

            mIdleState = CharState_SpecialIdle;
            mAnimation->play(groupname, Priority_Default,
                             MWRender::Animation::Group_All, false, 1.0f,
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

bool CharacterController::isAnimPlaying(const std::string &groupName)
{
    if(mAnimation == NULL)
        return false;
    return mAnimation->isPlaying(groupName);
}


void CharacterController::clearAnimQueue()
{
    if(mAnimQueue.size() > 0)
        mAnimation->disable(mAnimQueue.front().first);
    mAnimQueue.clear();
}


void CharacterController::forceStateUpdate()
{
    if(!mAnimation)
        return;
    clearAnimQueue();

    refreshCurrentAnims(mIdleState, mMovementState, true);
    if(mDeathState != CharState_None)
    {
        const StateInfo *state = std::find_if(sStateList, sStateListEnd, FindCharState(mDeathState));
        if(state == sStateListEnd)
            throw std::runtime_error("Failed to find character state "+Ogre::StringConverter::toString(mDeathState));

        mCurrentDeath = state->groupname;
        if(!mAnimation->getInfo(mCurrentDeath))
            mAnimation->play(mCurrentDeath, Priority_Death, MWRender::Animation::Group_All,
                             false, 1.0f, "start", "stop", 0.0f, 0);
    }
}

void CharacterController::kill()
{
    static const CharacterState deathstates[] = {
        CharState_Death1, CharState_Death2, CharState_Death3, CharState_Death4, CharState_Death5
    };

    if(mDeathState != CharState_None)
        return;

    mDeathState = deathstates[(int)(rand()/((double)RAND_MAX+1.0)*5.0)];
    const StateInfo *state = std::find_if(sStateList, sStateListEnd, FindCharState(mDeathState));
    if(state == sStateListEnd)
        throw std::runtime_error("Failed to find character state "+Ogre::StringConverter::toString(mDeathState));

    mCurrentDeath = state->groupname;
    if(mAnimation && !mAnimation->getInfo(mCurrentDeath))
        mAnimation->play(mCurrentDeath, Priority_Death, MWRender::Animation::Group_All,
                         false, 1.0f, "start", "stop", 0.0f, 0);
}

void CharacterController::resurrect()
{
    if(mDeathState == CharState_None)
        return;

    if(mAnimation)
        mAnimation->disable(mCurrentDeath);
    mCurrentDeath.empty();
    mDeathState = CharState_None;
}


}
