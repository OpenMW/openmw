
#include "action.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/soundmanager.hpp"

MWWorld::Action::Action() {}

MWWorld::Action::~Action() {}

void MWWorld::Action::execute (const Ptr& actor)
{
    if (!mSoundId.empty())
    {
    	if (onActor)
    	{
    		std::cout << "Douglas - Som Normal" << std::endl;
    		MWBase::Environment::get().getSoundManager()->playSound(mSoundId, 1.0, 1.0,
    				MWBase::SoundManager::Play_NoTrack);
    	}
    	else
    	{
    		std::cout << "Douglas - Som 3D" << std::endl;
    		MWBase::Environment::get().getSoundManager()->playSound3D (actor, mSoundId, 1.0, 1.0,
    				MWBase::SoundManager::Play_NoTrack);
    	}
    }

    executeImp (actor);
}

void MWWorld::Action::setSound (const std::string& id, const bool onActorValue)
{
    mSoundId = id;
    onActor = onActorValue;
}
