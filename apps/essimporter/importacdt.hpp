#ifndef OPENMW_ESSIMPORT_ACDT_H
#define OPENMW_ESSIMPORT_ACDT_H

#include <string>

namespace ESM
{
    struct ESMReader;
}

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

    struct ActorData
    {
        ACDT mACDT;

        int mSkills[27][2];

        // creature combat stats, base and modified
        // I think these can be ignored in the conversion, because it is not possible
        // to change them ingame
        int mCombatStats[3][2];

        std::string mScript;

        void load(ESM::ESMReader& esm);
    };

    /// Unknown, shared by (at least) REFR and CellRef
    struct ACSC
    {
        unsigned char unknown[112];
    };

}

#endif
