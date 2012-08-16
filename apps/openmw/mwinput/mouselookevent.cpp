#include "mouselookevent.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/player.hpp"

#include <OIS/OIS.h>
#include <OgreCamera.h>
#include <OgreSceneNode.h>

using namespace OIS;
using namespace MWInput;

void MouseLookEvent::event(Type type, int index, const void *p)
{
    if (type != EV_MouseMove || mDisabled) {
        return;
    }

    MouseEvent *arg = (MouseEvent*)(p);

    float x = arg->state.X.rel * sensX;
    float y = arg->state.Y.rel * sensY;

    MWBase::World *world = MWBase::Environment::get().getWorld();
    world->rotateObject(world->getPlayer().getPlayer(), -y, 0.f, x, true);
}
