#include "aitravel.hpp"

#include <algorithm>

#include <components/esm3/aisequence.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/cellstore.hpp"

#include "movement.hpp"
#include "creaturestats.hpp"

namespace
{

bool isWithinMaxRange(const osg::Vec3f& pos1, const osg::Vec3f& pos2)
{
    // Maximum travel distance for vanilla compatibility.
    // Was likely meant to prevent NPCs walking into non-loaded exterior cells, but for some reason is used in interior cells as well.
    // We can make this configurable at some point, but the default *must* be the below value. Anything else will break shoddily-written content (*cough* MW *cough*) in bizarre ways.
    return (pos1 - pos2).length2() <= 7168*7168;
}

    float getActorRadius(const MWWorld::ConstPtr& actor)
    {
        const osg::Vec3f halfExtents = MWBase::Environment::get().getWorld()->getPathfindingHalfExtents(actor);
        return std::max(halfExtents.x(), std::max(halfExtents.y(), halfExtents.z()));
    }
}

namespace MWMechanics
{
    AiTravel::AiTravel(float x, float y, float z, bool repeat, AiTravel*)
        : TypedAiPackage<AiTravel>(repeat), mX(x), mY(y), mZ(z), mHidden(false)
    {
    }

    AiTravel::AiTravel(float x, float y, float z, AiInternalTravel* derived)
        : TypedAiPackage<AiTravel>(derived), mX(x), mY(y), mZ(z), mHidden(true)
    {
    }

    AiTravel::AiTravel(float x, float y, float z, bool repeat)
        : AiTravel(x, y, z, repeat, this)
    {
    }

    AiTravel::AiTravel(const ESM::AiSequence::AiTravel *travel)
        : TypedAiPackage<AiTravel>(travel->mRepeat), mX(travel->mData.mX), mY(travel->mData.mY), mZ(travel->mData.mZ), mHidden(false)
    {
        // Hidden ESM::AiSequence::AiTravel package should be converted into MWMechanics::AiInternalTravel type
        assert(!travel->mHidden);
    }

    bool AiTravel::execute (const MWWorld::Ptr& actor, CharacterController& characterController, AiState& state, float duration)
    {
        MWBase::MechanicsManager* mechMgr = MWBase::Environment::get().getMechanicsManager();
        auto& stats = actor.getClass().getCreatureStats(actor);

        if (!stats.getMovementFlag(CreatureStats::Flag_ForceJump) && !stats.getMovementFlag(CreatureStats::Flag_ForceSneak)
                && (mechMgr->isTurningToPlayer(actor) || mechMgr->getGreetingState(actor) == Greet_InProgress))
            return false;

        const osg::Vec3f actorPos(actor.getRefData().getPosition().asVec3());
        const osg::Vec3f targetPos(mX, mY, mZ);

        stats.setMovementFlag(CreatureStats::Flag_Run, false);
        stats.setDrawState(DrawState_Nothing);

        // Note: we should cancel internal "return after combat" package, if original location is too far away
        if (!isWithinMaxRange(targetPos, actorPos))
            return mHidden;

        // Unfortunately, with vanilla assets destination is sometimes blocked by other actor.
        // If we got close to target, check for actors nearby. If they are, finish AI package.
        if (mDestinationCheck.update(duration) == Misc::TimerStatus::Elapsed)
        {
            std::vector<MWWorld::Ptr> occupyingActors;
            if (isAreaOccupiedByOtherActor(actor, targetPos, &occupyingActors))
            {
                const float actorRadius = getActorRadius(actor);
                const float distanceToTarget = distance(actorPos, targetPos);
                for (const MWWorld::Ptr& other : occupyingActors)
                {
                    const float otherRadius = getActorRadius(other);
                    const auto [minRadius, maxRadius] = std::minmax(actorRadius, otherRadius);
                    constexpr float toleranceFactor = 1.25;
                    if (minRadius * toleranceFactor + maxRadius > distanceToTarget)
                    {
                        actor.getClass().getMovementSettings(actor).mPosition[1] = 0;
                        return true;
                    }
                }
            }
        }

        if (pathTo(actor, targetPos, duration))
        {
            actor.getClass().getMovementSettings(actor).mPosition[1] = 0;
            return true;
        }
        return false;
    }

    void AiTravel::fastForward(const MWWorld::Ptr& actor, AiState& state)
    {
        osg::Vec3f pos(mX, mY, mZ);
        if (!isWithinMaxRange(pos, actor.getRefData().getPosition().asVec3()))
            return;
        // does not do any validation on the travel target (whether it's in air, inside collision geometry, etc),
        // that is the user's responsibility
        MWBase::Environment::get().getWorld()->moveObject(actor, pos);
        actor.getClass().adjustPosition(actor, false);
        reset();
    }

    void AiTravel::writeState(ESM::AiSequence::AiSequence &sequence) const
    {
        std::unique_ptr<ESM::AiSequence::AiTravel> travel(new ESM::AiSequence::AiTravel());
        travel->mData.mX = mX;
        travel->mData.mY = mY;
        travel->mData.mZ = mZ;
        travel->mHidden = mHidden;
        travel->mRepeat = getRepeat();

        ESM::AiSequence::AiPackageContainer package;
        package.mType = ESM::AiSequence::Ai_Travel;
        package.mPackage = travel.release();
        sequence.mPackages.push_back(package);
    }

    AiInternalTravel::AiInternalTravel(float x, float y, float z)
        : AiTravel(x, y, z, this)
    {
    }

    AiInternalTravel::AiInternalTravel(const ESM::AiSequence::AiTravel* travel)
        : AiTravel(travel->mData.mX, travel->mData.mY, travel->mData.mZ, this)
    {
    }

    std::unique_ptr<AiPackage> AiInternalTravel::clone() const
    {
        return std::make_unique<AiInternalTravel>(*this);
    }
}

