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
#include "security.hpp"

#include "../mwrender/animation.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/windowmanager.hpp"

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

static const std::string sDeathList[] = {
    "death1" ,
    "death2" ,
    "death3" ,
    "death4" ,
    "death5" ,
    "swimdeath",
};
static const int sDeathListSize = sizeof(sDeathList)/sizeof(sDeathList[0]);

static const std::string sHitList[] = {
    "hit1" ,
    "hit2" ,
    "hit3" ,
    "hit4" ,
    "hit5" ,
    "knockdown" ,
};
static const int sHitListSize = sizeof(sHitList)/sizeof(sHitList[0]);

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

    { CharState_Jump, "jump" },

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
    // hit recoils/knockdown animations handling
    if(mPtr.getClass().isActor())
    {
        bool recovery = mPtr.getClass().getCreatureStats(mPtr).getHitRecovery();
        bool knockdown = mPtr.getClass().getCreatureStats(mPtr).getKnockedDown();
        if(mHitState == CharState_None)
        {
            if(knockdown)
            {
                mHitState = CharState_KnockDown;
                mCurrentHit = sHitList[sHitListSize-1];
                mAnimation->play(mCurrentHit, Priority_Knockdown, MWRender::Animation::Group_All, true, 1, "start", "stop", 0.0f, 0);
            }
            else if (recovery)
            {
                mHitState = CharState_Hit;
                int iHit = rand() % (sHitListSize-1);
                mCurrentHit = sHitList[iHit];
                if(mPtr.getRefData().getHandle()=="player" && !mAnimation->hasAnimation(mCurrentHit))
                {
                    //only 3 different hit animations if player is in 1st person
                    int iHit = rand() % (sHitListSize-3);
                    mCurrentHit = sHitList[iHit];
                }
                mAnimation->play(mCurrentHit, Priority_Hit, MWRender::Animation::Group_All, true, 1, "start", "stop", 0.0f, 0);
            }
        }
        else if(!mAnimation->isPlaying(mCurrentHit))
        {
            mCurrentHit.erase();
            if (knockdown)
                mPtr.getClass().getCreatureStats(mPtr).setKnockedDown(false);
            if (recovery)
                mPtr.getClass().getCreatureStats(mPtr).setHitRecovery(false);
            mHitState = CharState_None;
        }
    }

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
        else if(mIdleState != CharState_None)
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
        if(!mCurrentIdle.empty())
            mAnimation->play(mCurrentIdle, Priority_Default, MWRender::Animation::Group_All, false,
                             1.0f, "start", "stop", 0.0f, ~0ul);
    }

    if(force && mJumpState != JumpState_None)
    {
        std::string jump;
        MWRender::Animation::Group jumpgroup = MWRender::Animation::Group_All;
        if(mJumpState != JumpState_None)
        {
            jump = "jump";
            if(weap != sWeaponTypeListEnd)
            {
                jump += weap->shortgroup;
                if(!mAnimation->hasAnimation(jump))
                {
                    jumpgroup = MWRender::Animation::Group_LowerBody;
                    jump = "jump";
                }
            }
        }

        if(mJumpState == JumpState_Falling)
        {
            int mode = ((jump == mCurrentJump) ? 2 : 1);

            mAnimation->disable(mCurrentJump);
            mCurrentJump = jump;
            mAnimation->play(mCurrentJump, Priority_Jump, jumpgroup, false,
                             1.0f, ((mode!=2)?"start":"loop start"), "stop", 0.0f, ~0ul);
        }
        else
        {
            mAnimation->disable(mCurrentJump);
            mCurrentJump.clear();
            mAnimation->play(jump, Priority_Jump, jumpgroup, true,
                             1.0f, "loop stop", "stop", 0.0f, 0);
        }
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
                std::string::size_type swimpos = movement.find("swim");
                if(swimpos == std::string::npos)
                    movement.clear();
                else
                {
                    movegroup = MWRender::Animation::Group_LowerBody;
                    movement.erase(swimpos, 4);
                    if(!mAnimation->hasAnimation(movement))
                        movement.clear();
                }
            }
        }

        /* If we're playing the same animation, restart from the loop start instead of the
         * beginning. */
        int mode = ((movement == mCurrentMovement) ? 2 : 1);

        mAnimation->disable(mCurrentMovement);
        mCurrentMovement = movement;
        if(!mCurrentMovement.empty())
        {
            float vel, speedmult = 1.0f;
            if(mMovementSpeed > 0.0f && (vel=mAnimation->getVelocity(mCurrentMovement)) > 1.0f)
                speedmult = mMovementSpeed / vel;
            mAnimation->play(mCurrentMovement, Priority_Movement, movegroup, false,
                             speedmult, ((mode!=2)?"start":"loop start"), "stop", 0.0f, ~0ul);
        }
    }
}


