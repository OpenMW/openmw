#include <components/openmw-mp/Log.hpp>

#include "../mwbase/environment.hpp"
#include "../mwmechanics/movement.hpp"
#include "../mwmechanics/npcstats.hpp"
#include "../mwrender/animation.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/worldimp.hpp"

#include "LocalActor.hpp"
#include "Main.hpp"
#include "Networking.hpp"
#include "ActorList.hpp"

using namespace mwmp;
using namespace std;

LocalActor::LocalActor()
{
    posWasChanged = false;
}

LocalActor::~LocalActor()
{

}

void LocalActor::update(bool forceUpdate)
{
    updatePosition(forceUpdate);
    updateDrawState();
}

void LocalActor::updatePosition(bool forceUpdate)
{
    bool posIsChanging = (direction.pos[0] != 0 || direction.pos[1] != 0 || direction.pos[2] != 0 ||
        direction.rot[0] != 0 || direction.rot[1] != 0 || direction.rot[2] != 0);

    if (posIsChanging || posWasChanged)
    {
        posWasChanged = posIsChanging;

        position = ptr.getRefData().getPosition();

        ActorList *actorList = mwmp::Main::get().getNetworking()->getActorList();
        actorList->addPositionActor(*this);
    }
}

void LocalActor::updateDrawState()
{
    drawState = ptr.getClass().getNpcStats(ptr).getDrawState();
}

MWWorld::Ptr LocalActor::getPtr()
{
    return ptr;
}

void LocalActor::setPtr(const MWWorld::Ptr& newPtr)
{
    ptr = newPtr;

    refId = newPtr.getCellRef().getRefId();
    refNumIndex = newPtr.getCellRef().getRefNum().mIndex;
    mpNum = newPtr.getCellRef().getMpNum();
}
