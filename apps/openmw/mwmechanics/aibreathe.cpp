#include "aibreathe.hpp"

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"

#include "npcstats.hpp"

#include "movement.hpp"
#include "steering.hpp"

MWMechanics::AiBreathe::AiBreathe()
: AiPackage()
{

}

bool MWMechanics::AiBreathe::execute (const MWWorld::Ptr& actor, CharacterController& characterController, AiState& state, float duration)
{
    static const float fHoldBreathTime = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fHoldBreathTime")->mValue.getFloat();

    const MWWorld::Class& actorClass = actor.getClass();
    if (actorClass.isNpc())
    {
        if (actorClass.getNpcStats(actor).getTimeToStartDrowning() < fHoldBreathTime / 2)
        {
            actorClass.getCreatureStats(actor).setMovementFlag(CreatureStats::Flag_Run, true);

            actorClass.getMovementSettings(actor).mPosition[1] = 1;
            smoothTurn(actor, -180, 0);

            return false;
        }
    }

    return true;
}

MWMechanics::AiBreathe *MWMechanics::AiBreathe::clone() const
{
    return new AiBreathe(*this);
}

int MWMechanics::AiBreathe::getTypeId() const
{
    return TypeIdBreathe;
}

unsigned int MWMechanics::AiBreathe::getPriority() const
{
    return 2;
}
