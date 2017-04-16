//
// Created by koncord on 16.04.17.
//

#ifndef OPENMW_PROCESSORPLAYERATTACK_HPP
#define OPENMW_PROCESSORPLAYERATTACK_HPP


#include "apps/openmw/mwmp/PlayerProcessor.hpp"
#include "apps/openmw/mwbase/world.hpp"
#include "apps/openmw/mwworld/containerstore.hpp"
#include "apps/openmw/mwworld/inventorystore.hpp"
#include "apps/openmw/mwmechanics/combat.hpp"

#include "apps/openmw/mwbase/environment.hpp"

namespace mwmp
{
    class ProcessorPlayerAttack : public PlayerProcessor
    {
    public:
        ProcessorPlayerAttack()
        {
            BPP_INIT(ID_PLAYER_ATTACK)
        }

        virtual void Do(PlayerPacket &packet, BasePlayer *player)
        {
            DedicatedPlayer &dedicatedPlayer = static_cast<DedicatedPlayer&>(*player);
            //cout << "Player: " << dedicatedPlayer.Npc()->mName << " pressed: " << (dedicatedPlayer.getAttack()->pressed == 1) << endl;
            if (dedicatedPlayer.attack.pressed == 0)
            {
                LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "Attack success: %s", dedicatedPlayer.attack.success ? "true" : "false");

                if (dedicatedPlayer.attack.success == 1)
                    LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "Damage: %f",dedicatedPlayer.attack.damage);
            }

            MWMechanics::CreatureStats &stats = dedicatedPlayer.getPtr().getClass().getNpcStats(dedicatedPlayer.getPtr());
            stats.getSpells().setSelectedSpell(dedicatedPlayer.attack.refid);

            MWWorld::Ptr victim;
            if (dedicatedPlayer.attack.target == getLocalPlayer()->guid)
                victim = MWBase::Environment::get().getWorld()->getPlayerPtr();
            else if (Players::getPlayer(dedicatedPlayer.attack.target) != 0)
                victim = Players::getPlayer(dedicatedPlayer.attack.target)->getPtr();

            MWWorld::Ptr attacker;
            attacker = dedicatedPlayer.getPtr();

            // Get the weapon used (if hand-to-hand, weapon = inv.end())
            if (dedicatedPlayer.drawState == 1)
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
                        MWMechanics::blockMeleeAttack(attacker, victim, weapon, dedicatedPlayer.attack.damage, 1);
                    dedicatedPlayer.getPtr().getClass().onHit(victim, dedicatedPlayer.attack.damage, healthdmg, weapon, attacker, osg::Vec3f(),
                                                  dedicatedPlayer.attack.success);
                }
            }
            else
            {
                LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "SpellId: %s", dedicatedPlayer.attack.refid.c_str());
                LOG_APPEND(Log::LOG_VERBOSE, " - success: %d", dedicatedPlayer.attack.success);
            }
        }
    };
}


#endif //OPENMW_PROCESSORPLAYERATTACK_HPP
