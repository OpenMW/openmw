#ifndef OPENMW_MWRENDER_VISMASK_H
#define OPENMW_MWRENDER_VISMASK_H

namespace MWRender
{

    /// Node masks used for controlling visibility of game objects.
    enum VisMask
    {
        Mask_UpdateVisitor = 0x1, // reserved for separating UpdateVisitors from CullVisitors

        // child of Scene
        Mask_Effect = 0x2,
        Mask_Debug = 0x4,
        Mask_Actor = 0x8,

        // top level masks
        Mask_Scene = 0x10,
        Mask_GUI = 0x20
    };

}

#endif