void CharacterController::getWeaponGroup(WeaponType weaptype, std::string &group)
{
    const WeaponInfo *info = std::find_if(sWeaponTypeList, sWeaponTypeListEnd, FindWeaponType(weaptype));
    if(info != sWeaponTypeListEnd)
        group = info->longgroup;
}


MWWorld::ContainerStoreIterator CharacterController::getActiveWeapon(NpcStats &stats, MWWorld::InventoryStore &inv, WeaponType *weaptype)
{
    if(stats.getDrawState() == DrawState_Spell)
    {
        *weaptype = WeapType_Spell;
        return inv.end();
    }

    if(stats.getDrawState() == MWMechanics::DrawState_Weapon)
    {
        MWWorld::ContainerStoreIterator weapon = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);
        if(weapon == inv.end())
            *weaptype = WeapType_HandToHand;
        else
        {
            const std::string &type = weapon->getTypeName();
            if(type == typeid(ESM::Lockpick).name() || type == typeid(ESM::Probe).name())
                *weaptype = WeapType_PickProbe;
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
                        *weaptype = WeapType_OneHand;
                        break;
                    case ESM::Weapon::LongBladeTwoHand:
                    case ESM::Weapon::BluntTwoClose:
                    case ESM::Weapon::AxeTwoHand:
                        *weaptype = WeapType_TwoHand;
                        break;
                    case ESM::Weapon::BluntTwoWide:
                    case ESM::Weapon::SpearTwoWide:
                        *weaptype = WeapType_TwoWide;
                        break;
                    case ESM::Weapon::MarksmanBow:
                        *weaptype = WeapType_BowAndArrow;
                        break;
                    case ESM::Weapon::MarksmanCrossbow:
                        *weaptype = WeapType_Crossbow;
                        break;
                    case ESM::Weapon::MarksmanThrown:
                        *weaptype = WeapType_ThowWeapon;
                        break;
                }
            }
        }

        return weapon;
    }

    return inv.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);
}

void CharacterController::playRandomDeath(float startpoint)
{
    if(MWWorld::Class::get(mPtr).isNpc())
    {
        if(MWBase::Environment::get().getWorld()->isSwimming(mPtr))
        {
            mDeathState = CharState_SwimDeath;
            mCurrentDeath = sDeathList[sDeathListSize-1];   //last in the list is 'swimdeath'
        }
        else
        {
            int num = rand() % (sDeathListSize-1);
            mDeathState = static_cast<CharacterState>(CharState_Death1 + num);
            mCurrentDeath = sDeathList[num];
        }
    }
    else
    {
        mDeathState = CharState_Death1;
        mCurrentDeath = "death1";
    }

    mAnimation->play(mCurrentDeath, Priority_Death, MWRender::Animation::Group_All,
                    false, 1.0f, "start", "stop", 0.0f, 0);
}

CharacterController::CharacterController(const MWWorld::Ptr &ptr, MWRender::Animation *anim)
    : mPtr(ptr)
    , mAnimation(anim)
    , mIdleState(CharState_None)
    , mMovementState(CharState_None)
    , mMovementSpeed(0.0f)
    , mDeathState(CharState_None)
    , mHitState(CharState_None)
    , mUpperBodyState(UpperCharState_Nothing)
    , mJumpState(JumpState_None)
    , mWeaponType(WeapType_None)
    , mSkipAnim(false)
    , mSecondsOfRunning(0)
    , mSecondsOfSwimming(0)
{
    if(!mAnimation)
        return;

    const MWWorld::Class &cls = MWWorld::Class::get(mPtr);
    if(cls.isActor())
    {
        /* Accumulate along X/Y only for now, until we can figure out how we should
         * handle knockout and death which moves the character down. */
        mAnimation->setAccumulation(Ogre::Vector3(1.0f, 1.0f, 0.0f));

        if(mPtr.getTypeName() == typeid(ESM::NPC).name())
        {
            getActiveWeapon(cls.getNpcStats(mPtr), cls.getInventoryStore(mPtr), &mWeaponType);
            if(mWeaponType != WeapType_None)
            {
                getWeaponGroup(mWeaponType, mCurrentWeapon);
                mUpperBodyState = UpperCharState_WeapEquiped;
                mAnimation->showWeapons(true);
            }
        }

        if(!cls.getCreatureStats(mPtr).isDead())
            mIdleState = CharState_Idle;
        else
        {
            /* FIXME: Get the actual death state used. */
            mDeathState = CharState_Death1;
        }
    }
    else
    {
        /* Don't accumulate with non-actors. */
        mAnimation->setAccumulation(Ogre::Vector3(0.0f));

        mIdleState = CharState_Idle;
    }

    refreshCurrentAnims(mIdleState, mMovementState, true);

    if(mDeathState != CharState_None)
    {
        playRandomDeath(1.0f);
    }
}

