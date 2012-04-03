#ifndef GAME_RENDER_CONST_H
#define GAME_RENDER_CONST_H

#include <OgreRenderQueue.h>

namespace MWRender
{

// Render queue groups
/// \todo

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

    // Actors (player, npcs, creatures)
    RV_Actors = 16,

    // Misc objects (containers, dynamic objects)
    RV_Misc = 32,

    RV_Sky = 64,

    RV_Map = RV_Terrain + RV_Statics + RV_StaticsSmall + RV_Misc + RV_Water,

    /// \todo markers (normally hidden)

    RV_All = 255
};

}

#endif
