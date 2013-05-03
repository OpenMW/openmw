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

#include "../mwrender/animation.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/player.hpp"
#include "../mwworld/class.hpp"


namespace MWMechanics
{

static const struct {
    CharacterState state;
    const char groupname[32];
} sStateList[] = {
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

    { CharState_Jump, "jump" },

    { CharState_Death1, "death1" },
    { CharState_Death2, "death2" },
    { CharState_Death3, "death3" },
    { CharState_Death4, "death4" },
    { CharState_Death5, "death5" },
};
static const size_t sStateListSize = sizeof(sStateList)/sizeof(sStateList[0]);

static void getStateInfo(CharacterState state, std::string *group)
{
    for(size_t i = 0;i < sStateListSize;i++)
    {
        if(sStateList[i].state == state)
        {
            *group = sStateList[i].groupname;
            return;
        }
    }
    throw std::runtime_error("Failed to find character state "+Ogre::StringConverter::toString(state));
}


CharacterController::CharacterController(const MWWorld::Ptr &ptr, MWRender::Animation *anim, CharacterState state, bool loop)
    : mPtr(ptr)
    , mAnimation(anim)
    , mCharState(state)
    , mSkipAnim(false)
    , mMovingAnim(false)
    , mSecondsOfRunning(0)
    , mSecondsOfSwimming(0)
    , mLooping(false)
{
    if(!mAnimation)
        return;

    std::string group;
    getStateInfo(mCharState, &group);
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
    if(mAnimation->hasAnimation(group))
        mMovingAnim = mAnimation->play(group, "start", "stop", 1.0f, loop ? (~(size_t)0) : 0);
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
        const MWBase::World *world = MWBase::Environment::get().getWorld();
        const MWWorld::Class &cls = MWWorld::Class::get(mPtr);

        bool onground = world->isOnGround(mPtr);
        bool inwater = world->isSwimming(mPtr);
        bool isrunning = cls.getStance(mPtr, MWWorld::Class::Run);
        bool sneak = cls.getStance(mPtr, MWWorld::Class::Sneak);
        const Ogre::Vector3 &vec = cls.getMovementVector(mPtr);
        const Ogre::Vector3 &rot = cls.getRotationVector(mPtr);
        speed = cls.getSpeed(mPtr);

        // advance athletics
        if (vec.squaredLength() > 0 && mPtr == MWBase::Environment::get().getWorld()->getPlayer().getPlayer())
        {
            if (inwater)
            {
                mSecondsOfSwimming += duration;
                while (mSecondsOfSwimming > 1)
                {
                    MWWorld::Class::get(mPtr).skillUsageSucceeded(mPtr, ESM::Skill::Athletics, 1);
                    mSecondsOfSwimming -= 1;
                }
            }
            else if (isrunning)
            {
                mSecondsOfRunning += duration;
                while (mSecondsOfRunning > 1)
                {
                    MWWorld::Class::get(mPtr).skillUsageSucceeded(mPtr, ESM::Skill::Athletics, 0);
                    mSecondsOfRunning -= 1;
                }
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
                                 : (sneak ? CharState_SneakRight : (isrunning ? CharState_RunRight : CharState_WalkRight)), true);
            else if(vec.x < 0.0f)
                setState(inwater ? (isrunning ? CharState_SwimRunLeft : CharState_SwimWalkLeft)
                                 : (sneak ? CharState_SneakLeft : (isrunning ? CharState_RunLeft : CharState_WalkLeft)), true);

            // If this animation isn't moving us sideways, do it manually
            if(!mMovingAnim)
                movement.mPosition[0] += vec.x * (speed*duration);
            // Apply any forward/backward movement manually
            movement.mPosition[1] += vec.y * (speed*duration);
        }
        else if(vec.y != 0.0f && speed > 0.0f)
        {
            if(vec.y > 0.0f)
                setState(inwater ? (isrunning ? CharState_SwimRunForward : CharState_SwimWalkForward)
                                 : (sneak ? CharState_SneakForward : (isrunning ? CharState_RunForward : CharState_WalkForward)), true);
            else if(vec.y < 0.0f)
                setState(inwater ? (isrunning ? CharState_SwimRunBack : CharState_SwimWalkBack)
                                 : (sneak ? CharState_SneakBack : (isrunning ? CharState_RunBack : CharState_WalkBack)), true);

            // Apply any sideways movement manually
            movement.mPosition[0] += vec.x * (speed*duration);
            // If this animation isn't moving us forward/backward, do it manually
            if(!mMovingAnim)
                movement.mPosition[1] += vec.y * (speed*duration);
        }
        else if(rot.z != 0.0f && !inwater && !sneak)
        {
            if(rot.z > 0.0f)
                setState(CharState_TurnRight, true);
            else if(rot.z < 0.0f)
                setState(CharState_TurnLeft, true);
        }
        else if(getState() != CharState_SpecialIdle || !mAnimation->isPlaying(0))
        {
            if(mAnimQueue.size() == 0)
                setState((inwater ? CharState_IdleSwim : (sneak ? CharState_IdleSneak : CharState_Idle)), true);
            else
            {
                mMovingAnim = mAnimation->play(mAnimQueue.front().first,
                                               "start", "stop", 0.0f,
                                               mAnimQueue.front().second);
                mAnimQueue.pop_front();
            }
        }

        movement.mRotation[0] += rot.x * duration;
        movement.mRotation[1] += rot.y * duration;
        movement.mRotation[2] += rot.z * duration;
    }

    if(mAnimation && !mSkipAnim)
    {
        mAnimation->setSpeed(speed);
        Ogre::Vector3 moved = mAnimation->runAnimation(duration);
        movement.mPosition[0] += moved.x;
        movement.mPosition[1] += moved.y;
        movement.mPosition[2] += moved.z;
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
        if(mode != 0 || getState() != CharState_SpecialIdle)
        {
            mAnimQueue.clear();
            mCharState = CharState_SpecialIdle;
            mLooping = false;
            mMovingAnim = mAnimation->play(groupname, ((mode==2) ? "loop start" : "start"), "stop", 0.0f, count-1);
        }
        else if(mode == 0)
        {
            mAnimQueue.clear();
            mAnimQueue.push_back(std::make_pair(groupname, count-1));
        }
    }
}

void CharacterController::skipAnim()
{
    mSkipAnim = true;
}


void CharacterController::setState(CharacterState state, bool loop)
{
    if(mCharState == state)
        return;
    mCharState = state;
    mLooping = loop;

    forceStateUpdate();
}

void CharacterController::forceStateUpdate()
{
    if(!mAnimation)
        return;
    mAnimQueue.clear();

    std::string anim;
    getStateInfo(mCharState, &anim);
    if((mMovingAnim=mAnimation->hasAnimation(anim)) != false)
        mMovingAnim = mAnimation->play(anim, "start", "stop", 0.0f, mLooping ? (~(size_t)0) : 0);
}

}
