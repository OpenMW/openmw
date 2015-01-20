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
#pragma pack(push)
#pragma pack(1)
    struct ACDT
    {
        // Note, not stored at *all*:
        // - Level changes are lost on reload, except for the player (there it's in the NPC record).
        unsigned char mUnknown1[16];
        float mBreathMeter; // Seconds left before drowning
        unsigned char mUnknown2[20];
        float mDynamic[3][2];
        unsigned char mUnknown3[16];
        float mAttributes[8][2];
        unsigned char mUnknown4[109];
        // This seems to increase when purchasing training, though I don't see it anywhere ingame.
        int mGold;
        unsigned char mUnknown5[7];
    };
#pragma pack(pop)

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
