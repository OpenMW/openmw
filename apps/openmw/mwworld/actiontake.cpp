#include "actiontake.hpp"

#include <components/openmw-mp/Base/WorldEvent.hpp>
#include "../mwmp/Main.hpp"
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
        actor.getClass().getContainerStore (actor).add (getTarget(), getTarget().getRefData().getCount(), actor);

        // Added by tes3mp
        mwmp::WorldEvent *event = mwmp::Main::get().getNetworking()->createWorldEvent();
        event->cell = *getTarget().getCell()->getCell();
        event->cellRef.mRefID = getTarget().getCellRef().getRefId();
        event->cellRef.mRefNum = getTarget().getCellRef().getRefNum();
        mwmp::Main::get().getNetworking()->getWorldPacket(ID_OBJECT_DELETE)->Send(event);

        printf("Sending ID_OBJECT_DELETE about\n- cellRef: %s, %i\n- cell: %s\n",
            event->cellRef.mRefID.c_str(),
            event->cellRef.mRefNum.mIndex,
            event->cell.getDescription().c_str());

        MWBase::Environment::get().getWorld()->deleteObject (getTarget());
    }
}
