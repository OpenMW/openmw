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
    RQG_Ripples = RQG_Water+1,

    // Sky late (sun & sun flare)
    RQG_SkiesLate = Ogre::RENDER_QUEUE_SKIES_LATE
};

// Visibility flags
enum VisibilityFlags
{
    // Terrain
    RV_Terrain = (1<<0),

    // Statics (e.g. trees, houses)
    RV_Statics = (1<<1),

    // Small statics
    RV_StaticsSmall = (1<<2),

    // Water
    RV_Water = (1<<3),

    // Actors (npcs, creatures)
    RV_Actors = (1<<4),

    // Misc objects (containers, dynamic objects)
    RV_Misc = (1<<5),

    // VFX, don't appear on map and don't cast shadows
    RV_Effects = (1<<6),

    RV_Sky = (1<<7),

    // not visible in reflection
    RV_NoReflection = (1<<8),

    RV_OcclusionQuery = (1<<9),

    RV_Debug = (1<<10),

    // overlays, we only want these on the main render target
    RV_Overlay = (1<<11),

    // First person meshes do not cast shadows
    RV_FirstPerson = (1<<12),

    RV_Map = RV_Terrain + RV_Statics + RV_StaticsSmall + RV_Misc + RV_Water,

    RV_Refraction = RV_Actors + RV_Misc + RV_Statics + RV_StaticsSmall + RV_Terrain + RV_Effects + RV_Sky + RV_FirstPerson
};

}

#endif
