#ifndef GAME_RENDER_BLENDMASK_H
#define GAME_RENDER_BLENDMASK_H

#include <cstddef>

namespace MWRender
{
    enum BlendMask
    {
        BlendMask_LowerBody = 1 << 0,
        BlendMask_Torso = 1 << 1,
        BlendMask_LeftArm = 1 << 2,
        BlendMask_RightArm = 1 << 3,

        BlendMask_UpperBody = BlendMask_Torso | BlendMask_LeftArm | BlendMask_RightArm,

        BlendMask_All = BlendMask_LowerBody | BlendMask_UpperBody
    };
    /* This is the number of *discrete* blend masks. */
    static constexpr size_t sNumBlendMasks = 4;
}
#endif