CharacterController::~CharacterController()
{
}


void CharacterController::updatePtr(const MWWorld::Ptr &ptr)
{
    mPtr = ptr;
}

bool CharacterController::updateNpcState(bool onground, bool inwater, bool isrunning, bool sneak)
{
   const MWWorld::Class &cls = MWWorld::Class::get(mPtr);
    NpcStats &stats = cls.getNpcStats(mPtr);
    WeaponType weaptype = WeapType_None;
    MWWorld::InventoryStore &inv = cls.getInventoryStore(mPtr);
    MWWorld::ContainerStoreIterator weapon = getActiveWeapon(stats, inv, &weaptype);
    const bool isWerewolf = stats.isWerewolf();

    bool forcestateupdate = false;
    if(weaptype != mWeaponType && mHitState != CharState_KnockDown)
    {
        forcestateupdate = true;

        // Shields/torches shouldn't be visible during spellcasting or hand-to-hand
        // There seems to be no text keys for this purpose, except maybe for "[un]equip start/stop",
        // but they are also present in weapon drawing animation.
        mAnimation->showCarriedLeft(weaptype != WeapType_Spell && weaptype != WeapType_HandToHand);

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

            if(isWerewolf)
            {
                const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
                const ESM::Sound *sound = store.get<ESM::Sound>().searchRandom("WolfEquip");
                if(sound)
                {
                    MWBase::SoundManager *sndMgr = MWBase::Environment::get().getSoundManager();
                    sndMgr->playSound3D(mPtr, sound->mId, 1.0f, 1.0f);
                }
            }
        }

        if(weapon != inv.end() && !(weaptype == WeapType_None && mWeaponType == WeapType_Spell))
        {
            std::string soundid = (weaptype == WeapType_None) ?
                                   MWWorld::Class::get(*weapon).getDownSoundId(*weapon) :
                                   MWWorld::Class::get(*weapon).getUpSoundId(*weapon);
            if(!soundid.empty())
            {
                MWBase::SoundManager *sndMgr = MWBase::Environment::get().getSoundManager();
                sndMgr->playSound3D(mPtr, soundid, 1.0f, 1.0f);
            }
        }

        mWeaponType = weaptype;
        getWeaponGroup(mWeaponType, mCurrentWeapon);
    }

    if(isWerewolf)
    {
        MWBase::SoundManager *sndMgr = MWBase::Environment::get().getSoundManager();
        if(isrunning && !inwater && mWeaponType == WeapType_None)
        {
            if(!sndMgr->getSoundPlaying(mPtr, "WolfRun"))
                sndMgr->playSound3D(mPtr, "WolfRun", 1.0f, 1.0f, MWBase::SoundManager::Play_TypeSfx,
                                    MWBase::SoundManager::Play_Loop);
        }
        else
            sndMgr->stopSound3D(mPtr, "WolfRun");
    }

    bool isWeapon = (weapon != inv.end() && weapon->getTypeName() == typeid(ESM::Weapon).name());
    float weapSpeed = 1.0f;
    if(isWeapon)
        weapSpeed = weapon->get<ESM::Weapon>()->mBase->mData.mSpeed;

    float complete;
    bool animPlaying;
    if(stats.getAttackingOrSpell())
    {
        if(mUpperBodyState == UpperCharState_WeapEquiped && mHitState == CharState_None)
        {
            MWBase::Environment::get().getWorld()->breakInvisibility(mPtr);
            mAttackType.clear();
            if(mWeaponType == WeapType_Spell)
            {
                // Unset casting flag, otherwise pressing the mouse button down would
                // continue casting every frame if there is no animation
                mPtr.getClass().getCreatureStats(mPtr).setAttackingOrSpell(false);

                const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();

                // For the player, set the spell we want to cast
                // This has to be done at the start of the casting animation,
                // *not* when selecting a spell in the GUI (otherwise you could change the spell mid-animation)
                if (mPtr.getRefData().getHandle() == "player")
                    stats.getSpells().setSelectedSpell(MWBase::Environment::get().getWindowManager()->getSelectedSpell());

                std::string spellid = stats.getSpells().getSelectedSpell();

                if(!spellid.empty() && MWBase::Environment::get().getWorld()->startSpellCast(mPtr))
                {
                    static const std::string schools[] = {
                        "alteration", "conjuration", "destruction", "illusion", "mysticism", "restoration"
                    };

                    const ESM::Spell *spell = store.get<ESM::Spell>().find(spellid);
                    const ESM::ENAMstruct &effectentry = spell->mEffects.mList.at(0);

                    const ESM::MagicEffect *effect;
                    effect = store.get<ESM::MagicEffect>().find(effectentry.mEffectID);

                    const ESM::Static* castStatic;
                    if (!effect->mCasting.empty())
                        castStatic = store.get<ESM::Static>().find (effect->mCasting);
                    else
                        castStatic = store.get<ESM::Static>().find ("VFX_DefaultCast");

                    mAnimation->addEffect("meshes\\" + castStatic->mModel, effect->mIndex);

                    castStatic = MWBase::Environment::get().getWorld()->getStore().get<ESM::Static>().find ("VFX_Hands");
                    //mAnimation->addEffect("meshes\\" + castStatic->mModel, -1, false, "Bip01 L Hand", effect->mParticle);
                    //mAnimation->addEffect("meshes\\" + castStatic->mModel, -1, false, "Bip01 R Hand", effect->mParticle);
                    mAnimation->addEffect("meshes\\" + castStatic->mModel, -1, false, "Left Hand", effect->mParticle);
                    mAnimation->addEffect("meshes\\" + castStatic->mModel, -1, false, "Right Hand", effect->mParticle);

                    switch(effectentry.mRange)
                    {
                        case 0: mAttackType = "self"; break;
                        case 1: mAttackType = "touch"; break;
                        case 2: mAttackType = "target"; break;
                    }

                    mAnimation->play(mCurrentWeapon, Priority_Weapon,
                                     MWRender::Animation::Group_UpperBody, true,
                                     weapSpeed, mAttackType+" start", mAttackType+" stop",
                                     0.0f, 0);
                    mUpperBodyState = UpperCharState_CastingSpell;

                    MWBase::SoundManager *sndMgr = MWBase::Environment::get().getSoundManager();
                    if(!effect->mCastSound.empty())
                        sndMgr->playSound3D(mPtr, effect->mCastSound, 1.0f, 1.0f);
                    else
                        sndMgr->playSound3D(mPtr, schools[effect->mData.mSchool]+" cast", 1.0f, 1.0f);
                }
                if (inv.getSelectedEnchantItem() != inv.end())
                {
                    // Enchanted items cast immediately (no animation)
                    MWBase::Environment::get().getWorld()->castSpell(mPtr);
                }
            }
            else if(mWeaponType == WeapType_PickProbe)
            {
                MWWorld::Ptr item = *weapon;
                MWWorld::Ptr target = MWBase::Environment::get().getWorld()->getFacedObject();
                std::string resultMessage, resultSound;

                if(!target.isEmpty())
                {
                    if(item.getTypeName() == typeid(ESM::Lockpick).name())
                        Security(mPtr).pickLock(target, item, resultMessage, resultSound);
                    else if(item.getTypeName() == typeid(ESM::Probe).name())
                        Security(mPtr).probeTrap(target, item, resultMessage, resultSound);
                }
                mAnimation->play(mCurrentWeapon, Priority_Weapon,
                                 MWRender::Animation::Group_UpperBody, true,
                                 1.0f, "start", "stop", 0.0, 0);
                mUpperBodyState = UpperCharState_FollowStartToFollowStop;

                if(!resultMessage.empty())
                    MWBase::Environment::get().getWindowManager()->messageBox(resultMessage);
                if(!resultSound.empty())
                    MWBase::Environment::get().getSoundManager()->playSound(resultSound, 1.0f, 1.0f);

                // Set again, just to update the charge bar
                if(item.getRefData().getCount())
                    MWBase::Environment::get().getWindowManager()->setSelectedWeapon(item);
            }
            else
            {
                if(mWeaponType == WeapType_Crossbow || mWeaponType == WeapType_BowAndArrow ||
                   mWeaponType == WeapType_ThowWeapon)
                    mAttackType = "shoot";
                else
                {
                    int attackType = stats.getAttackType();
                    if(isWeapon && Settings::Manager::getBool("best attack", "Game"))
                        attackType = getBestAttack(weapon->get<ESM::Weapon>()->mBase);

                    if (attackType == MWMechanics::CreatureStats::AT_Chop)
                        mAttackType = "chop";
                    else if (attackType == MWMechanics::CreatureStats::AT_Slash)
                        mAttackType = "slash";
                    else
                        mAttackType = "thrust";
                }

                mAnimation->play(mCurrentWeapon, Priority_Weapon,
                                 MWRender::Animation::Group_UpperBody, false,
                                 weapSpeed, mAttackType+" start", mAttackType+" min attack",
                                 0.0f, 0);
                mUpperBodyState = UpperCharState_StartToMinAttack;   
            }
        }

        animPlaying = mAnimation->getInfo(mCurrentWeapon, &complete);
    }
    else
    {
        animPlaying = mAnimation->getInfo(mCurrentWeapon, &complete);
        if(mUpperBodyState == UpperCharState_MinAttackToMaxAttack && mHitState != CharState_KnockDown)
        {
            if(mAttackType != "shoot")
            {
                MWBase::SoundManager *sndMgr = MWBase::Environment::get().getSoundManager();

                if(isWerewolf)
                {
                    const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
                    const ESM::Sound *sound = store.get<ESM::Sound>().searchRandom("WolfSwing");
                    if(sound)
                        sndMgr->playSound3D(mPtr, sound->mId, 1.0f, 1.0f);
                }
                else
                {
                    std::string sound = "SwishM";
                    if(complete < 0.5f)
                        sndMgr->playSound3D(mPtr, sound, 1.0f, 0.8f); //Weak attack
                    else if(complete < 1.0f)
                        sndMgr->playSound3D(mPtr, sound, 1.0f, 1.0f); //Medium attack
                    else
                        sndMgr->playSound3D(mPtr, sound, 1.0f, 1.2f); //Strong attack
                }
            }
            stats.setAttackStrength(complete);

            mAnimation->disable(mCurrentWeapon);
            mAnimation->play(mCurrentWeapon, Priority_Weapon,
                             MWRender::Animation::Group_UpperBody, false,
                             weapSpeed, mAttackType+" max attack", mAttackType+" min hit",
                             1.0f-complete, 0);
            mUpperBodyState = UpperCharState_MaxAttackToMinHit;
        }
        else if (mHitState == CharState_KnockDown)
        {
            mUpperBodyState = UpperCharState_WeapEquiped;
            mAnimation->disable(mCurrentWeapon);
        }
    }

    if(!animPlaying)
    {
        if(mUpperBodyState == UpperCharState_EquipingWeap ||
           mUpperBodyState == UpperCharState_FollowStartToFollowStop ||
           mUpperBodyState == UpperCharState_CastingSpell)
        {
            mUpperBodyState = UpperCharState_WeapEquiped;
            //don't allow to continue playing hit animation on UpperBody after actor had attacked during it
            if(mHitState == CharState_Hit) 
            {
                mAnimation->changeGroups(mCurrentHit, MWRender::Animation::Group_LowerBody);
                //commenting out following 2 lines will give a bit different combat dynamics(slower)
                mHitState = CharState_None;
                mCurrentHit.clear();
            }
        }
        else if(mUpperBodyState == UpperCharState_UnEquipingWeap)
            mUpperBodyState = UpperCharState_Nothing;
    }
    else if(complete >= 1.0f)
    {
        std::string start, stop;
        switch(mUpperBodyState)
        {
            case UpperCharState_StartToMinAttack:
                start = mAttackType+" min attack";
                stop = mAttackType+" max attack";
                mUpperBodyState = UpperCharState_MinAttackToMaxAttack;
                break;
            case UpperCharState_MaxAttackToMinHit:
                if(mAttackType == "shoot")
                {
                    start = mAttackType+" min hit";
                    stop = mAttackType+" release";
                }
                else
                {
                    start = mAttackType+" min hit";
                    stop = mAttackType+" hit";
                }
                mUpperBodyState = UpperCharState_MinHitToHit;
                break;
            case UpperCharState_MinHitToHit:
                if(mAttackType == "shoot")
                {
                    start = mAttackType+" follow start";
                    stop = mAttackType+" follow stop";
                }
                else
                {
                    float str = stats.getAttackStrength();
                    start = mAttackType+((str < 0.5f) ? " small follow start"
                                                                  : (str < 1.0f) ? " medium follow start"
                                                                                 : " large follow start");
                    stop = mAttackType+((str < 0.5f) ? " small follow stop"
                                                                 : (str < 1.0f) ? " medium follow stop"
                                                                                : " large follow stop");
                }
                mUpperBodyState = UpperCharState_FollowStartToFollowStop;
                break;
            default:
                break;
        }

        if(!start.empty())
        {
            mAnimation->disable(mCurrentWeapon);
            if (mUpperBodyState == UpperCharState_FollowStartToFollowStop)
                mAnimation->play(mCurrentWeapon, Priority_Weapon,
                                 MWRender::Animation::Group_UpperBody, true,
                                 weapSpeed, start, stop, 0.0f, 0);
            else
                mAnimation->play(mCurrentWeapon, Priority_Weapon,
                                 MWRender::Animation::Group_UpperBody, false,
                                 weapSpeed, start, stop, 0.0f, 0);
        }
    }
    

    MWWorld::ContainerStoreIterator torch = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedLeft);
    if(torch != inv.end() && torch->getTypeName() == typeid(ESM::Light).name()
            && mWeaponType != WeapType_Spell && mWeaponType != WeapType_HandToHand)

    {
        mAnimation->play("torch", Priority_Torch, MWRender::Animation::Group_LeftArm,
            false, 1.0f, "start", "stop", 0.0f, (~(size_t)0));
    }
    else if (mAnimation->isPlaying("torch"))
    {
        mAnimation->disable("torch");
    }

    return forcestateupdate;
}

