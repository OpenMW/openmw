#include "actiontake.hpp"

#include <components/openmw-mp/Log.hpp>
#include "../mwmp/Main.hpp"
#include "../mwmp/Networking.hpp"
#include "../mwmp/WorldEvent.hpp"
#include "../mwmp/LocalPlayer.hpp"
#include "../mwworld/cellstore.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"

#include "class.hpp"
#include "containerstore.hpp"

namespace MWWorld
{
    ActionTake::ActionTake (const MWWorld::Ptr& object) : Action (true, object) {}

    void ActionTake::executeImp (const Ptr& actor)
    {
        MWBase::Environment::get().getMechanicsManager()->itemTaken(
                    actor, getTarget(), MWWorld::Ptr(), getTarget().getRefData().getCount());
        MWWorld::Ptr newitem = *actor.getClass().getContainerStore (actor).add (getTarget(), getTarget().getRefData().getCount(), actor);

        /*
            Start of tes3mp addition

            Send an ID_OBJECT_DELETE packet every time an item is taken from the world
            by the player outside of the inventory screen

            Send an ID_PLAYER_INVENTORY packet as well because of the item thus gained
            by the player
        */
        mwmp::WorldEvent *worldEvent = mwmp::Main::get().getNetworking()->resetWorldEvent();
        worldEvent->cell = *getTarget().getCell()->getCell();

        mwmp::WorldObject worldObject;
        worldObject.refId = getTarget().getCellRef().getRefId();
        worldObject.refNumIndex = getTarget().getCellRef().getRefNum().mIndex;
        worldObject.mpNum = getTarget().getCellRef().getMpNum();
        worldEvent->addObject(worldObject);

        mwmp::Main::get().getNetworking()->getWorldPacket(ID_OBJECT_DELETE)->setEvent(worldEvent);
        mwmp::Main::get().getNetworking()->getWorldPacket(ID_OBJECT_DELETE)->Send();

        LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "Sending ID_OBJECT_DELETE about\n- cellRef: %s, %i\n- cell: %s",
                           worldObject.refId.c_str(), worldObject.refNumIndex, worldEvent->cell.getDescription().c_str());

        mwmp::Main::get().getLocalPlayer()->sendInventory();
        /*
            End of tes3mp addition
        */

        MWBase::Environment::get().getWorld()->deleteObject (getTarget());
        setTarget(newitem);
    }
}
