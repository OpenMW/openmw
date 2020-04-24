#ifndef CSV_RENDER_ELEMENTS_H
#define CSV_RENDER_ELEMENTS_H

namespace CSVRender
{

    /// Node masks used on the OSG scene graph in OpenMW-CS.
    /// @note See the respective file in OpenMW (apps/openmw/mwrender/vismask.hpp)
    /// for general usage hints about node masks.
    /// @copydoc MWRender::VisMask
    enum Mask
    {
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
