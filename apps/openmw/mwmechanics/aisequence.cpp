
#include "aisequence.hpp"

#include "aipackage.hpp"

#include "aiwander.hpp"
#include "aiescort.hpp"
#include "aitravel.hpp"
#include "aifollow.hpp"
#include "aiactivate.hpp"
#include "aicombat.hpp"
#include "aipursue.hpp"

#include "../mwworld/class.hpp"
#include "creaturestats.hpp"
#include "npcstats.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

namespace MWMechanics
{

void AiSequence::copy (const AiSequence& sequence)
{
    for (std::list<AiPackage *>::const_iterator iter (sequence.mPackages.begin());
        iter!=sequence.mPackages.end(); ++iter)
        mPackages.push_back ((*iter)->clone());
}

AiSequence::AiSequence() : mDone (false), mLastAiPackage(-1) {}

AiSequence::AiSequence (const AiSequence& sequence) : mDone (false)
{
    copy (sequence);
}

AiSequence& AiSequence::operator= (const AiSequence& sequence)
{
    if (this!=&sequence)
    {
        clear();
        copy (sequence);
        mDone = sequence.mDone;
    }

    return *this;
}

AiSequence::~AiSequence()
{
    clear();
}

int AiSequence::getTypeId() const
{
    if (mPackages.empty())
        return -1;

    return mPackages.front()->getTypeId();
}

bool AiSequence::getCombatTarget(std::string &targetActorId) const
{
    if (getTypeId() != AiPackage::TypeIdCombat)
        return false;
    const AiCombat *combat = static_cast<const AiCombat *>(mPackages.front());
    
    MWWorld::Ptr target = combat->getTarget();
    targetActorId = target.getClass().getCreatureStats(target).getActorId();

    return true;
}

bool AiSequence::canAddTarget(const ESM::Position& actorPos, float distToTarget) const
{
    bool firstCombatFound = false;
    MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();

    for(std::list<AiPackage*>::const_iterator it = mPackages.begin(); it != mPackages.end(); ++it)
    {
        if ((*it)->getTypeId() == AiPackage::TypeIdCombat)
        {
            firstCombatFound = true;

            const AiCombat *combat = static_cast<const AiCombat *>(*it);
            if (combat->getTarget() != player ) return false; // only 1 non-player target allowed
            else
            {
                // add new target only if current target (player) is farther
                ESM::Position &targetPos = combat->getTarget().getRefData().getPosition();

                float distToCurrTarget = (Ogre::Vector3(targetPos.pos) - Ogre::Vector3(actorPos.pos)).length();
                return (distToCurrTarget > distToTarget);
            }
        }
        else if (firstCombatFound) break; // assumes combat packages go one-by-one in packages list
    }
    return true;
}

void AiSequence::stopCombat()
{
    while (getTypeId() == AiPackage::TypeIdCombat)
    {
        delete *mPackages.begin();
        mPackages.erase (mPackages.begin());
    }
}

void AiSequence::stopPursuit()
{
    while (getTypeId() == AiPackage::TypeIdPursue)
    {
        delete *mPackages.begin();
        mPackages.erase (mPackages.begin());
    }
}

bool AiSequence::isPackageDone() const
{
    return mDone;
}

void AiSequence::execute (const MWWorld::Ptr& actor,float duration)
{
    if(actor != MWBase::Environment::get().getWorld()->getPlayerPtr())
    {
        if (!mPackages.empty())
        {
            MWMechanics::AiPackage* package = mPackages.front();
            mLastAiPackage = package->getTypeId();

            // if active package is combat one, choose nearest target
            if (mLastAiPackage == AiPackage::TypeIdCombat)
            {
                std::list<AiPackage *>::const_iterator itActualCombat;

                float nearestDist = std::numeric_limits<float>::max();
                Ogre::Vector3 vActorPos = Ogre::Vector3(actor.getRefData().getPosition().pos);

                const AiCombat *package;

                for(std::list<AiPackage *>::const_iterator it = mPackages.begin(); it != mPackages.end(); ++it)
                {
                    package = static_cast<const AiCombat *>(*it);

                    if ((*it)->getTypeId() != AiPackage::TypeIdCombat) break;

                    ESM::Position &targetPos = package->getTarget().getRefData().getPosition();

                    float distTo = (Ogre::Vector3(targetPos.pos) - vActorPos).length();
                    if (distTo < nearestDist)
                    {
                        nearestDist = distTo;
                        itActualCombat = it;
                    }
                }

                if (mPackages.begin() != itActualCombat)
                {
                    // move combat package with nearest target to the front
                    mPackages.splice(mPackages.begin(), mPackages, itActualCombat);
                }
            }

            if (package->execute (actor,duration))
            {
                // To account for the rare case where AiPackage::execute() queued another AI package
                // (e.g. AiPursue executing a dialogue script that uses startCombat)
                std::list<MWMechanics::AiPackage*>::iterator toRemove =
                        std::find(mPackages.begin(), mPackages.end(), package);
                mPackages.erase(toRemove);
                delete package;
                mDone = true;
            }
            else
            {
                mDone = false;
            }
        }
    }
}

void AiSequence::clear()
{
    for (std::list<AiPackage *>::const_iterator iter (mPackages.begin()); iter!=mPackages.end(); ++iter)
        delete *iter;

    mPackages.clear();
}

void AiSequence::stack (const AiPackage& package, const MWWorld::Ptr& actor)
{
    if (package.getTypeId() == AiPackage::TypeIdCombat || package.getTypeId() == AiPackage::TypeIdPursue)
    {
        // Notify AiWander of our current position so we can return to it after combat finished
        for (std::list<AiPackage *>::const_iterator iter (mPackages.begin()); iter!=mPackages.end(); ++iter)
        {
            if((*iter)->getTypeId() == AiPackage::TypeIdPursue && package.getTypeId() == AiPackage::TypeIdPursue
                && static_cast<const AiPursue*>(*iter)->getTarget() == static_cast<const AiPursue*>(&package)->getTarget())
            {
                return; // target is already pursued
            }
            else if ((*iter)->getTypeId() == AiPackage::TypeIdWander)
                static_cast<AiWander*>(*iter)->setReturnPosition(Ogre::Vector3(actor.getRefData().getPosition().pos));
        }
    }

    for(std::list<AiPackage *>::iterator it = mPackages.begin(); it != mPackages.end(); ++it)
    {
        if(mPackages.front()->getPriority() <= package.getPriority())
        {
            mPackages.insert(it,package.clone());
            return;
        }
    }

    if(mPackages.empty())
        mPackages.push_front (package.clone());
}

void AiSequence::queue (const AiPackage& package)
{
    mPackages.push_back (package.clone());
}

AiPackage* MWMechanics::AiSequence::getActivePackage()
{
    if(mPackages.empty())
        throw std::runtime_error(std::string("No AI Package!"));
    else
        return mPackages.front();
}

void AiSequence::fill(const ESM::AIPackageList &list)
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
            package = new MWMechanics::AiWander(data.mDistance, data.mDuration, data.mTimeOfDay, idles, data.mShouldRepeat);
        }
        else if (it->mType == ESM::AI_Escort)
        {
            ESM::AITarget data = it->mTarget;
            MWWorld::Ptr target = MWBase::Environment::get().getWorld()->getPtr(data.mId.toString(), false);
            package = new MWMechanics::AiEscort(target, data.mDuration, data.mX, data.mY, data.mZ);
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
            MWWorld::Ptr target = MWBase::Environment::get().getWorld()->getPtr(data.mId.toString(), false);
            package = new MWMechanics::AiFollow(target, data.mDuration, data.mX, data.mY, data.mZ);
        }
        mPackages.push_back(package);
    }
}

} // namespace MWMechanics
