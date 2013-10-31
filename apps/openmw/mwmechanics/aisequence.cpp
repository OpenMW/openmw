
#include "aisequence.hpp"

#include "aipackage.hpp"

#include "aiwander.hpp"
#include "aiescort.hpp"
#include "aitravel.hpp"
#include "aifollow.hpp"
#include "aiactivate.hpp"
#include "aicombat.hpp"

#include "../mwworld/class.hpp"
#include "creaturestats.hpp"
#include "npcstats.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwworld/player.hpp"

#include "../mwbase/mechanicsmanager.hpp"

void MWMechanics::AiSequence::copy (const AiSequence& sequence)
{
    for (std::list<AiPackage *>::const_iterator iter (sequence.mPackages.begin());
        iter!=sequence.mPackages.end(); ++iter)
        mPackages.push_back ((*iter)->clone());
    mCombat = sequence.mCombat;
    mCombatPackage = 0;
    if(sequence.mCombat) mCombatPackage = sequence.mCombatPackage->clone();
}

MWMechanics::AiSequence::AiSequence() : mDone (false), mCombat (false), mCombatPackage (0) {}

MWMechanics::AiSequence::AiSequence (const AiSequence& sequence) : mDone (false)
{
    copy (sequence);
}

MWMechanics::AiSequence& MWMechanics::AiSequence::operator= (const AiSequence& sequence)
{
    if (this!=&sequence)
    {
        clear();
        copy (sequence);
    }
    
    return *this;
}

MWMechanics::AiSequence::~AiSequence()
{
    clear();
}

int MWMechanics::AiSequence::getTypeId() const
{
    if (mPackages.empty())
        return -1;
        
    return mPackages.front()->getTypeId();
}

bool MWMechanics::AiSequence::isPackageDone() const
{
    return mDone;
}

void MWMechanics::AiSequence::execute (const MWWorld::Ptr& actor,float duration)
{
    if(actor != MWBase::Environment::get().getWorld()->getPlayer().getPlayer())
    {
        if(mCombat)
        {
            mCombatPackage->execute(actor,duration);
        }
        else
        {
            if(actor.getTypeName() == typeid(ESM::NPC).name())
            {
                ESM::Position playerpos = MWBase::Environment::get().getWorld()->getPlayer().getPlayer().getRefData().getPosition();
                ESM::Position actorpos = actor.getRefData().getPosition();
                float d = sqrt((actorpos.pos[0] - playerpos.pos[0])*(actorpos.pos[0] - playerpos.pos[0])
                    +(actorpos.pos[1] - playerpos.pos[1])*(actorpos.pos[1] - playerpos.pos[1])
                    +(actorpos.pos[2] - playerpos.pos[2])*(actorpos.pos[2] - playerpos.pos[2]));
                float fight = actor.getClass().getCreatureStats(actor).getAiSetting(1);
                float disp = MWBase::Environment::get().getMechanicsManager()->getDerivedDisposition(actor);
                bool LOS = MWBase::Environment::get().getWorld()->getLOS(actor,MWBase::Environment::get().getWorld()->getPlayer().getPlayer());
                if(  ( (fight == 100 ) 
                    || (fight >= 95 && d <= 3000)
                    || (fight >= 90 && d <= 2000)
                    || (fight >= 80 && d <= 1000)
                    || (fight >= 80 && disp <= 40) 
                    || (fight >= 70 && disp <= 35 && d <= 1000) 
                    || (fight >= 60 && disp <= 30 && d <= 1000) 
                    || (fight >= 50 && disp == 0) 
                    || (fight >= 40 && disp <= 10 && d <= 500) )
                    && LOS
                    )
                {
                    mCombat = true;
                    mCombatPackage = new AiCombat("player");
                }
            }
            if (!mPackages.empty())
            {
                if (mPackages.front()->execute (actor,duration))
                {
                    mPackages.erase (mPackages.begin());
                    mDone = true;
                }
                else
                    mDone = false;    
            }
        }
    }
}

void MWMechanics::AiSequence::clear()
{
    for (std::list<AiPackage *>::const_iterator iter (mPackages.begin()); iter!=mPackages.end(); ++iter)
        delete *iter;
    
    if(mCombatPackage) 
    {
        delete mCombatPackage;
        mCombatPackage = 0;
    }
    mPackages.clear();
}

void MWMechanics::AiSequence::stack (const AiPackage& package)
{
    mPackages.push_front (package.clone());
}

void MWMechanics::AiSequence::queue (const AiPackage& package)
{
    mPackages.push_back (package.clone());
}

void MWMechanics::AiSequence::fill(const ESM::AIPackageList &list)
{
    for (std::vector<ESM::AIPackage>::const_iterator it = list.mList.begin(); it != list.mList.end(); ++it)
    {
        MWMechanics::AiPackage* package;
        if (it->mType == ESM::AI_Wander)
        {
            ESM::AIWander data = it->mWander;
            std::vector<int> idles;
            for (int i=0; i<8; ++i)
                idles.push_back(data.mIdle[i]);
            package = new MWMechanics::AiWander(data.mDistance, data.mDuration, data.mTimeOfDay, idles, data.mUnk);
        }
        else if (it->mType == ESM::AI_Escort)
        {
            ESM::AITarget data = it->mTarget;
            package = new MWMechanics::AiEscort(data.mId.toString(), data.mDuration, data.mX, data.mY, data.mZ);
        }
        else if (it->mType == ESM::AI_Travel)
        {
            ESM::AITravel data = it->mTravel;
            package = new MWMechanics::AiTravel(data.mX, data.mY, data.mZ);
        }
        else if (it->mType == ESM::AI_Activate)
        {
            ESM::AIActivate data = it->mActivate;
            package = new MWMechanics::AiActivate(data.mName.toString());
        }
        else //if (it->mType == ESM::AI_Follow)
        {
            ESM::AITarget data = it->mTarget;
            package = new MWMechanics::AiFollow(data.mId.toString(), data.mDuration, data.mX, data.mY, data.mZ);
        }
        mPackages.push_back(package);
    }
}
