#include "aisequence.hpp"

#include <limits>
#include <iostream>

#include <components/esm/aisequence.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "aipackage.hpp"
#include "aistate.hpp"
#include "aiwander.hpp"
#include "aiescort.hpp"
#include "aitravel.hpp"
#include "aifollow.hpp"
#include "aiactivate.hpp"
#include "aicombat.hpp"
#include "aipursue.hpp"
#include "actorutil.hpp"

namespace MWMechanics
{

void AiSequence::copy (const AiSequence& sequence)
{
    for (std::list<AiPackage *>::const_iterator iter (sequence.mPackages.begin());
        iter!=sequence.mPackages.end(); ++iter)
        mPackages.push_back ((*iter)->clone());
}

AiSequence::AiSequence() : mDone (false), mRepeat(false), mLastAiPackage(-1) {}

AiSequence::AiSequence (const AiSequence& sequence)
{
    copy (sequence);
    mDone = sequence.mDone;
    mLastAiPackage = sequence.mLastAiPackage;
    mRepeat = sequence.mRepeat;
}

AiSequence& AiSequence::operator= (const AiSequence& sequence)
{
    if (this!=&sequence)
    {
        clear();
        copy (sequence);
        mDone = sequence.mDone;
        mLastAiPackage = sequence.mLastAiPackage;
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

bool AiSequence::getCombatTarget(MWWorld::Ptr &targetActor) const
{
    if (getTypeId() != AiPackage::TypeIdCombat)
        return false;
    
    targetActor = mPackages.front()->getTarget();

    return !targetActor.isEmpty();
}

bool AiSequence::getCombatTargets(std::vector<MWWorld::Ptr> &targetActors) const
{
    for (std::list<AiPackage*>::const_iterator it = mPackages.begin(); it != mPackages.end(); ++it)
    {
        if ((*it)->getTypeId() == MWMechanics::AiPackage::TypeIdCombat)
            targetActors.push_back((*it)->getTarget());
    }

    return !targetActors.empty();
}

std::list<AiPackage*>::const_iterator AiSequence::begin() const
{
    return mPackages.begin();
}

std::list<AiPackage*>::const_iterator AiSequence::end() const
{
    return mPackages.end();
}

void AiSequence::erase(std::list<AiPackage*>::const_iterator package)
{
    // Not sure if manually terminated packages should trigger mDone, probably not?
    for(std::list<AiPackage*>::iterator it = mPackages.begin(); it != mPackages.end(); ++it)
    {
        if (package == it)
        {
            delete *it;
            mPackages.erase(it);
            return;
        }
    }
    throw std::runtime_error("can't find package to erase");
}

bool AiSequence::isInCombat() const
{
    for(std::list<AiPackage*>::const_iterator it = mPackages.begin(); it != mPackages.end(); ++it)
    {
        if ((*it)->getTypeId() == AiPackage::TypeIdCombat)
            return true;
    }
    return false;
}

bool AiSequence::hasPackage(int typeId) const
{
    for (std::list<AiPackage*>::const_iterator it = mPackages.begin(); it != mPackages.end(); ++it)
    {
        if ((*it)->getTypeId() == typeId)
            return true;
    }
    return false;
}

bool AiSequence::isInCombat(const MWWorld::Ptr &actor) const
{
    for(std::list<AiPackage*>::const_iterator it = mPackages.begin(); it != mPackages.end(); ++it)
    {
        if ((*it)->getTypeId() == AiPackage::TypeIdCombat)
        {
            if ((*it)->getTarget() == actor)
                return true;
        }
    }
    return false;
}

void AiSequence::stopCombat()
{
    for(std::list<AiPackage*>::iterator it = mPackages.begin(); it != mPackages.end(); )
    {
        if ((*it)->getTypeId() == AiPackage::TypeIdCombat)
        {
            delete *it;
            it = mPackages.erase(it);
        }
        else
            ++it;
    }
}

void AiSequence::stopPursuit()
{
    for(std::list<AiPackage*>::iterator it = mPackages.begin(); it != mPackages.end(); )
    {
        if ((*it)->getTypeId() == AiPackage::TypeIdPursue)
        {
            delete *it;
            it = mPackages.erase(it);
        }
        else
            ++it;
    }
}

bool AiSequence::isPackageDone() const
{
    return mDone;
}

bool isActualAiPackage(int packageTypeId)
{
    return (packageTypeId != AiPackage::TypeIdCombat
                   && packageTypeId != AiPackage::TypeIdPursue
                   && packageTypeId != AiPackage::TypeIdAvoidDoor
                   && packageTypeId != AiPackage::TypeIdFace);
}

void AiSequence::execute (const MWWorld::Ptr& actor, CharacterController& characterController, AiState& state, float duration)
{
    if(actor != getPlayer())
    {
        if (mPackages.empty())
        {
            mLastAiPackage = -1;
            return;
        }

        MWMechanics::AiPackage* package = mPackages.front();
        int packageTypeId = package->getTypeId();
        // workaround ai packages not being handled as in the vanilla engine
        if (isActualAiPackage(packageTypeId))
            mLastAiPackage = packageTypeId;
        // if active package is combat one, choose nearest target
        if (packageTypeId == AiPackage::TypeIdCombat)
        {
            std::list<AiPackage *>::iterator itActualCombat;

            float nearestDist = std::numeric_limits<float>::max();
            osg::Vec3f vActorPos = actor.getRefData().getPosition().asVec3();

            for(std::list<AiPackage *>::iterator it = mPackages.begin(); it != mPackages.end();)
            {
                if ((*it)->getTypeId() != AiPackage::TypeIdCombat) break;

                MWWorld::Ptr target = static_cast<const AiCombat *>(*it)->getTarget();

                // target disappeared (e.g. summoned creatures)
                if (target.isEmpty())
                {
                    delete *it;
                    it = mPackages.erase(it);
                }
                else
                {
                    const ESM::Position &targetPos = target.getRefData().getPosition();

                    float distTo = (targetPos.asVec3() - vActorPos).length();

                    // Small threshold for changing target
                    if (it == mPackages.begin())
                        distTo = std::max(0.f, distTo - 50.f);

                    if (distTo < nearestDist)
                    {
                        nearestDist = distTo;
                        itActualCombat = it;
                    }
                    ++it;
                }
            }

            if (!mPackages.empty())
            {
                if (nearestDist < std::numeric_limits<float>::max() && mPackages.begin() != itActualCombat)
                {
                    // move combat package with nearest target to the front
                    mPackages.splice(mPackages.begin(), mPackages, itActualCombat);
                }

                package = mPackages.front();
            }
        }

        try
        {
            if (package->execute (actor,characterController,state,duration))
            {
                // Put repeating noncombat AI packages on the end of the stack so they can be used again
                if (isActualAiPackage(packageTypeId) && (mRepeat || package->getRepeat()))
                {
                    package->reset();
                    mPackages.push_back(package->clone());
                }
                // To account for the rare case where AiPackage::execute() queued another AI package
                // (e.g. AiPursue executing a dialogue script that uses startCombat)
                std::list<MWMechanics::AiPackage*>::iterator toRemove =
                        std::find(mPackages.begin(), mPackages.end(), package);
                mPackages.erase(toRemove);
                delete package;
                if (isActualAiPackage(packageTypeId))
                    mDone = true;
            }
            else
            {
                mDone = false;
            }
        }
        catch (std::exception& e)
        {
            std::cerr << "Error during AiSequence::execute: " << e.what() << std::endl;
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
    if (actor == getPlayer())
        throw std::runtime_error("Can't add AI packages to player");

    // Stop combat when a non-combat AI package is added
    if (isActualAiPackage(package.getTypeId()))
        stopCombat();

    // remove previous packages if required
    if (package.shouldCancelPreviousAi())
    {
        for(std::list<AiPackage *>::iterator it = mPackages.begin(); it != mPackages.end();)
        {
            if((*it)->canCancel())
            {
                delete *it;
                it = mPackages.erase(it);
            }
            else
                ++it;
        }
        mRepeat=false;
    }

    // insert new package in correct place depending on priority
    for(std::list<AiPackage *>::iterator it = mPackages.begin(); it != mPackages.end(); ++it)
    {
        if((*it)->getPriority() <= package.getPriority())
        {
            mPackages.insert(it,package.clone());
            return;
        }
    }

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
    // If there is more than one package in the list, enable repeating
    if (!list.mList.empty() && list.mList.begin() != (list.mList.end()-1))
        mRepeat = true;

    for (std::vector<ESM::AIPackage>::const_iterator it = list.mList.begin(); it != list.mList.end(); ++it)
    {
        MWMechanics::AiPackage* package;
        if (it->mType == ESM::AI_Wander)
        {
            ESM::AIWander data = it->mWander;
            std::vector<unsigned char> idles;
            idles.reserve(8);
            for (int i=0; i<8; ++i)
                idles.push_back(data.mIdle[i]);
            package = new MWMechanics::AiWander(data.mDistance, data.mDuration, data.mTimeOfDay, idles, data.mShouldRepeat != 0);
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

void AiSequence::writeState(ESM::AiSequence::AiSequence &sequence) const
{
    for (std::list<AiPackage *>::const_iterator iter (mPackages.begin()); iter!=mPackages.end(); ++iter)
    {
        (*iter)->writeState(sequence);
    }
}

void AiSequence::readState(const ESM::AiSequence::AiSequence &sequence)
{
    if (!sequence.mPackages.empty())
        clear();

    // If there is more than one non-combat, non-pursue package in the list, enable repeating.
    int count = 0;
    for (std::vector<ESM::AiSequence::AiPackageContainer>::const_iterator it = sequence.mPackages.begin();
         it != sequence.mPackages.end(); ++it)
    {    
        if (isActualAiPackage(it->mType))
            count++;
    }

    if (count > 1)
        mRepeat = true;

    // Load packages
    for (std::vector<ESM::AiSequence::AiPackageContainer>::const_iterator it = sequence.mPackages.begin();
         it != sequence.mPackages.end(); ++it)
    {
        std::unique_ptr<MWMechanics::AiPackage> package (nullptr);
        switch (it->mType)
        {
        case ESM::AiSequence::Ai_Wander:
        {
            package.reset(new AiWander(static_cast<ESM::AiSequence::AiWander*>(it->mPackage)));
            break;
        }
        case ESM::AiSequence::Ai_Travel:
        {
            package.reset(new AiTravel(static_cast<ESM::AiSequence::AiTravel*>(it->mPackage)));
            break;
        }
        case ESM::AiSequence::Ai_Escort:
        {
            package.reset(new AiEscort(static_cast<ESM::AiSequence::AiEscort*>(it->mPackage)));
            break;
        }
        case ESM::AiSequence::Ai_Follow:
        {
            package.reset(new AiFollow(static_cast<ESM::AiSequence::AiFollow*>(it->mPackage)));
            break;
        }
        case ESM::AiSequence::Ai_Activate:
        {
            package.reset(new AiActivate(static_cast<ESM::AiSequence::AiActivate*>(it->mPackage)));
            break;
        }
        case ESM::AiSequence::Ai_Combat:
        {
            package.reset(new AiCombat(static_cast<ESM::AiSequence::AiCombat*>(it->mPackage)));
            break;
        }
        case ESM::AiSequence::Ai_Pursue:
        {
            package.reset(new AiPursue(static_cast<ESM::AiSequence::AiPursue*>(it->mPackage)));
            break;
        }
        default:
            break;
        }

        if (!package.get())
            continue;

        mPackages.push_back(package.release());
    }
}

void AiSequence::fastForward(const MWWorld::Ptr& actor, AiState& state)
{
    if (!mPackages.empty())
    {
        MWMechanics::AiPackage* package = mPackages.front();
        package->fastForward(actor, state);
    }
}

} // namespace MWMechanics
