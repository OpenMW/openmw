#ifndef GAME_RENDER_CONST_H
#define GAME_RENDER_CONST_H

#include <OgreRenderQueue.h>

namespace MWRender
{

// Render queue groups
enum RenderQueueGroups
{
    // Sky early (atmosphere, clouds, moons)
    RQG_SkiesEarly = Ogre::RENDER_QUEUE_SKIES_EARLY,

    RQG_Main = Ogre::RENDER_QUEUE_MAIN,

    RQG_Alpha = Ogre::RENDER_QUEUE_MAIN+1,

    RQG_OcclusionQuery = Ogre::RENDER_QUEUE_6,

    RQG_UnderWater = Ogre::RENDER_QUEUE_4,

    RQG_Water = RQG_Alpha,

    // Sky late (sun & sun flare)
    RQG_SkiesLate = Ogre::RENDER_QUEUE_SKIES_LATE
};

// Visibility flags
enum VisibilityFlags
{
    // Terrain
    RV_Terrain = 1,

    // Statics (e.g. trees, houses)
    RV_Statics = 2,

    // Small statics
    RV_StaticsSmall = 4,

    // Water
    RV_Water = 8,

    // Actors (npcs, creatures)
    RV_Actors = 16,

    // Misc objects (containers, dynamic objects)
    RV_Misc = 32,

    RV_Sky = 64,

    // not visible in reflection
    RV_NoReflection = 128,

    RV_OcclusionQuery = 256,

    RV_Debug = 512,

    // overlays, we only want these on the main render target
    RV_Overlay = 1024,

    RV_Map = RV_Terrain + RV_Statics + RV_StaticsSmall + RV_Misc + RV_Water
};

}

#endif
