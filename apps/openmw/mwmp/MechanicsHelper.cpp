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
#include "CellController.hpp"

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

Attack *MechanicsHelper::getLocalAttack(const MWWorld::Ptr& ptr)
{
    if (ptr == MWBase::Environment::get().getWorld()->getPlayerPtr())
        return &mwmp::Main::get().getLocalPlayer()->attack;
    else if (mwmp::Main::get().getCellController()->isLocalActor(ptr))
        return &mwmp::Main::get().getCellController()->getLocalActor(ptr)->attack;

    return NULL;
}

Attack *MechanicsHelper::getDedicatedAttack(const MWWorld::Ptr& ptr)
{
    if (mwmp::PlayerList::isDedicatedPlayer(ptr))
        return &mwmp::PlayerList::getPlayer(ptr)->attack;
    else if (mwmp::Main::get().getCellController()->isDedicatedActor(ptr))
        return &mwmp::Main::get().getCellController()->getDedicatedActor(ptr)->attack;

    return NULL;
}

void MechanicsHelper::assignAttackTarget(Attack* attack, const MWWorld::Ptr& target)
{
    if (mwmp::PlayerList::isDedicatedPlayer(target))
    {
        attack->target.guid = mwmp::PlayerList::getPlayer(target)->guid;
        attack->target.refId.clear();
    }
    else
    {
        MWWorld::CellRef *targetRef = &target.getCellRef();

        attack->target.refId = targetRef->getRefId();
        attack->target.refNumIndex = targetRef->getRefNum().mIndex;
        attack->target.mpNum = targetRef->getMpNum();
    }
}

void MechanicsHelper::processAttack(Attack attack, const MWWorld::Ptr& attacker)
{
    if (attack.pressed == false)
    {
        LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "Attack success: %s", attack.success ? "true" : "false");

        if (attack.success == true)
            LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "Damage: %f", attack.damage);
    }

    MWMechanics::CreatureStats &attackerStats = attacker.getClass().getNpcStats(attacker);
    attackerStats.getSpells().setSelectedSpell(attack.spellId);

    MWWorld::Ptr victim;
    
    if (attack.target.guid == mwmp::Main::get().getLocalPlayer()->guid)
        victim = MWBase::Environment::get().getWorld()->getPlayerPtr();
    else if (PlayerList::getPlayer(attack.target.guid) != 0)
        victim = PlayerList::getPlayer(attack.target.guid)->getPtr();

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
