#include <components/openmw-mp/Log.hpp>

#include "../mwbase/environment.hpp"
#include "../mwmechanics/movement.hpp"
#include "../mwmechanics/npcstats.hpp"
#include "../mwrender/animation.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/worldimp.hpp"

#include "LocalActor.hpp"

using namespace mwmp;
using namespace std;

LocalActor::LocalActor()
{
    headPitch = -1;
    headYaw = -1;
    hasAnimation = false;
    hasAnimStates = false;
    hasMovement = false;
}

LocalActor::~LocalActor()
{

}

void LocalActor::update()
{
    updatePosition();
    updateDrawState();
}

void LocalActor::updatePosition()
{
    position = ptr.getRefData().getPosition();
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
