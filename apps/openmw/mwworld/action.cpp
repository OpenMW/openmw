
#include "action.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwbase/soundmanager.hpp"

MWWorld::Action::Action() {
	teleport = false;
}

MWWorld::Action::~Action() {}

void MWWorld::Action::execute (const Ptr& actor)
{
    if (!mSoundId.empty())
    {
    	if (teleport == true)
    	{
    		//this is a teleport action, so we need to call playSound
    		MWBase::Environment::get().getSoundManager()->playSound(mSoundId, 1.0, 1.0,
    				MWBase::SoundManager::Play_NoTrack);
    	}
    	else
    	{
    		MWBase::Environment::get().getSoundManager()->playSound3D (actor, mSoundId, 1.0, 1.0,
    				MWBase::SoundManager::Play_NoTrack);
    	}
    }

    executeImp (actor);
}

void MWWorld::Action::setSound (const std::string& id)
{
    mSoundId = id;
}
