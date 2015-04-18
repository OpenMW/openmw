#ifndef OPENMW_MWRENDER_VISMASK_H
#define OPENMW_MWRENDER_VISMASK_H

namespace MWRender
{

    /// Node masks used for controlling visibility of game objects.
    enum VisMask
    {
        Mask_UpdateVisitor = 0x1, // reserved for separating UpdateVisitors from CullVisitors

        Mask_Effect = 0x2
    };

}

#endif
