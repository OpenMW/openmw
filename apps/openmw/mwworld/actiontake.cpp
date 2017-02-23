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

        // Added by tes3mp
        mwmp::WorldEvent *worldEvent = mwmp::Main::get().getNetworking()->resetWorldEvent();
        worldEvent->cell = *getTarget().getCell()->getCell();

        mwmp::WorldObject worldObject;
        worldObject.refId = getTarget().getCellRef().getRefId();
        worldObject.refNumIndex = getTarget().getCellRef().getRefNum().mIndex;
        worldEvent->addObject(worldObject);

        mwmp::Main::get().getNetworking()->getWorldPacket(ID_OBJECT_DELETE)->Send(worldEvent);

        LOG_MESSAGE_SIMPLE(Log::LOG_VERBOSE, "Sending ID_OBJECT_DELETE about\n- cellRef: %s, %i\n- cell: %s",
            worldObject.refId.c_str(),
            worldObject.refNumIndex,
            worldEvent->cell.getDescription().c_str());

        // LocalPlayer's inventory has changed, so send a packet with it
        mwmp::Main::get().getLocalPlayer()->sendInventory();

        MWBase::Environment::get().getWorld()->deleteObject (getTarget());
        setTarget(newitem);
    }
}
