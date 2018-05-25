#include "aitravel.hpp"

#include <components/esm/aisequence.hpp>
#include <components/esm/loadcell.hpp>

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/cellstore.hpp"

#include "steering.hpp"
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
        ESM::Position pos = actor.getRefData().getPosition();

        actor.getClass().getCreatureStats(actor).setMovementFlag(CreatureStats::Flag_Run, false);
        actor.getClass().getCreatureStats(actor).setDrawState(DrawState_Nothing);

        if (!isWithinMaxRange(osg::Vec3f(mX, mY, mZ), pos.asVec3()))
            return false;

        if (pathTo(actor, ESM::Pathgrid::Point(static_cast<int>(mX), static_cast<int>(mY), static_cast<int>(mZ)), duration))
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

