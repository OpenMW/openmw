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
#include "../mwbase/world.hpp"

#include "../mwworld/class.hpp"

#include "movementsolver.hpp"


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

    { CharState_WalkForward, "walkforward" },
    { CharState_WalkBack, "walkback" },
    { CharState_WalkLeft, "walkleft" },
    { CharState_WalkRight, "walkright" },

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
    mMovementSolver = new MovementSolver(mPtr);
    if(!mAnimation)
        return;

    mAnimation->setController(this);

    getStateInfo(mState, &mCurrentGroup);
    /* Accumulate along X/Y only for now, until we can figure out how we should
     * handle knockout and death which moves the character down. */
    mAnimation->setAccumulation(Ogre::Vector3(1.0f, 1.0f, 0.0f));
    if(mAnimation->hasAnimation(mCurrentGroup))
        mAnimation->play(mCurrentGroup, "stop", loop);
}

CharacterController::CharacterController(const CharacterController &rhs)
  : mPtr(rhs.mPtr), mAnimation(rhs.mAnimation), mAnimQueue(rhs.mAnimQueue)
  , mCurrentGroup(rhs.mCurrentGroup), mState(rhs.mState)
  , mSkipAnim(rhs.mSkipAnim)
{
    mMovementSolver = new MovementSolver(mPtr);
    if(!mAnimation)
        return;
    /* We've been copied. Update the animation with the new controller. */
    mAnimation->setController(this);
}

CharacterController::~CharacterController()
{
    delete mMovementSolver;
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

    if(evt == "stop")
    {
        if(mAnimQueue.size() >= 2 && mAnimQueue[0] == mAnimQueue[1])
        {
            mAnimQueue.pop_front();
            mAnimation->play(mCurrentGroup, "loop start", false);
        }
        else if(mAnimQueue.size() > 0)
        {
            mAnimQueue.pop_front();
            if(mAnimQueue.size() > 0)
            {
                mCurrentGroup = mAnimQueue.front();
                mAnimation->play(mCurrentGroup, "start", false);
            }
        }
        return;
    }

    std::cerr<< "Unhandled animation event: "<<evt <<std::endl;
}


Ogre::Vector3 CharacterController::update(float duration)
{
    const MWWorld::Class &cls = MWWorld::Class::get(mPtr);
    const Ogre::Vector3 &vec = cls.getMovementVector(mPtr);

    if(std::abs(vec.x/2.0f) > std::abs(vec.y))
    {
        if(vec.x > 0.0f)
            setState(CharState_WalkRight, true);
        else if(vec.x < 0.0f)
            setState(CharState_WalkLeft, true);
    }
    else if(vec.y > 0.0f)
        setState(CharState_WalkForward, true);
    else if(vec.y < 0.0f)
        setState(CharState_WalkBack, true);
    else
    {
        if(!(getState() >= CharState_Death1))
            setState(CharState_Idle, true);
    }

    Ogre::Vector3 movement = Ogre::Vector3::ZERO;
    if(mAnimation && !mSkipAnim)
    {
        // FIXME: The speed should actually be determined by the character's
        // stance (running, sneaking, etc) and stats
        mAnimation->setSpeedMult(1.0f);
        movement += mAnimation->runAnimation(duration);
    }
    mSkipAnim = false;

    if(duration > 0.0f)
    {
        const ESM::Position &refpos = mPtr.getRefData().getPosition();

        // Rotates first around z, then y, then x
        movement = (Ogre::Quaternion(Ogre::Radian(-refpos.rot[0]), Ogre::Vector3::UNIT_X)*
                    Ogre::Quaternion(Ogre::Radian(-refpos.rot[1]), Ogre::Vector3::UNIT_Y)*
                    Ogre::Quaternion(Ogre::Radian(-refpos.rot[2]), Ogre::Vector3::UNIT_Z)) *
                   movement;

        Ogre::Vector3 pos(refpos.pos);

        // FIXME: Get the actual radius for the object. Maybe this should go into mwworld to replace pmove?
        Ogre::Vector3 res = mMovementSolver->move(pos, movement, duration, Ogre::Vector3(15,15,30));
        MWBase::Environment::get().getWorld()->moveObject(mPtr, res.x, res.y, res.z);
    }

    return Ogre::Vector3(0.0f);
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
            mAnimation->play(mCurrentGroup, ((mode==2) ? "loop start" : "start"), false);
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
    mState = state;

    if(!mAnimation)
        return;
    mAnimQueue.clear();

    std::string anim;
    getStateInfo(mState, &anim);
    if(mAnimation->hasAnimation(anim))
    {
        mCurrentGroup = anim;
        mAnimation->play(mCurrentGroup, "start", loop);
    }
}

}
