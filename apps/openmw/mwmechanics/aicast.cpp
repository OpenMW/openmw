#include "aicast.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/class.hpp"

#include "aicombataction.hpp"
#include "creaturestats.hpp"
#include "steering.hpp"

MWMechanics::AiCast::AiCast(const std::string& targetId, const std::string& spellId, bool manualSpell)
    : mTargetId(targetId), mSpellId(spellId), mCasting(false), mManual(manualSpell), mDistance(0)
{
    ActionSpell action = ActionSpell(spellId);
    bool isRanged;
    mDistance = action.getCombatRange(isRanged);
}

MWMechanics::AiPackage *MWMechanics::AiCast::clone() const
{
    return new AiCast(*this);
}

bool MWMechanics::AiCast::execute(const MWWorld::Ptr& actor, MWMechanics::CharacterController& characterController, MWMechanics::AiState& state, float duration)
{
    MWWorld::Ptr target;
    if (actor.getCellRef().getRefId() == mTargetId)
    {
        // If the target has the same ID as caster, consider that actor casts spell with Self range.
        target = actor;
    }
    else
    {
        target = getTarget();
        if (!target)
            return true;

        if (!mManual && !pathTo(actor, target.getRefData().getPosition().asVec3(), duration, mDistance))
        {
            return false;
        }

        osg::Vec3f targetPos = target.getRefData().getPosition().asVec3();
        if (target.getClass().isActor())
        {
            osg::Vec3f halfExtents = MWBase::Environment::get().getWorld()->getHalfExtents(target);
            targetPos.z() += halfExtents.z() * 2 * 0.75f;
        }

        osg::Vec3f actorPos = actor.getRefData().getPosition().asVec3();
        osg::Vec3f halfExtents = MWBase::Environment::get().getWorld()->getHalfExtents(actor);
        actorPos.z() += halfExtents.z() * 2 * 0.75f;

        osg::Vec3f dir = targetPos - actorPos;

        bool turned = smoothTurn(actor, getZAngleToDir(dir), 2, osg::DegreesToRadians(3.f));
        turned &= smoothTurn(actor, getXAngleToDir(dir), 0, osg::DegreesToRadians(3.f));

        if (!turned)
            return false;
    }

    // Check if the actor is already casting another spell
    bool isCasting = MWBase::Environment::get().getMechanicsManager()->isCastingSpell(actor);
    if (isCasting && !mCasting)
        return false;

    if (!mCasting)
    {
        MWBase::Environment::get().getMechanicsManager()->castSpell(actor, mSpellId, mManual);
        mCasting = true;
        return false;
    }

    // Finish package, if actor finished spellcasting
    return !isCasting;
}

MWWorld::Ptr MWMechanics::AiCast::getTarget() const
{
    MWWorld::Ptr target = MWBase::Environment::get().getWorld()->searchPtr(mTargetId, false);

    return target;
}

int MWMechanics::AiCast::getTypeId() const
{
    return AiPackage::TypeIdCast;
}

unsigned int MWMechanics::AiCast::getPriority() const
{
    return 3;
}
