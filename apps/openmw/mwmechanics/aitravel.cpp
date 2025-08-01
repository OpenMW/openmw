#include "aitravel.hpp"

#include <algorithm>

#include <components/esm3/aisequence.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/class.hpp"

#include "character.hpp"
#include "creaturestats.hpp"
#include "greetingstate.hpp"
#include "movement.hpp"

namespace
{

    constexpr float TRAVEL_FINISH_TIME = 2.f;

    bool isWithinMaxRange(const osg::Vec3f& pos1, const osg::Vec3f& pos2)
    {
        // Maximum travel distance for vanilla compatibility.
        // Was likely meant to prevent NPCs walking into non-loaded exterior cells, but for some reason is used in
        // interior cells as well. We can make this configurable at some point, but the default *must* be the below
        // value. Anything else will break shoddily-written content (*cough* MW *cough*) in bizarre ways.
        return (pos1 - pos2).length2() <= 7168 * 7168;
    }

}

namespace MWMechanics
{
    AiTravel::AiTravel(float x, float y, float z, bool repeat, AiTravel*)
        : TypedAiPackage<AiTravel>(repeat)
        , mX(x)
        , mY(y)
        , mZ(z)
        , mHidden(false)
        , mDestinationTimer(TRAVEL_FINISH_TIME)
    {
    }

    AiTravel::AiTravel(float x, float y, float z, AiInternalTravel* derived)
        : TypedAiPackage<AiTravel>(derived)
        , mX(x)
        , mY(y)
        , mZ(z)
        , mHidden(true)
        , mDestinationTimer(TRAVEL_FINISH_TIME)
    {
    }

    AiTravel::AiTravel(float x, float y, float z, bool repeat)
        : AiTravel(x, y, z, repeat, this)
    {
    }

    AiTravel::AiTravel(const ESM::AiSequence::AiTravel* travel)
        : TypedAiPackage<AiTravel>(travel->mRepeat)
        , mX(travel->mData.mX)
        , mY(travel->mData.mY)
        , mZ(travel->mData.mZ)
        , mHidden(false)
        , mDestinationTimer(TRAVEL_FINISH_TIME)
    {
        // Hidden ESM::AiSequence::AiTravel package should be converted into MWMechanics::AiInternalTravel type
        assert(!travel->mHidden);
    }

    bool AiTravel::execute(
        const MWWorld::Ptr& actor, CharacterController& characterController, AiState& state, float duration)
    {
        MWBase::MechanicsManager* mechMgr = MWBase::Environment::get().getMechanicsManager();
        auto& stats = actor.getClass().getCreatureStats(actor);

        if (!stats.getMovementFlag(CreatureStats::Flag_ForceJump)
            && !stats.getMovementFlag(CreatureStats::Flag_ForceSneak)
            && (mechMgr->isTurningToPlayer(actor) || mechMgr->getGreetingState(actor) == GreetingState::InProgress))
            return false;

        const osg::Vec3f actorPos(actor.getRefData().getPosition().asVec3());
        const osg::Vec3f targetPos(mX, mY, mZ);

        stats.setMovementFlag(CreatureStats::Flag_Run, false);
        stats.setDrawState(DrawState::Nothing);

        // Note: we should cancel internal "return after combat" package, if original location is too far away
        if (!isWithinMaxRange(targetPos, actorPos))
            return mHidden;

        if (pathTo(actor, targetPos, duration, characterController.getSupportedMovementDirections()))
        {
            actor.getClass().getMovementSettings(actor).mPosition[1] = 0;
            return true;
        }

        // If we've been close enough to the destination for some time give up like Morrowind.
        // The end condition should be pretty much accurate.
        // FIXME: But the timing isn't. Right now we're being very generous,
        // but Morrowind might stop the actor prematurely under unclear conditions.

        // Note Morrowind uses the halved eye level, but this is close enough.
        float dist
            = distanceIgnoreZ(actorPos, targetPos) - MWBase::Environment::get().getWorld()->getHalfExtents(actor).z();
        const float endTolerance = std::max(64.f, actor.getClass().getCurrentSpeed(actor) * duration);

        // Even if we have entered the threshold, we might have been pushed away. Reset the timer if we're currently too
        // far.
        if (dist > endTolerance)
        {
            mDestinationTimer = TRAVEL_FINISH_TIME;
            return false;
        }

        mDestinationTimer -= duration;
        if (mDestinationTimer > 0)
            return false;

        actor.getClass().getMovementSettings(actor).mPosition[1] = 0;
        return true;
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

    void AiTravel::writeState(ESM::AiSequence::AiSequence& sequence) const
    {
        auto travel = std::make_unique<ESM::AiSequence::AiTravel>();
        travel->mData.mX = mX;
        travel->mData.mY = mY;
        travel->mData.mZ = mZ;
        travel->mHidden = mHidden;
        travel->mRepeat = getRepeat();

        ESM::AiSequence::AiPackageContainer package;
        package.mType = ESM::AiSequence::Ai_Travel;
        package.mPackage = std::move(travel);
        sequence.mPackages.push_back(std::move(package));
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
