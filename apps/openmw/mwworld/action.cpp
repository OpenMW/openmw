
#include "action.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/soundmanager.hpp"

MWWorld::Action::Action() {}

MWWorld::Action::~Action() {}

void MWWorld::Action::execute (const Ptr& actor)
{
    if (!mSoundId.empty())
        MWBase::Environment::get().getSoundManager()->playSound3D (actor, mSoundId, 1.0, 1.0,
            MWBase::SoundManager::Play_NoTrack);

    executeImp (actor);
}

void MWWorld::Action::setSound (const std::string& id)
{
    mSoundId = id;
}
