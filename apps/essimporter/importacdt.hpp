#ifndef OPENMW_ESSIMPORT_ACDT_H
#define OPENMW_ESSIMPORT_ACDT_H

namespace ESSImport
{

    /// Actor data, shared by (at least) REFR and CellRef
    struct ACDT
    {
        unsigned char mUnknown1[40];
        float mDynamic[3][2];
        unsigned char mUnknown2[16];
        float mAttributes[8][2];
        unsigned char mUnknown3[120];
    };

    /// Unknown, shared by (at least) REFR and CellRef
    struct ACSC
    {
        unsigned char unknown[112];
    };

}

#endif
