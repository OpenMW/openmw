#include "aisequence.hpp"

#include <algorithm>
#include <limits>

#include <components/debug/debuglog.hpp>
#include <components/esm3/aisequence.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwworld/class.hpp"
#include "actorutil.hpp"
#include "aiactivate.hpp"
#include "aicombat.hpp"
#include "aicombataction.hpp"
#include "aiescort.hpp"
#include "aifollow.hpp"
#include "aipackage.hpp"
#include "aipursue.hpp"
#include "aitravel.hpp"
#include "aiwander.hpp"
#include "creaturestats.hpp"

namespace MWMechanics
{

    void AiSequence::copy(const AiSequence& sequence)
    {
        for (const auto& package : sequence.mPackages)
            mPackages.push_back(package->clone());

        // We need to keep an AiWander storage, if present - it has a state machine.
        // Not sure about another temporary storages
        sequence.mAiState.copy<AiWanderStorage>(mAiState);

        mNumCombatPackages = sequence.mNumCombatPackages;
        mNumPursuitPackages = sequence.mNumPursuitPackages;
    }

    AiSequence::AiSequence()
        : mDone(false)
        , mLastAiPackage(AiPackageTypeId::None)
    {
    }

    AiSequence::AiSequence(const AiSequence& sequence)
    {
        copy(sequence);
        mDone = sequence.mDone;
        mLastAiPackage = sequence.mLastAiPackage;
    }

    AiSequence& AiSequence::operator=(const AiSequence& sequence)
    {
        if (this != &sequence)
        {
            clear();
            copy(sequence);
            mDone = sequence.mDone;
            mLastAiPackage = sequence.mLastAiPackage;
        }

        return *this;
    }

    AiSequence::~AiSequence()
    {
        clear();
    }

    void AiSequence::onPackageAdded(const AiPackage& package)
    {
        if (package.getTypeId() == AiPackageTypeId::Combat)
            mNumCombatPackages++;
        else if (package.getTypeId() == AiPackageTypeId::Pursue)
            mNumPursuitPackages++;

        assert(mNumCombatPackages >= 0);
        assert(mNumPursuitPackages >= 0);
    }

    void AiSequence::onPackageRemoved(const AiPackage& package)
    {
        if (package.getTypeId() == AiPackageTypeId::Combat)
        {
            mNumCombatPackages--;
            if (mNumCombatPackages == 0)
                mResetFriendlyHits = true;
        }
        else if (package.getTypeId() == AiPackageTypeId::Pursue)
            mNumPursuitPackages--;

        assert(mNumCombatPackages >= 0);
        assert(mNumPursuitPackages >= 0);
    }

    AiPackageTypeId AiSequence::getTypeId() const
    {
        if (mPackages.empty())
            return AiPackageTypeId::None;

        return mPackages.front()->getTypeId();
    }

    bool AiSequence::getCombatTarget(MWWorld::Ptr& targetActor) const
    {
        if (getTypeId() != AiPackageTypeId::Combat)
            return false;

        targetActor = mPackages.front()->getTarget();

        return !targetActor.isEmpty();
    }

    bool AiSequence::getCombatTargets(std::vector<MWWorld::Ptr>& targetActors) const
    {
        for (auto it = mPackages.begin(); it != mPackages.end(); ++it)
        {
            if ((*it)->getTypeId() == MWMechanics::AiPackageTypeId::Combat)
                targetActors.push_back((*it)->getTarget());
        }

        return !targetActors.empty();
    }

    AiPackages::iterator AiSequence::erase(AiPackages::iterator package)
    {
        // Not sure if manually terminated packages should trigger mDone, probably not?
        auto& ptr = *package;
        onPackageRemoved(*ptr);

        return mPackages.erase(package);
    }

    bool AiSequence::isInCombat() const
    {
        return mNumCombatPackages > 0;
    }

    bool AiSequence::isInPursuit() const
    {
        return mNumPursuitPackages > 0;
    }

    bool AiSequence::isFleeing() const
    {
        if (!isInCombat())
            return false;

        const AiCombatStorage* storage = mAiState.getPtr<AiCombatStorage>();
        return storage && storage->isFleeing();
    }

    bool AiSequence::isEngagedWithActor() const
    {
        if (!isInCombat())
            return false;

        for (auto it = mPackages.begin(); it != mPackages.end(); ++it)
        {
            if ((*it)->getTypeId() == AiPackageTypeId::Combat)
            {
                MWWorld::Ptr target2 = (*it)->getTarget();
                if (!target2.isEmpty() && target2.getClass().isNpc())
                    return true;
            }
        }
        return false;
    }

