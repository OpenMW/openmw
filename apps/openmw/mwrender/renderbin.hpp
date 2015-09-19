#ifndef OPENMW_MWRENDER_RENDERBIN_H
#define OPENMW_MWRENDER_RENDERBIN_H

namespace MWRender
{

    /// Defines the render bin numbers used in the OpenMW scene graph. The bin with the lowest number is rendered first.
    /// Beware of RenderBin nesting, in most cases you will want to use setNestRenderBins(false).
    enum RenderBins
    {
        RenderBin_Sky = -1,
        RenderBin_Default = 0,
        RenderBin_Water = 9,
        RenderBin_OcclusionQuery = 10,
        RenderBin_SunGlare = 11
    };

}

#endif
