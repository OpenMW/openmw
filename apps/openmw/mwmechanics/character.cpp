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
#include "../mwbase/soundmanager.hpp"

#include "../mwworld/class.hpp"

namespace MWMechanics
{

static const struct {
    CharacterState state;
    const char groupname[32];
    Ogre::Vector3 accumulate;
} sStateList[] = {
    { CharState_Idle, "idle", Ogre::Vector3(1.0f, 1.0f, 0.0f) },

    { CharState_WalkForward, "walkforward", Ogre::Vector3(0.0f, 1.0f, 0.0f) },
    { CharState_WalkBack, "walkback", Ogre::Vector3(0.0f, 1.0f, 0.0f) },
    { CharState_WalkLeft, "walkleft", Ogre::Vector3(1.0f, 0.0f, 0.0f) },
    { CharState_WalkRight, "walkright", Ogre::Vector3(1.0f, 0.0f, 0.0f) },

    { CharState_Dead, "death1", Ogre::Vector3(1.0f, 1.0f, 0.0f) },
};
static const size_t sStateListSize = sizeof(sStateList)/sizeof(sStateList[0]);

static void getStateInfo(CharacterState state, std::string *group, Ogre::Vector3 *accum)
{
    for(size_t i = 0;i < sStateListSize;i++)
    {
        if(sStateList[i].state == state)
        {
            *group = sStateList[i].groupname;
            *accum = sStateList[i].accumulate;
            return;
        }
    }
    throw std::runtime_error("Failed to find character state "+Ogre::StringConverter::toString(state));
}

CharacterController::CharacterController(const MWWorld::Ptr &ptr, MWRender::Animation *anim, CharacterState state, bool loop)
  : mPtr(ptr), mAnimation(anim), mDirection(Ogre::Vector3::ZERO), mState(state), mSkipAnim(false), mLoop(loop)
{
    if(!mAnimation)
        return;

    mAnimation->setController(this);

    Ogre::Vector3 accum;
    getStateInfo(mState, &mCurrentGroup, &accum);
    mAnimation->setAccumulation(accum);
    if(mAnimation->hasAnimation(mCurrentGroup))
        mAnimation->play(mCurrentGroup, "stop");
}

CharacterController::CharacterController(const CharacterController &rhs)
  : mPtr(rhs.mPtr), mAnimation(rhs.mAnimation), mAnimQueue(rhs.mAnimQueue)
  , mCurrentGroup(rhs.mCurrentGroup), mDirection(rhs.mDirection)
  , mState(rhs.mState), mSkipAnim(rhs.mSkipAnim), mLoop(rhs.mLoop)
{
    if(!mAnimation)
        return;
    /* We've been copied. Update the animation with the new controller. */
    mAnimation->setController(this);
}


void CharacterController::markerEvent(float time, const std::string &evt)
{
    if(evt.compare(0, 7, "sound: ") == 0)
    {
        MWBase::SoundManager *sndMgr = MWBase::Environment::get().getSoundManager();
        sndMgr->playSound3D(mPtr, evt.substr(7), 1.0f, 1.0f);
        return;
    }
    if(evt.compare(0, 10, "soundgen: ") == 0)
    {
        // FIXME: Lookup the SoundGen (SNDG) for the specified sound that corresponds
        // to this actor type
        return;
    }

    if(evt == "loop stop")
    {
        if(mAnimQueue.size() == 0)
        {
            if(time > 0.0f && mLoop)
                mAnimation->play(mCurrentGroup, "loop start");
        }
        else if(mAnimQueue.size() >= 2 && mAnimQueue[0] == mAnimQueue[1])
        {
            mAnimQueue.pop_front();
            mAnimation->play(mCurrentGroup, "loop start");
        }
        return;
    }

    if(evt == "stop")
    {
        if(mAnimQueue.size() == 0)
        {
            if(time > 0.0f && mLoop)
                mAnimation->play(mCurrentGroup, "loop start");
        }
        else if(mAnimQueue.size() >= 2 && mAnimQueue[0] == mAnimQueue[1])
        {
            mAnimQueue.pop_front();
            mAnimation->play(mCurrentGroup, "loop start");
        }
        else if(mAnimQueue.size() > 0)
        {
            mAnimQueue.pop_front();
            if(mAnimQueue.size() > 0)
            {
                mCurrentGroup = mAnimQueue.front();
                mAnimation->play(mCurrentGroup, "start");
            }
        }
        return;
    }
}


void CharacterController::setDirection(const Ogre::Vector3 &dir)
{
    // HACK: The direction length we get is too large.
    float mult = dir.length() / 32.0f;
    mult = std::max(1.0f, mult);
    if(mAnimation)
        mAnimation->setSpeedMult(mult);
    mDirection = dir.normalisedCopy();
}


Ogre::Vector3 CharacterController::update(float duration)
{
    Ogre::Vector3 movement = Ogre::Vector3::ZERO;
    if(mAnimation && !mSkipAnim)
        movement += mAnimation->runAnimation(duration);
    mSkipAnim = false;

    if(!(getState() == CharState_Idle || getState() == CharState_Dead))
    {
        movement = mDirection * movement.length();
    }

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
            mState = CharState_Idle;
            mLoop = false;
            mAnimation->setAccumulation(Ogre::Vector3::ZERO);
            mAnimation->play(mCurrentGroup, ((mode==2) ? "loop start" : "start"));
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
        return;
    else
    {
        mState = state;
        mLoop = loop;
    }

    if(!mAnimation)
        return;
    mAnimQueue.clear();

    std::string anim;
    Ogre::Vector3 accum;
    getStateInfo(mState, &anim, &accum);
    if(mAnimation->hasAnimation(anim))
    {
        mCurrentGroup = anim;
        mAnimation->setAccumulation(accum);
        mAnimation->play(mCurrentGroup, "start");
    }
}

}
