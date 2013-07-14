#include "security.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/player.hpp"

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

#include "npcstats.hpp"
#include "creaturestats.hpp"

namespace MWMechanics
{

    Security::Security(const MWWorld::Ptr &actor)
        : mActor(actor)
    {
        CreatureStats& creatureStats = MWWorld::Class::get(actor).getCreatureStats(actor);
        NpcStats& npcStats = MWWorld::Class::get(actor).getNpcStats(actor);
        mAgility = creatureStats.getAttribute(ESM::Attribute::Agility).getModified();
        mLuck = creatureStats.getAttribute(ESM::Attribute::Luck).getModified();
        mSecuritySkill = npcStats.getSkill(ESM::Skill::Security).getModified();
        mFatigueTerm = creatureStats.getFatigueTerm();
    }

    void Security::pickLock(const MWWorld::Ptr &lock, const MWWorld::Ptr &lockpick,
                            std::string& resultMessage, std::string& resultSound)
    {
        if (lock.getCellRef().mLockLevel <= 0)
            return;

        int lockStrength = lock.getCellRef().mLockLevel;

        float pickQuality = lockpick.get<ESM::Lockpick>()->mBase->mData.mQuality;

        float fPickLockMult = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fPickLockMult")->getFloat();

        float x = 0.2 * mAgility + 0.1 * mLuck + mSecuritySkill;
        x *= pickQuality * mFatigueTerm;
        x += fPickLockMult * lockStrength;

        resultSound = "Open Lock Fail";
        if (x <= 0)
            resultMessage = "#{sLockImpossible}";
        else
        {
            int roll = static_cast<float> (std::rand()) / RAND_MAX * 100;
            if (roll <= x)
            {
                MWWorld::Class::get(lock).unlock(lock);
                resultMessage = "#{sLockSuccess}";
                resultSound = "Open Lock";
                MWWorld::Class::get(mActor).skillUsageSucceeded(mActor, ESM::Skill::Security, 1);
            }
            else
                resultMessage = "#{sLockFail}";
        }

        if (lockpick.getCellRef().mCharge == -1)
            lockpick.getCellRef().mCharge = lockpick.get<ESM::Lockpick>()->mBase->mData.mUses;
        --lockpick.getCellRef().mCharge;
        if (!lockpick.getCellRef().mCharge)
            lockpick.getRefData().setCount(0);
    }

    void Security::probeTrap(const MWWorld::Ptr &trap, const MWWorld::Ptr &probe,
                             std::string& resultMessage, std::string& resultSound)
    {
        if (trap.getCellRef().mTrap  == "")
            return;

        float probeQuality = probe.get<ESM::Probe>()->mBase->mData.mQuality;

        const ESM::Spell* trapSpell = MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().find(trap.getCellRef().mTrap);
        float trapSpellPoints = trapSpell->mData.mCost;

        float fTrapCostMult = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fTrapCostMult")->getFloat();

        float x = 0.2 * mAgility + 0.1 * mLuck + mSecuritySkill;
        x += fTrapCostMult * trapSpellPoints;
        x *= probeQuality * mFatigueTerm;

        resultSound = "Disarm Trap Fail";
        if (x <= 0)
            resultMessage = "#{sTrapImpossible}";
        else
        {
            int roll = static_cast<float> (std::rand()) / RAND_MAX * 100;
            if (roll <= x)
            {
                trap.getCellRef().mTrap = "";

                resultSound = "Disarm Trap";
                resultMessage = "#{sTrapSuccess}";
                MWWorld::Class::get(mActor).skillUsageSucceeded(mActor, ESM::Skill::Security, 0);
            }
            else
                resultMessage = "#{sTrapFail}";
        }

        if (probe.getCellRef().mCharge == -1)
            probe.getCellRef().mCharge = probe.get<ESM::Probe>()->mBase->mData.mUses;
        --probe.getCellRef().mCharge;
        if (!probe.getCellRef().mCharge)
            probe.getRefData().setCount(0);
    }

}
