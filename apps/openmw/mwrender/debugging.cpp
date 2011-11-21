#include "debugging.hpp"

#include <assert.h>

#include "OgreRoot.h"
#include "OgreRenderWindow.h"
#include "OgreSceneManager.h"
#include "OgreViewport.h"
#include "OgreCamera.h"
#include "OgreTextureManager.h"

#include "../mwworld/world.hpp" // these includes can be removed once the static-hack is gone
#include "../mwworld/ptr.hpp"
#include <components/esm/loadstat.hpp>

#include "player.hpp"

using namespace MWRender;
using namespace Ogre;

Debugging::Debugging(OEngine::Physic::PhysicEngine* engine){
	eng = engine;
}


bool Debugging::toggleRenderMode (int mode){
	 switch (mode)
    {
        case MWWorld::World::Render_CollisionDebug:

            // TODO use a proper function instead of accessing the member variable
            // directly.
            eng->setDebugRenderingMode (!eng->isDebugCreated);
            return eng->isDebugCreated;
    }

    return false;
}
