#ifndef CSV_WIDGET_INSTANCEDRAGMODES_H
#define CSV_WIDGET_INSTANCEDRAGMODES_H

namespace CSVRender
{
    enum DragMode
    {
        DragMode_None,
        DragMode_Move,
        DragMode_Rotate,
        DragMode_Scale,
        DragMode_Select_Only,
        DragMode_Select_Add,
        DragMode_Select_Remove,
        DragMode_Select_Invert
    };
}
#endif
