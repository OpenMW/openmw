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

namespace MWMechanics
{

CharacterController::CharacterController(const MWWorld::Ptr &ptr, MWRender::Animation *anim, CharacterState state)
  : mPtr(ptr), mAnimation(anim), mState(state)
{
    if(!mAnimation)
        return;

    mAnimation->setController(this);
    switch(mState)
    {
        case CharState_Idle:
            mAnimation->playGroup("idle", 1, 1);
            break;
        case CharState_Dead:
            mAnimation->playGroup("death1", 1, 1);
            break;
    }
}


void CharacterController::setState(CharacterState state)
{
    mState = state;
    switch(mState)
    {
        case CharState_Idle:
            mAnimation->playGroup("idle", 1, 1);
            break;
        case CharState_Dead:
            mAnimation->playGroup("death1", 1, 1);
            break;
    }
}

}
