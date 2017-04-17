#include <components/openmw-mp/Log.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwmechanics/combat.hpp"
#include "../mwmechanics/npcstats.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/inventorystore.hpp"

#include "MechanicsHelper.hpp"
#include "Main.hpp"
#include "LocalPlayer.hpp"
#include "DedicatedPlayer.hpp"

using namespace mwmp;

mwmp::MechanicsHelper::MechanicsHelper()
{

}

mwmp::MechanicsHelper::~MechanicsHelper()
{

}

osg::Vec3f MechanicsHelper::getLinearInterpolation(osg::Vec3f start, osg::Vec3f end, float percent)
{
    osg::Vec3f position(percent, percent, percent);

    return (start + osg::componentMultiply(position, (end - start)));
}

void MechanicsHelper::processAttack(const MWWorld::Ptr& attacker, Attack attack)
{
    if (attack.pressed == 0)
    {
        LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "Attack success: %s", attack.success ? "true" : "false");

        if (attack.success == 1)
            LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "Damage: %f", attack.damage);
    }

    MWMechanics::CreatureStats &attackerStats = attacker.getClass().getNpcStats(attacker);
    attackerStats.getSpells().setSelectedSpell(attack.spellId);

    MWWorld::Ptr victim;
    
    if (attack.targetGuid == mwmp::Main::get().getLocalPlayer()->guid)
        victim = MWBase::Environment::get().getWorld()->getPlayerPtr();
    else if (Players::getPlayer(attack.targetGuid) != 0)
        victim = Players::getPlayer(attack.targetGuid)->getPtr();

    // Get the weapon used (if hand-to-hand, weapon = inv.end())
    if (attackerStats.getDrawState() == MWMechanics::DrawState_Weapon)
    {
        MWWorld::InventoryStore &inv = attacker.getClass().getInventoryStore(attacker);
        MWWorld::ContainerStoreIterator weaponslot = inv.getSlot(
            MWWorld::InventoryStore::Slot_CarriedRight);

        MWWorld::Ptr weapon = ((weaponslot != inv.end()) ? *weaponslot : MWWorld::Ptr());
        if (!weapon.isEmpty() && weapon.getTypeName() != typeid(ESM::Weapon).name())
            weapon = MWWorld::Ptr();

        if (victim.mRef != 0)
        {
            bool healthdmg;
            if (!weapon.isEmpty())
                healthdmg = true;
            else
            {
                MWMechanics::CreatureStats &otherstats = victim.getClass().getCreatureStats(victim);
                healthdmg = otherstats.isParalyzed() || otherstats.getKnockedDown();
            }

            if (!weapon.isEmpty())
                MWMechanics::blockMeleeAttack(attacker, victim, weapon, attack.damage, 1);

            attacker.getClass().onHit(victim, attack.damage, healthdmg, weapon, attacker, osg::Vec3f(),
                attack.success);
        }
    }
    else
    {
        LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "SpellId: %s", attack.spellId.c_str());
        LOG_APPEND(Log::LOG_VERBOSE, " - success: %d", attack.success);
    }
}