void CharacterController::update(float duration)
{
    MWBase::World *world = MWBase::Environment::get().getWorld();
    const MWWorld::Class &cls = MWWorld::Class::get(mPtr);
    Ogre::Vector3 movement(0.0f);

    updateVisibility();

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
        bool onground = world->isOnGround(mPtr);
        bool inwater = world->isSwimming(mPtr);
        bool isrunning = cls.getStance(mPtr, MWWorld::Class::Run);
        bool sneak = cls.getStance(mPtr, MWWorld::Class::Sneak);
        bool flying = world->isFlying(mPtr);
        Ogre::Vector3 vec = cls.getMovementVector(mPtr);
        vec.normalise();
        if(mHitState != CharState_None && mJumpState == JumpState_None)
            vec = Ogre::Vector3(0.0f);
        Ogre::Vector3 rot = cls.getRotationVector(mPtr);
        mMovementSpeed = cls.getSpeed(mPtr);

        vec.x *= mMovementSpeed;
        vec.y *= mMovementSpeed;

        CharacterState movestate = CharState_None;
        CharacterState idlestate = CharState_SpecialIdle;
        bool forcestateupdate = false;

        isrunning = isrunning && std::abs(vec[0])+std::abs(vec[1]) > 0.0f;


        // advance athletics
        if(std::abs(vec[0])+std::abs(vec[1]) > 0.0f && mPtr.getRefData().getHandle() == "player")
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

        // reduce fatigue
        const MWWorld::Store<ESM::GameSetting> &gmst = world->getStore().get<ESM::GameSetting>();
        float fatigueLoss = 0;
        static const float fFatigueRunBase = gmst.find("fFatigueRunBase")->getFloat();
        static const float fFatigueRunMult = gmst.find("fFatigueRunMult")->getFloat();
        static const float fFatigueSwimWalkBase = gmst.find("fFatigueSwimWalkBase")->getFloat();
        static const float fFatigueSwimRunBase = gmst.find("fFatigueSwimRunBase")->getFloat();
        static const float fFatigueSwimWalkMult = gmst.find("fFatigueSwimWalkMult")->getFloat();
        static const float fFatigueSwimRunMult = gmst.find("fFatigueSwimRunMult")->getFloat();
        static const float fFatigueSneakBase = gmst.find("fFatigueSneakBase")->getFloat();
        static const float fFatigueSneakMult = gmst.find("fFatigueSneakMult")->getFloat();

        const float encumbrance = cls.getEncumbrance(mPtr) / cls.getCapacity(mPtr);
        if (encumbrance < 1)
        {
            if (sneak)
                fatigueLoss = fFatigueSneakBase + encumbrance * fFatigueSneakMult;
            else
            {
                if (inwater)
                {
                    if (!isrunning)
                        fatigueLoss = fFatigueSwimWalkBase + encumbrance * fFatigueSwimWalkMult;
                    else
                        fatigueLoss = fFatigueSwimRunBase + encumbrance * fFatigueSwimRunMult;
                }
                if (isrunning)
                    fatigueLoss = fFatigueRunBase + encumbrance * fFatigueRunMult;
            }
        }
        fatigueLoss *= duration;
        DynamicStat<float> fatigue = cls.getCreatureStats(mPtr).getFatigue();
        fatigue.setCurrent(fatigue.getCurrent() - fatigueLoss, fatigue.getCurrent() < 0);
        cls.getCreatureStats(mPtr).setFatigue(fatigue);

        if(sneak || inwater || flying)
            vec.z = 0.0f;

        if (inwater || flying)
            cls.getCreatureStats(mPtr).land();

        if(!onground && !flying && !inwater)
        {
            // In the air (either getting up —ascending part of jump— or falling).

            if (world->isSlowFalling(mPtr))
            {
                // SlowFalling spell effect is active, do not keep previous fall height
                cls.getCreatureStats(mPtr).land();
            }

            forcestateupdate = (mJumpState != JumpState_Falling);
            mJumpState = JumpState_Falling;

            // This is a guess. All that seems to be known is that "While the player is in the
            // air, fJumpMoveBase and fJumpMoveMult governs air control." Assuming Acrobatics
            // plays a role, this makes the most sense.
            float mult = 0.0f;
            if(cls.isNpc())
            {
                const NpcStats &stats = cls.getNpcStats(mPtr);
                mult = gmst.find("fJumpMoveBase")->getFloat() +
                       (stats.getSkill(ESM::Skill::Acrobatics).getModified()/100.0f *
                        gmst.find("fJumpMoveMult")->getFloat());
            }

            vec.x *= mult;
            vec.y *= mult;
            vec.z  = 0.0f;
        }
        else if(vec.z > 0.0f && mJumpState == JumpState_None)
        {
            // Started a jump.
            float z = cls.getJump(mPtr);
            if(vec.x == 0 && vec.y == 0)
                vec = Ogre::Vector3(0.0f, 0.0f, z);
            else
            {
                Ogre::Vector3 lat = Ogre::Vector3(vec.x, vec.y, 0.0f).normalisedCopy();
                vec = Ogre::Vector3(lat.x, lat.y, 1.0f) * z * 0.707f;
            }

            // advance acrobatics
            if (mPtr.getRefData().getHandle() == "player")
                cls.skillUsageSucceeded(mPtr, ESM::Skill::Acrobatics, 0);

            // decrease fatigue
            const MWWorld::Store<ESM::GameSetting> &gmst = world->getStore().get<ESM::GameSetting>();
            const float fatigueJumpBase = gmst.find("fFatigueJumpBase")->getFloat();
            const float fatigueJumpMult = gmst.find("fFatigueJumpMult")->getFloat();
            const float normalizedEncumbrance = cls.getEncumbrance(mPtr) / cls.getCapacity(mPtr);
            const int fatigueDecrease = fatigueJumpBase + (1 - normalizedEncumbrance) * fatigueJumpMult;
            DynamicStat<float> fatigue = cls.getCreatureStats(mPtr).getFatigue();
            fatigue.setCurrent(fatigue.getCurrent() - fatigueDecrease);
            cls.getCreatureStats(mPtr).setFatigue(fatigue);
        }
        else if(mJumpState == JumpState_Falling)
        {
            forcestateupdate = true;
            mJumpState = JumpState_Landing;
            vec.z = 0.0f;

            float height = cls.getCreatureStats(mPtr).land();
            float healthLost = cls.getFallDamage(mPtr, height);
            if (healthLost > 0.0f)
            {
                const float fatigueTerm = cls.getCreatureStats(mPtr).getFatigueTerm();

                // inflict fall damages
                DynamicStat<float> health = cls.getCreatureStats(mPtr).getHealth();
                int realHealthLost = healthLost * (1.0f - 0.25 * fatigueTerm);
                health.setCurrent(health.getCurrent() - realHealthLost);
                cls.getCreatureStats(mPtr).setHealth(health);
                cls.onHit(mPtr, realHealthLost, true, MWWorld::Ptr(), MWWorld::Ptr(), true);

                // report acrobatics progression
                if (mPtr.getRefData().getHandle() == "player")
                    cls.skillUsageSucceeded(mPtr, ESM::Skill::Acrobatics, 1);

                const float acrobaticsSkill = cls.getNpcStats(mPtr).getSkill(ESM::Skill::Acrobatics).getModified();
                if (healthLost > (acrobaticsSkill * fatigueTerm))
                {
                    //TODO: actor falls over
                }
            }
        }
        else
        {
           if(!(vec.z > 0.0f))
                mJumpState = JumpState_None;
            vec.z = 0.0f;

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
        }

        if (onground)
            cls.getCreatureStats(mPtr).land();

        if(movestate != CharState_None)
            clearAnimQueue();

        if(mAnimQueue.empty())
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

        if(cls.isNpc())
            forcestateupdate = updateNpcState(onground, inwater, isrunning, sneak) || forcestateupdate;

        refreshCurrentAnims(idlestate, movestate, forcestateupdate);

        if (!mSkipAnim)
        {
            if(mHitState != CharState_KnockDown)
            {
                rot *= duration * Ogre::Math::RadiansToDegrees(1.0f);
                world->rotateObject(mPtr, rot.x, rot.y, rot.z, true);
            }
            else //avoid z-rotating for knockdown
                world->rotateObject(mPtr, rot.x, rot.y, 0.0f, true);

            world->queueMovement(mPtr, vec);
        }

        movement = vec;
    }
    else if(cls.getCreatureStats(mPtr).isDead())
    {
        MWBase::Environment::get().getWorld()->enableActorCollision(mPtr, false);
        world->queueMovement(mPtr, Ogre::Vector3(0.0f));
    }

    if(mAnimation && !mSkipAnim)
    {
        Ogre::Vector3 moved = mAnimation->runAnimation(duration);
        if(duration > 0.0f)
            moved /= duration;
        else
            moved = Ogre::Vector3(0.0f);

        // Ensure we're moving in generally the right direction...
        if(mMovementSpeed > 0.f)
        {
            float l = moved.length();

            if((movement.x < 0.0f && movement.x < moved.x*2.0f) ||
               (movement.x > 0.0f && movement.x > moved.x*2.0f))
                moved.x = movement.x;
            if((movement.y < 0.0f && movement.y < moved.y*2.0f) ||
               (movement.y > 0.0f && movement.y > moved.y*2.0f))
                moved.y = movement.y;
            if((movement.z < 0.0f && movement.z < moved.z*2.0f) ||
               (movement.z > 0.0f && movement.z > moved.z*2.0f))
                moved.z = movement.z;
            // but keep the original speed
            float newLength = moved.length();
            if (newLength > 0)
                moved *= (l / newLength);
        }

        // Update movement
        if(moved.squaredLength() > 1.0f)
            world->queueMovement(mPtr, moved);
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
        if(mode != 0 || mAnimQueue.empty() || !isAnimPlaying(mAnimQueue.front().first))
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
    if(!mAnimQueue.empty())
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
        playRandomDeath();
    }
}

