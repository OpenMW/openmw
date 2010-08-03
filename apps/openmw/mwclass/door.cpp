
#include "door.hpp"

#include <components/esm/loaddoor.hpp>

#include <components/esm_store/cell_store.hpp>

#include "../mwrender/playerpos.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/nullaction.hpp"
#include "../mwworld/actionteleport.hpp"
#include "../mwworld/environment.hpp"
#include "../mwworld/world.hpp"

namespace MWClass
{
    std::string Door::getName (const MWWorld::Ptr& ptr) const
    {
        ESMS::LiveCellRef<ESM::Door, MWWorld::RefData> *ref =
            ptr.get<ESM::Door>();

        return ref->base->name;
    }

    boost::shared_ptr<MWWorld::Action> Door::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor, const MWWorld::Environment& environment) const
    {
        ESMS::LiveCellRef<ESM::Door, MWWorld::RefData> *ref =
            ptr.get<ESM::Door>();

        if (ref->ref.teleport)
        {
            // teleport door
            if (environment.mWorld->getPlayerPos().getPlayer()==actor)
            {
                // the player is using the door
                return boost::shared_ptr<MWWorld::Action> (
                    new MWWorld::ActionTeleportPlayer (ref->ref.destCell, ref->ref.doorDest));
            }
            else
            {
                // another NPC or a create is using the door
                // TODO return action for teleporting other NPC/creature
                return boost::shared_ptr<MWWorld::Action> (new MWWorld::NullAction);
            }
        }
        else
        {
            // animated door
            // TODO return action for rotating the door
            return boost::shared_ptr<MWWorld::Action> (new MWWorld::NullAction);
        }
    }

    void Door::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Door);

        registerClass (typeid (ESM::Door).name(), instance);
    }
}
