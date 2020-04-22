#include "aisequence.hpp"

#include <limits>

#include <components/debug/debuglog.hpp>
#include <components/esm/aisequence.hpp>

#include "../mwbase/world.hpp"

#include "aipackage.hpp"
#include "aistate.hpp"
#include "aiwander.hpp"
#include "aiescort.hpp"
#include "aitravel.hpp"
#include "aifollow.hpp"
#include "aiactivate.hpp"
#include "aicombat.hpp"
#include "aicombataction.hpp"
#include "aipursue.hpp"
#include "actorutil.hpp"
#include "../mwworld/class.hpp"

namespace MWMechanics
{

void AiSequence::copy (const AiSequence& sequence)
{
    for (std::list<AiPackage *>::const_iterator iter (sequence.mPackages.begin());
        iter!=sequence.mPackages.end(); ++iter)
        mPackages.push_back ((*iter)->clone());

    // We need to keep an AiWander storage, if present - it has a state machine.
    // Not sure about another temporary storages
    sequence.mAiState.copy<AiWanderStorage>(mAiState);
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

bool AiSequence::isEngagedWithActor() const
{
    for (std::list<AiPackage *>::const_iterator it = mPackages.begin(); it != mPackages.end(); ++it)
    {
        if ((*it)->getTypeId() == AiPackage::TypeIdCombat)
        {
            MWWorld::Ptr target2 = (*it)->getTarget();
            if (!target2.isEmpty() && target2.getClass().isNpc())
                return true;
        }
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
    return (packageTypeId >= AiPackage::TypeIdWander &&
            packageTypeId <= AiPackage::TypeIdActivate);
}

void AiSequence::execute (const MWWorld::Ptr& actor, CharacterController& characterController, float duration, bool outOfRange)
{
    if(actor != getPlayer())
    {
        if (mPackages.empty())
        {
            mLastAiPackage = -1;
            return;
        }

        MWMechanics::AiPackage* package = mPackages.front();
        if (!package->alwaysActive() && outOfRange)
            return;

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

            float bestRating = 0.f;

            for(std::list<AiPackage *>::iterator it = mPackages.begin(); it != mPackages.end();)
            {
                if ((*it)->getTypeId() != AiPackage::TypeIdCombat) break;

                MWWorld::Ptr target = (*it)->getTarget();

                // target disappeared (e.g. summoned creatures)
                if (target.isEmpty())
                {
                    delete *it;
                    it = mPackages.erase(it);
                }
                else
                {
                    float rating = MWMechanics::getBestActionRating(actor, target);

                    const ESM::Position &targetPos = target.getRefData().getPosition();

                    float distTo = (targetPos.asVec3() - vActorPos).length2();

                    // Small threshold for changing target
                    if (it == mPackages.begin())
                        distTo = std::max(0.f, distTo - 2500.f);

                    // if a target has higher priority than current target or has same priority but closer
                    if (rating > bestRating || ((distTo < nearestDist) && rating == bestRating))
                    {
                        nearestDist = distTo;
                        itActualCombat = it;
                        bestRating = rating;
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
            if (package->execute (actor, characterController, mAiState, duration))
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
            Log(Debug::Error) << "Error during AiSequence::execute: " << e.what();
        }
    }
}

void AiSequence::clear()
{
    for (std::list<AiPackage *>::const_iterator iter (mPackages.begin()); iter!=mPackages.end(); ++iter)
        delete *iter;

    mPackages.clear();
}

void AiSequence::stack (const AiPackage& package, const MWWorld::Ptr& actor, bool cancelOther)
{
    if (actor == getPlayer())
        throw std::runtime_error("Can't add AI packages to player");

    // Stop combat when a non-combat AI package is added
    if (isActualAiPackage(package.getTypeId()))
        stopCombat();

    // We should return a wandering actor back after combat, casting or pursuit.
    // The same thing for actors without AI packages.
    // Also there is no point to stack return packages.
    int currentTypeId = getTypeId();
    int newTypeId = package.getTypeId();
    if (currentTypeId <= MWMechanics::AiPackage::TypeIdWander
        && !hasPackage(MWMechanics::AiPackage::TypeIdInternalTravel)
        && (newTypeId <= MWMechanics::AiPackage::TypeIdCombat
        || newTypeId == MWMechanics::AiPackage::TypeIdPursue
        || newTypeId == MWMechanics::AiPackage::TypeIdCast))
    {
        osg::Vec3f dest;
        if (currentTypeId == MWMechanics::AiPackage::TypeIdWander)
        {
            AiPackage* activePackage = getActivePackage();
            dest = activePackage->getDestination(actor);
        }
        else
        {
            dest = actor.getRefData().getPosition().asVec3();
        }

        MWMechanics::AiTravel travelPackage(dest.x(), dest.y(), dest.z(), true);
        stack(travelPackage, actor, false);
    }

    // remove previous packages if required
    if (cancelOther && package.shouldCancelPreviousAi())
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
        // We should keep current AiCast package, if we try to add a new one.
        if ((*it)->getTypeId() == MWMechanics::AiPackage::TypeIdCast &&
            package.getTypeId() == MWMechanics::AiPackage::TypeIdCast)
        {
            continue;
        }

        if((*it)->getPriority() <= package.getPriority())
        {
            mPackages.insert(it,package.clone());
            return;
        }
    }

    mPackages.push_back (package.clone());

    // Make sure that temporary storage is empty
    if (cancelOther)
    {
        mAiState.moveIn(new AiCombatStorage());
        mAiState.moveIn(new AiFollowStorage());
        mAiState.moveIn(new AiWanderStorage());
    }
}

bool MWMechanics::AiSequence::isEmpty() const
{
    return mPackages.empty();
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

    sequence.mLastAiPackage = mLastAiPackage;
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
        std::unique_ptr<MWMechanics::AiPackage> package;
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

    mLastAiPackage = sequence.mLastAiPackage;
}

void AiSequence::fastForward(const MWWorld::Ptr& actor)
{
    if (!mPackages.empty())
    {
        MWMechanics::AiPackage* package = mPackages.front();
        package->fastForward(actor, mAiState);
    }
}

} // namespace MWMechanics
