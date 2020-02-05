#include "aitravel.hpp"

#include <components/esm/aisequence.hpp>

#include "../mwbase/environment.hpp"
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

}

namespace MWMechanics
{
    AiTravel::AiTravel(float x, float y, float z, bool hidden)
    : mX(x),mY(y),mZ(z),mHidden(hidden)
    {
    }

    AiTravel::AiTravel(const ESM::AiSequence::AiTravel *travel)
        : mX(travel->mData.mX), mY(travel->mData.mY), mZ(travel->mData.mZ), mHidden(travel->mHidden)
    {
    }

    AiTravel *MWMechanics::AiTravel::clone() const
    {
        return new AiTravel(*this);
    }

    bool AiTravel::execute (const MWWorld::Ptr& actor, CharacterController& characterController, AiState& state, float duration)
    {
        auto& stats = actor.getClass().getCreatureStats(actor);

        if (stats.isTurningToPlayer() || stats.getGreetingState() == Greet_InProgress)
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
        int destinationTolerance = 64;
        if (distance(actorPos, targetPos) <= destinationTolerance)
        {
            std::vector<MWWorld::Ptr> targetActors;
            std::pair<MWWorld::Ptr, osg::Vec3f> result = MWBase::Environment::get().getWorld()->getHitContact(actor, destinationTolerance, targetActors);

            if (!result.first.isEmpty())
            {
                actor.getClass().getMovementSettings(actor).mPosition[1] = 0;
                return true;
            }
        }

        if (pathTo(actor, targetPos, duration))
        {
            actor.getClass().getMovementSettings(actor).mPosition[1] = 0;
            return true;
        }
        return false;
    }

    int AiTravel::getTypeId() const
    {
        return mHidden ? TypeIdInternalTravel : TypeIdTravel;
    }

    void AiTravel::fastForward(const MWWorld::Ptr& actor, AiState& state)
    {
        if (!isWithinMaxRange(osg::Vec3f(mX, mY, mZ), actor.getRefData().getPosition().asVec3()))
            return;
        // does not do any validation on the travel target (whether it's in air, inside collision geometry, etc),
        // that is the user's responsibility
        MWBase::Environment::get().getWorld()->moveObject(actor, mX, mY, mZ);
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

        ESM::AiSequence::AiPackageContainer package;
        package.mType = ESM::AiSequence::Ai_Travel;
        package.mPackage = travel.release();
        sequence.mPackages.push_back(package);
    }
}

