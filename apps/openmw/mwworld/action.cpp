
#include "action.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwbase/soundmanager.hpp"

const MWWorld::Ptr& MWWorld::Action::getTarget() const
{
    return mTarget;
}

MWWorld::Action::Action (bool keepSound, const Ptr& target) : mKeepSound (keepSound), mTarget (target)
{}

MWWorld::Action::~Action() {}

void MWWorld::Action::execute (const Ptr& actor)
{
    if (!mSoundId.empty())
    {
        if (mKeepSound && actor.getRefData().getHandle()=="player")
        {
            // sound moves with player when teleporting
            MWBase::Environment::get().getSoundManager()->playSound(mSoundId, 1.0, 1.0,
                MWBase::SoundManager::Play_NoTrack);
        }
        else
        {
            bool local = mTarget.isEmpty() || !mTarget.isInCell(); // no usable target
        
            MWBase::Environment::get().getSoundManager()->playSound3D (local ? actor : mTarget,
                mSoundId, 1.0, 1.0,
                mKeepSound ? MWBase::SoundManager::Play_NoTrack : MWBase::SoundManager::Play_Normal);
        }
    }

    executeImp (actor);
}

void MWWorld::Action::setSound (const std::string& id)
{
    mSoundId = id;
}
