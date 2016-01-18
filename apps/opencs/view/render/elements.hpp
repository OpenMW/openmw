#ifndef CSV_RENDER_ELEMENTS_H
#define CSV_RENDER_ELEMENTS_H

namespace CSVRender
{
    /// Visual elements in a scene
    enum Elements
    {
        // elements that are part of the actual scene
        Element_Reference = 0x1,
        Element_Pathgrid = 0x2,
        Element_Water = 0x4,
        Element_Fog = 0x8,
        Element_Terrain = 0x10,

        // control elements
        Element_CellMarker = 0x10000,
        Element_CellArrow = 0x20000,
        Element_CellBorder = 0x40000
    };
}

#endif
