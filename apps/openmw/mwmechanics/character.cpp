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

#include "../mwrender/animation.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

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
  : mPtr(ptr), mAnimation(anim), mState(state), mSkipAnim(false)
{
    if(!mAnimation)
        return;

    mAnimation->setController(this);

    getStateInfo(mState, &mCurrentGroup);
    if(ptr.getTypeName() == typeid(ESM::Activator).name())
    {
        /* Don't accumulate with activators (they don't get moved). */
        mAnimation->setAccumulation(Ogre::Vector3::ZERO);
    }
    else
    {
        /* Accumulate along X/Y only for now, until we can figure out how we should
         * handle knockout and death which moves the character down. */
        mAnimation->setAccumulation(Ogre::Vector3(1.0f, 1.0f, 0.0f));
    }
    if(mAnimation->hasAnimation(mCurrentGroup))
        mAnimation->play(mCurrentGroup, "stop", "stop", loop);
}

CharacterController::CharacterController(const CharacterController &rhs)
  : mPtr(rhs.mPtr), mAnimation(rhs.mAnimation), mAnimQueue(rhs.mAnimQueue)
  , mCurrentGroup(rhs.mCurrentGroup), mState(rhs.mState)
  , mSkipAnim(rhs.mSkipAnim)
{
    if(!mAnimation)
        return;
    /* We've been copied. Update the animation with the new controller. */
    mAnimation->setController(this);
}

CharacterController::~CharacterController()
{
}


void CharacterController::updatePtr(const MWWorld::Ptr &ptr)
{
    mPtr = ptr;
}


void CharacterController::markerEvent(float time, const std::string &evt)
{
    if(evt == "stop")
    {
        if(mAnimQueue.size() >= 2 && mAnimQueue[0] == mAnimQueue[1])
        {
            mAnimQueue.pop_front();
            mAnimation->play(mCurrentGroup, "loop start", "stop", false);
        }
        else if(mAnimQueue.size() > 0)
        {
            mAnimQueue.pop_front();
            if(mAnimQueue.size() > 0)
            {
                mCurrentGroup = mAnimQueue.front();
                mAnimation->play(mCurrentGroup, "start", "stop", false);
            }
        }
        return;
    }

    std::cerr<< "Unhandled animation event: "<<evt <<std::endl;
}


Ogre::Vector3 CharacterController::update(float duration)
{
    Ogre::Vector3 movement(0.0f);

    float speed = 0.0f;
    if(!(getState() >= CharState_Death1))
    {
        const MWBase::World *world = MWBase::Environment::get().getWorld();
        const MWWorld::Class &cls = MWWorld::Class::get(mPtr);
        const Ogre::Vector3 &vec = cls.getMovementVector(mPtr);

        bool onground = world->isOnGround(mPtr);
        bool inwater = world->isSwimming(mPtr);
        bool isrunning = cls.getStance(mPtr, MWWorld::Class::Run);
        bool sneak = cls.getStance(mPtr, MWWorld::Class::Sneak);
        speed = cls.getSpeed(mPtr);

        /* FIXME: The state should be set to Jump, and X/Y movement should be disallowed except
         * for the initial thrust (which would be carried by "physics" until landing). */
        if(onground && vec.z > 0.0f)
        {
            float x = cls.getJump(mPtr);

            if(vec.x == 0 && vec.y == 0)
                movement.z += x*duration;
            else
            {
                /* FIXME: this would be more correct if we were going into a jumping state,
                 * rather than normal walking/idle states. */
                //Ogre::Vector3 lat = Ogre::Vector3(vec.x, vec.y, 0.0f).normalisedCopy();
                //movement += Ogre::Vector3(lat.x, lat.y, 1.0f) * x * 0.707f * duration;
                movement.z += x * 0.707f * duration;
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

            // Apply any forward/backward movement manually
            movement.y += vec.y * (speed*duration);
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
            movement.x += vec.x * (speed*duration);
        }
        else if(mAnimQueue.size() == 0)
            setState((inwater ? CharState_IdleSwim : (sneak ? CharState_IdleSneak : CharState_Idle)), true);
    }

    if(mAnimation && !mSkipAnim)
    {
        mAnimation->setSpeed(speed);
        movement += mAnimation->runAnimation(duration);
    }
    mSkipAnim = false;

    return movement;
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
            while(count-- > 0)
                mAnimQueue.push_back(groupname);
            mCurrentGroup = groupname;
            mState = CharState_SpecialIdle;
            mAnimation->play(mCurrentGroup, ((mode==2) ? "loop start" : "start"), "stop", false);
        }
        else if(mode == 0)
        {
            mAnimQueue.resize(1);
            while(count-- > 0)
                mAnimQueue.push_back(groupname);
        }
    }
}

void CharacterController::skipAnim()
{
    mSkipAnim = true;
}


void CharacterController::setState(CharacterState state, bool loop)
{
    if(mState == state)
    {
        if(mAnimation)
            mAnimation->setLooping(loop);
        return;
    }
    mState = state;

    if(!mAnimation)
        return;
    mAnimQueue.clear();

    std::string anim;
    getStateInfo(mState, &anim);
    if(mAnimation->hasAnimation(anim))
    {
        mCurrentGroup = anim;
        mAnimation->play(mCurrentGroup, "start", "stop", loop);
    }
}

}
