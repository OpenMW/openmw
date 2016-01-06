#ifndef CSV_RENDER_ELEMENTS_H
#define CSV_RENDER_ELEMENTS_H

namespace CSVRender
{

    /// @note Enumeration values can be changed freely, as long as they do not collide.
    enum Mask
    {
        // internal use within NifLoader, do not change
        Mask_UpdateVisitor = 0x1,

        // elements that are part of the actual scene
        Mask_Reference = 0x2,
        Mask_Pathgrid = 0x4,
        Mask_Water = 0x8,
        Mask_Fog = 0x10,
        Mask_Terrain = 0x20,

        // used within models
        Mask_ParticleSystem = 0x100,

        Mask_Lighting = 0x200,

        // control elements
        Mask_CellMarker = 0x10000,
        Mask_CellArrow = 0x20000,
        Mask_CellBorder = 0x40000
    };
}

#endif