void CharacterController::kill()
{
    if(mDeathState != CharState_None)
        return;

    playRandomDeath();

    if(mAnimation)
    {
        mAnimation->disable(mCurrentIdle);
    }

    mIdleState = CharState_None;
    mCurrentIdle.clear();
}

void CharacterController::resurrect()
{
    if(mDeathState == CharState_None)
        return;

    if(mAnimation)
        mAnimation->disable(mCurrentDeath);
    mCurrentDeath.clear();
    mDeathState = CharState_None;
}

void CharacterController::updateContinuousVfx()
{
    // Keeping track of when to stop a continuous VFX seems to be very difficult to do inside the spells code,
    // as it's extremely spread out (ActiveSpells, Spells, InventoryStore effects, etc...) so we do it here.

    // Stop any effects that are no longer active
    std::vector<int> effects;
    mAnimation->getLoopingEffects(effects);

    for (std::vector<int>::iterator it = effects.begin(); it != effects.end(); ++it)
    {
        if (mPtr.getClass().getCreatureStats(mPtr).isDead()
            || mPtr.getClass().getCreatureStats(mPtr).getMagicEffects().get(MWMechanics::EffectKey(*it)).mMagnitude <= 0)
            mAnimation->removeEffect(*it);
    }
}

void CharacterController::updateVisibility()
{
    if (!mPtr.getClass().isActor())
        return;
    float alpha = 1.f;
    if (mPtr.getClass().getCreatureStats(mPtr).getMagicEffects().get(ESM::MagicEffect::Invisibility).mMagnitude)
    {
        if (mPtr.getRefData().getHandle() == "player")
            alpha = 0.4f;
        else
            alpha = 0.f;
    }
    float chameleon = mPtr.getClass().getCreatureStats(mPtr).getMagicEffects().get(ESM::MagicEffect::Chameleon).mMagnitude;
    if (chameleon)
    {
        alpha *= std::max(0.2f, (100.f - chameleon)/100.f);
    }

    mAnimation->setAlpha(alpha);
}

}