    bool AiSequence::hasPackage(AiPackageTypeId typeId) const
    {
        auto it = std::find_if(mPackages.begin(), mPackages.end(),
            [typeId](const auto& package) { return package->getTypeId() == typeId; });
        return it != mPackages.end();
    }

    bool AiSequence::isInCombat(const MWWorld::Ptr& actor) const
    {
        if (!isInCombat())
            return false;

        for (const auto& package : mPackages)
        {
            if (package->getTypeId() == AiPackageTypeId::Combat)
            {
                if (package->targetIs(actor))
                    return true;
            }
        }
        return false;
    }

    void AiSequence::removePackagesById(AiPackageTypeId id)
    {
        for (auto it = mPackages.begin(); it != mPackages.end();)
        {
            if ((*it)->getTypeId() == id)
            {
                it = erase(it);
            }
            else
                ++it;
        }
    }

    void AiSequence::stopCombat()
    {
        removePackagesById(AiPackageTypeId::Combat);
    }

    void AiSequence::stopCombat(const std::vector<MWWorld::Ptr>& targets)
    {
        for (auto it = mPackages.begin(); it != mPackages.end();)
        {
            if ((*it)->getTypeId() == AiPackageTypeId::Combat
                && std::find(targets.begin(), targets.end(), (*it)->getTarget()) != targets.end())
            {
                it = erase(it);
            }
            else
                ++it;
        }
    }

    void AiSequence::stopPursuit()
    {
        removePackagesById(AiPackageTypeId::Pursue);
    }

    bool AiSequence::isPackageDone() const
    {
        return mDone;
    }

    namespace
    {
        bool isActualAiPackage(AiPackageTypeId packageTypeId)
        {
            return (packageTypeId >= AiPackageTypeId::Wander && packageTypeId <= AiPackageTypeId::Activate);
        }
    }

