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

#include "../mwrender/animation.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/soundmanager.hpp"

#include "../mwworld/class.hpp"

namespace MWMechanics
{

CharacterController::CharacterController(const MWWorld::Ptr &ptr, MWRender::Animation *anim, CharacterState state)
  : mPtr(ptr), mAnimation(anim), mState(state), mSkipAnim(false)
{
    if(mAnimation)
        mAnimNames = mAnimation->getAnimationNames();
    if(mAnimNames.size() == 0)
    {
        mAnimation = NULL;
        return;
    }

    mAnimation->setController(this);
    setState(mState);
}

CharacterController::CharacterController(const CharacterController &rhs)
  : mPtr(rhs.mPtr), mAnimation(rhs.mAnimation), mAnimNames(rhs.mAnimNames)
  , mAnimQueue(rhs.mAnimQueue), mCurrentGroup(rhs.mCurrentGroup)
  , mState(rhs.mState), mSkipAnim(rhs.mSkipAnim)
{
    if(mAnimNames.size() == 0)
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
    std::string::size_type ms = mCurrentGroup.length()+2;
    if(evt.length() <= ms || evt.compare(0, ms-2, mCurrentGroup) != 0 || evt.compare(ms-2, 2, ": ") != 0)
    {
        std::cerr<< "Event \""<<evt<<"\" does not belong to group \""<<mCurrentGroup<<"\"" <<std::endl;
        return;
    }

    if(evt.compare(ms, evt.length()-ms, "loop stop") == 0)
    {
        if(mAnimQueue.size() == 0)
        {
            if(time > 0.0f && mState != CharState_Dead)
                mAnimation->play(mCurrentGroup, "loop start");
        }
        else if(mAnimQueue.size() >= 2 && mAnimQueue[0] == mAnimQueue[1])
        {
            mAnimQueue.pop_front();
            mAnimation->play(mCurrentGroup, "loop start");
        }
        return;
    }

    if(evt.compare(ms, evt.length()-ms, "stop") == 0)
    {
        if(mAnimQueue.size() == 0)
        {
            if(time > 0.0f && mState != CharState_Dead)
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


Ogre::Vector3 CharacterController::update(float duration)
{
    Ogre::Vector3 movement = Ogre::Vector3::ZERO;
    if(mAnimation && !mSkipAnim)
        movement += mAnimation->runAnimation(duration);
    mSkipAnim = false;

    if(getState() == CharState_SpecialIdle || getState() == CharState_Idle ||
       getState() == CharState_Dead)
    {
        // FIXME: mDirection shouldn't influence the movement here.
        movement += mDirection;
    }
    else
    {
        // FIXME: mDirection should be normalized after setting the speed of
        // the animation in setDirection, rather than here.
        movement = mDirection.normalisedCopy() * movement.length();
    }

    return movement;
}


void CharacterController::playGroup(const std::string &groupname, int mode, int count)
{
    if(std::find(mAnimNames.begin(), mAnimNames.end(), groupname) != mAnimNames.end())
    {
        count = std::max(count, 1);
        if(mode != 0 || mAnimQueue.size() == 0)
        {
            mAnimQueue.clear();
            while(count-- > 0)
                mAnimQueue.push_back(groupname);
            mCurrentGroup = groupname;
            mState = CharState_SpecialIdle;
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


void CharacterController::setState(CharacterState state)
{
    mState = state;

    if(mAnimNames.size() == 0)
        return;
    mAnimQueue.clear();
    switch(mState)
    {
        case CharState_SpecialIdle:
            break;
        case CharState_Idle:
            mCurrentGroup = "idle";
            mAnimation->setAccumulation(Ogre::Vector3::ZERO);
            mAnimation->play(mCurrentGroup, "start");
            break;
        case CharState_Dead:
            mCurrentGroup = "death1";
            mAnimation->setAccumulation(Ogre::Vector3(1.0f, 1.0f, 0.0f));
            mAnimation->play(mCurrentGroup, "start");
            break;
    }
}

}
