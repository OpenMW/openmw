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

        // top level masks
        Mask_Scene = (1<<5),
        Mask_GUI = (1<<6)
    };

}

#endif