    void AiSequence::execute(
        const MWWorld::Ptr& actor, CharacterController& characterController, float duration, bool outOfRange)
    {
        if (actor == getPlayer())
        {
            // Players don't use this.
            return;
        }

        if (mResetFriendlyHits)
        {
            actor.getClass().getCreatureStats(actor).resetFriendlyHits();
            mResetFriendlyHits = false;
        }

        if (mPackages.empty())
        {
            mLastAiPackage = AiPackageTypeId::None;
            return;
        }

        auto* package = mPackages.front().get();
        if (!package->alwaysActive() && outOfRange)
            return;

        auto packageTypeId = package->getTypeId();
        // workaround ai packages not being handled as in the vanilla engine
        if (isActualAiPackage(packageTypeId))
            mLastAiPackage = packageTypeId;
        // if active package is combat one, choose nearest target
        if (packageTypeId == AiPackageTypeId::Combat)
        {
            auto itActualCombat = mPackages.end();

            float nearestDist = std::numeric_limits<float>::max();
            osg::Vec3f vActorPos = actor.getRefData().getPosition().asVec3();

            float bestRating = 0.f;

            for (auto it = mPackages.begin(); it != mPackages.end();)
            {
                if ((*it)->getTypeId() != AiPackageTypeId::Combat)
                    break;

                MWWorld::Ptr target = (*it)->getTarget();

                // target disappeared (e.g. summoned creatures)
                if (target.isEmpty())
                {
                    it = erase(it);
                }
                else
                {
                    float rating = 0.f;
                    if (MWMechanics::canFight(actor, target))
                        rating = MWMechanics::getBestActionRating(actor, target);

                    const ESM::Position& targetPos = target.getRefData().getPosition();

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

            if (mPackages.empty())
                return;

            if (nearestDist < std::numeric_limits<float>::max() && mPackages.begin() != itActualCombat)
            {
                assert(itActualCombat != mPackages.end());
                // move combat package with nearest target to the front
                std::rotate(mPackages.begin(), itActualCombat, std::next(itActualCombat));
            }

            package = mPackages.front().get();
            packageTypeId = package->getTypeId();
        }

        try
        {
            if (package->execute(actor, characterController, mAiState, duration))
            {
                // Put repeating non-combat AI packages on the end of the stack so they can be used again
                if (isActualAiPackage(packageTypeId) && package->getRepeat())
                {
                    package->reset();
                    mPackages.push_back(package->clone());
                }

                // The active package is typically the first entry, this is however not always the case
                // e.g. AiPursue executing a dialogue script that uses startCombat adds a combat package to the front
                // due to the priority.
                auto activePackageIt = std::find_if(
                    mPackages.begin(), mPackages.end(), [&](auto& entry) { return entry.get() == package; });

                erase(activePackageIt);

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

    void AiSequence::clear()
    {
        mPackages.clear();
        mNumCombatPackages = 0;
        mNumPursuitPackages = 0;
    }

    void AiSequence::stack(const AiPackage& package, const MWWorld::Ptr& actor, bool cancelOther)
    {
        if (actor == getPlayer())
            throw std::runtime_error("Can't add AI packages to player");

        // Stop combat when a non-combat AI package is added
        if (isActualAiPackage(package.getTypeId()))
        {
            if (package.getTypeId() == MWMechanics::AiPackageTypeId::Follow
                || package.getTypeId() == MWMechanics::AiPackageTypeId::Escort)
            {
                const auto& mechanicsManager = MWBase::Environment::get().getMechanicsManager();
                std::vector<MWWorld::Ptr> newAllies = mechanicsManager->getActorsSidingWith(package.getTarget());
                std::vector<MWWorld::Ptr> allies = mechanicsManager->getActorsSidingWith(actor);
                for (const auto& ally : allies)
                    ally.getClass().getCreatureStats(ally).getAiSequence().stopCombat(newAllies);
                for (const auto& ally : newAllies)
                    ally.getClass().getCreatureStats(ally).getAiSequence().stopCombat(allies);
            }
            stopCombat();
        }

        // We should return a wandering actor back after combat, casting or pursuit.
        // The same thing for actors without AI packages.
        // Also there is no point to stack return packages.
        const auto currentTypeId = getTypeId();
        const auto newTypeId = package.getTypeId();
        if (currentTypeId <= MWMechanics::AiPackageTypeId::Wander
            && !hasPackage(MWMechanics::AiPackageTypeId::InternalTravel)
            && (newTypeId <= MWMechanics::AiPackageTypeId::Combat || newTypeId == MWMechanics::AiPackageTypeId::Pursue
                || newTypeId == MWMechanics::AiPackageTypeId::Cast))
        {
            osg::Vec3f dest;
            if (currentTypeId == MWMechanics::AiPackageTypeId::Wander)
            {
                dest = getActivePackage().getDestination(actor);
            }
            else
            {
                dest = actor.getRefData().getPosition().asVec3();
            }

            MWMechanics::AiInternalTravel travelPackage(dest.x(), dest.y(), dest.z());
            stack(travelPackage, actor, false);
        }

        // remove previous packages if required
        if (cancelOther && package.shouldCancelPreviousAi())
        {
            for (auto it = mPackages.begin(); it != mPackages.end();)
            {
                if ((*it)->canCancel())
                {
                    it = erase(it);
                }
                else
                    ++it;
            }
        }

        // insert new package in correct place depending on priority
        for (auto it = mPackages.begin(); it != mPackages.end(); ++it)
        {
            // We should override current AiCast package, if we try to add a new one.
            if ((*it)->getTypeId() == MWMechanics::AiPackageTypeId::Cast
                && package.getTypeId() == MWMechanics::AiPackageTypeId::Cast)
            {
                *it = package.clone();
                return;
            }

            if ((*it)->getPriority() <= package.getPriority())
            {
                onPackageAdded(package);
                mPackages.insert(it, package.clone());
                return;
            }
        }

        onPackageAdded(package);
        mPackages.push_back(package.clone());

        // Make sure that temporary storage is empty
        if (cancelOther)
        {
            mAiState.moveIn(std::make_unique<AiCombatStorage>());
            mAiState.moveIn(std::make_unique<AiFollowStorage>());
            mAiState.moveIn(std::make_unique<AiWanderStorage>());
        }
    }

    bool MWMechanics::AiSequence::isEmpty() const
    {
        return mPackages.empty();
    }

    const AiPackage& MWMechanics::AiSequence::getActivePackage() const
    {
        if (mPackages.empty())
            throw std::runtime_error(std::string("No AI Package!"));
        return *mPackages.front();
    }

    void AiSequence::fill(const ESM::AIPackageList& list)
    {
        for (const auto& esmPackage : list.mList)
        {
            std::unique_ptr<MWMechanics::AiPackage> package;
            if (esmPackage.mType == ESM::AI_Wander)
            {
                ESM::AIWander data = esmPackage.mWander;
                std::vector<unsigned char> idles;
                idles.reserve(8);
                for (int i = 0; i < 8; ++i)
                    idles.push_back(data.mIdle[i]);
                package = std::make_unique<MWMechanics::AiWander>(
                    data.mDistance, data.mDuration, data.mTimeOfDay, idles, data.mShouldRepeat != 0);
            }
            else if (esmPackage.mType == ESM::AI_Escort)
            {
                ESM::AITarget data = esmPackage.mTarget;
                package = std::make_unique<MWMechanics::AiEscort>(ESM::RefId::stringRefId(data.mId.toStringView()),
                    esmPackage.mCellName, data.mDuration, data.mX, data.mY, data.mZ, data.mShouldRepeat != 0);
            }
            else if (esmPackage.mType == ESM::AI_Travel)
            {
                ESM::AITravel data = esmPackage.mTravel;
                package = std::make_unique<MWMechanics::AiTravel>(data.mX, data.mY, data.mZ, data.mShouldRepeat != 0);
            }
            else if (esmPackage.mType == ESM::AI_Activate)
            {
                ESM::AIActivate data = esmPackage.mActivate;
                package = std::make_unique<MWMechanics::AiActivate>(
                    ESM::RefId::stringRefId(data.mName.toStringView()), data.mShouldRepeat != 0);
            }
            else // if (esmPackage.mType == ESM::AI_Follow)
            {
                ESM::AITarget data = esmPackage.mTarget;
                package = std::make_unique<MWMechanics::AiFollow>(ESM::RefId::stringRefId(data.mId.toStringView()),
                    esmPackage.mCellName, data.mDuration, data.mX, data.mY, data.mZ, data.mShouldRepeat != 0);
            }

            onPackageAdded(*package);
            mPackages.push_back(std::move(package));
        }
    }

    void AiSequence::writeState(ESM::AiSequence::AiSequence& sequence) const
    {
        for (const auto& package : mPackages)
            package->writeState(sequence);

        sequence.mLastAiPackage = static_cast<int>(mLastAiPackage);
    }

    void AiSequence::readState(const ESM::AiSequence::AiSequence& sequence)
    {
        if (!sequence.mPackages.empty())
            clear();

        // Load packages
        for (auto& container : sequence.mPackages)
        {
            std::unique_ptr<MWMechanics::AiPackage> package;
            switch (container.mType)
            {
                case ESM::AiSequence::Ai_Wander:
                {
                    package = std::make_unique<AiWander>(
                        &static_cast<const ESM::AiSequence::AiWander&>(*container.mPackage));
                    break;
                }
                case ESM::AiSequence::Ai_Travel:
                {
                    const ESM::AiSequence::AiTravel& source
                        = static_cast<const ESM::AiSequence::AiTravel&>(*container.mPackage);
                    if (source.mHidden)
                        package = std::make_unique<AiInternalTravel>(&source);
                    else
                        package = std::make_unique<AiTravel>(&source);
                    break;
                }
                case ESM::AiSequence::Ai_Escort:
                {
                    package = std::make_unique<AiEscort>(
                        &static_cast<const ESM::AiSequence::AiEscort&>(*container.mPackage));
                    break;
                }
                case ESM::AiSequence::Ai_Follow:
                {
                    package = std::make_unique<AiFollow>(
                        &static_cast<const ESM::AiSequence::AiFollow&>(*container.mPackage));
                    break;
                }
                case ESM::AiSequence::Ai_Activate:
                {
                    package = std::make_unique<AiActivate>(
                        &static_cast<const ESM::AiSequence::AiActivate&>(*container.mPackage));
                    break;
                }
                case ESM::AiSequence::Ai_Combat:
                {
                    package = std::make_unique<AiCombat>(
                        &static_cast<const ESM::AiSequence::AiCombat&>(*container.mPackage));
                    break;
                }
                case ESM::AiSequence::Ai_Pursue:
                {
                    package = std::make_unique<AiPursue>(
                        &static_cast<const ESM::AiSequence::AiPursue&>(*container.mPackage));
                    break;
                }
                default:
                    break;
            }

            if (!package.get())
                continue;

            onPackageAdded(*package);
            mPackages.push_back(std::move(package));
        }

        mLastAiPackage = static_cast<AiPackageTypeId>(sequence.mLastAiPackage);
    }

    void AiSequence::fastForward(const MWWorld::Ptr& actor)
    {
        if (!mPackages.empty())
        {
            mPackages.front()->fastForward(actor, mAiState);
        }
    }

} // namespace MWMechanics
