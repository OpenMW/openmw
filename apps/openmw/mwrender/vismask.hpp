#ifndef OPENMW_MWRENDER_VISMASK_H
#define OPENMW_MWRENDER_VISMASK_H

namespace MWRender
{

    /// Node masks used for controlling visibility of game objects.
    enum VisMask
    {
        Mask_UpdateVisitor = 0x1, // reserved for separating UpdateVisitors from CullVisitors

        // child of Scene
        Mask_Effect = (1<<1),
        Mask_Debug = (1<<2),
        Mask_Actor = (1<<3),
        Mask_Player = (1<<4),
        Mask_Sky = (1<<5),
        Mask_Water = (1<<6),
        Mask_SimpleWater = (1<<7),
        Mask_Terrain = (1<<8),
        Mask_FirstPerson = (1<<9),

        // top level masks
        Mask_Scene = (1<<10),
        Mask_GUI = (1<<11),

        // Set on a Geode
        Mask_ParticleSystem = (1<<12),

        // Set on cameras within the main scene graph
        Mask_RenderToTexture = (1<<13)

        // reserved: (1<<16) for SceneUtil::Mask_Lit
    };

}

#endif
