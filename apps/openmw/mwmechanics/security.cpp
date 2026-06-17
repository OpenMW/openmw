#include "security.hpp"

#include <components/misc/rng.hpp>

#include "../mwworld/class.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/esmstore.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/world.hpp"

#include "creaturestats.hpp"
#include "spellutil.hpp"

namespace MWMechanics
{

    Security::Security(const MWWorld::Ptr& actor)
        : mActor(actor)
    {
        CreatureStats& creatureStats = actor.getClass().getCreatureStats(actor);
        mAgility = creatureStats.getAttribute(ESM::Attribute::Agility).getModified();
        mLuck = creatureStats.getAttribute(ESM::Attribute::Luck).getModified();
        mSecuritySkill = static_cast<float>(actor.getClass().getSkill(actor, ESM::Skill::Security));
        mFatigueTerm = creatureStats.getFatigueTerm();
    }

    void Security::pickLock(const MWWorld::Ptr& lock, const MWWorld::Ptr& lockpick, std::string_view& resultMessage,
        std::string_view& resultSound)
    {
        // If it's unlocked or can not be unlocked back out immediately. Note that we're not strictly speaking checking
        // if the ref is locked, lock levels <= 0 can exist but they cannot be picked
        if (lock.getCellRef().getLockLevel() <= 0 || !lock.getClass().hasToolTip(lock))
            return;

        int uses = lockpick.getClass().getItemHealth(lockpick);
        if (uses == 0)
            return;

        int lockStrength = lock.getCellRef().getLockLevel();

        float pickQuality = lockpick.get<ESM::Lockpick>()->mBase->mData.mQuality;

        float fPickLockMult = MWBase::Environment::get()
                                  .getESMStore()
                                  ->get<ESM::GameSetting>()
                                  .find("fPickLockMult")
                                  ->mValue.getFloat();

        float x = 0.2f * mAgility + 0.1f * mLuck + mSecuritySkill;
        x *= pickQuality * mFatigueTerm;
        x += fPickLockMult * lockStrength;

        MWBase::Environment::get().getMechanicsManager()->unlockAttempted(mActor, lock);

        resultSound = "Open Lock Fail";
        if (x <= 0)
            resultMessage = "#{sLockImpossible}";
        else
        {
            auto& prng = MWBase::Environment::get().getWorld()->getPrng();
            if (Misc::Rng::roll0to99(prng) <= x)
            {
                lock.getCellRef().unlock();
                resultMessage = "#{sLockSuccess}";
                resultSound = "Open Lock";
                mActor.getClass().skillUsageSucceeded(mActor, ESM::Skill::Security, ESM::Skill::Security_PickLock);
            }
            else
                resultMessage = "#{sLockFail}";
        }

        lockpick.getCellRef().setCharge(--uses);
        if (!uses)
            lockpick.getContainerStore()->remove(lockpick, 1);
    }

    void Security::probeTrap(const MWWorld::Ptr& trap, const MWWorld::Ptr& probe, std::string_view& resultMessage,
        std::string_view& resultSound)
    {
        if (trap.getCellRef().getTrap().empty())
            return;

        int uses = probe.getClass().getItemHealth(probe);
        if (uses == 0)
            return;

        float probeQuality = probe.get<ESM::Probe>()->mBase->mData.mQuality;

        const ESM::Spell* trapSpell
            = MWBase::Environment::get().getESMStore()->get<ESM::Spell>().find(trap.getCellRef().getTrap());
        int trapSpellPoints = MWMechanics::calcSpellCost(*trapSpell);

        float fTrapCostMult = MWBase::Environment::get()
                                  .getESMStore()
                                  ->get<ESM::GameSetting>()
                                  .find("fTrapCostMult")
                                  ->mValue.getFloat();

        float x = 0.2f * mAgility + 0.1f * mLuck + mSecuritySkill;
        x += fTrapCostMult * trapSpellPoints;
        x *= probeQuality * mFatigueTerm;

        MWBase::Environment::get().getMechanicsManager()->unlockAttempted(mActor, trap);

        resultSound = "Disarm Trap Fail";
        if (x <= 0)
            resultMessage = "#{sTrapImpossible}";
        else
        {
            auto& prng = MWBase::Environment::get().getWorld()->getPrng();
            if (Misc::Rng::roll0to99(prng) <= x)
            {
                trap.getCellRef().setTrap(ESM::RefId());

                resultSound = "Disarm Trap";
                resultMessage = "#{sTrapSuccess}";
                mActor.getClass().skillUsageSucceeded(mActor, ESM::Skill::Security, ESM::Skill::Security_DisarmTrap);
            }
            else
                resultMessage = "#{sTrapFail}";
        }

        probe.getCellRef().setCharge(--uses);
        if (!uses)
            probe.getContainerStore()->remove(probe, 1);
    }

}
