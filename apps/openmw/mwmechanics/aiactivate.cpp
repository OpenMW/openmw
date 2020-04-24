#include "aiactivate.hpp"

#include <components/esm/aisequence.hpp>

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"

#include "../mwworld/class.hpp"

#include "creaturestats.hpp"
#include "movement.hpp"
#include "steering.hpp"

namespace MWMechanics
{
    AiActivate::AiActivate(const std::string &objectId)
        : mObjectId(objectId)
    {
    }

    AiActivate *MWMechanics::AiActivate::clone() const
    {
        return new AiActivate(*this);
    }

    bool AiActivate::execute(const MWWorld::Ptr& actor, CharacterController& characterController, AiState& state, float duration)
    {
        const MWWorld::Ptr target = MWBase::Environment::get().getWorld()->searchPtr(mObjectId, false); //The target to follow

        actor.getClass().getCreatureStats(actor).setDrawState(DrawState_Nothing);

        // Stop if the target doesn't exist
        // Really we should be checking whether the target is currently registered with the MechanicsManager
        if (target == MWWorld::Ptr() || !target.getRefData().getCount() || !target.getRefData().isEnabled())
            return true;

        // Turn to target and move to it directly, without pathfinding.
        const osg::Vec3f targetDir = target.getRefData().getPosition().asVec3() - actor.getRefData().getPosition().asVec3();

        zTurn(actor, std::atan2(targetDir.x(), targetDir.y()), 0.f);
        actor.getClass().getMovementSettings(actor).mPosition[1] = 1;
        actor.getClass().getMovementSettings(actor).mPosition[0] = 0;

        if (MWBase::Environment::get().getWorld()->getMaxActivationDistance() >= targetDir.length())
        {
            // Note: we intentionally do not cancel package after activation here for backward compatibility with original engine.
            MWBase::Environment::get().getWorld()->activate(target, actor);
        }
        return false;
    }

    int AiActivate::getTypeId() const
    {
        return TypeIdActivate;
    }

    void AiActivate::writeState(ESM::AiSequence::AiSequence &sequence) const
    {
        std::unique_ptr<ESM::AiSequence::AiActivate> activate(new ESM::AiSequence::AiActivate());
        activate->mTargetId = mObjectId;

        ESM::AiSequence::AiPackageContainer package;
        package.mType = ESM::AiSequence::Ai_Activate;
        package.mPackage = activate.release();
        sequence.mPackages.push_back(package);
    }

    AiActivate::AiActivate(const ESM::AiSequence::AiActivate *activate)
        : mObjectId(activate->mTargetId)
    {
    }
}
