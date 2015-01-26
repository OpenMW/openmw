#ifndef OPENMW_ESSIMPORT_ACDT_H
#define OPENMW_ESSIMPORT_ACDT_H

#include <string>

#include <components/esm/cellref.hpp>

#include "importscri.hpp"

namespace ESM
{
    class ESMReader;
}

namespace ESSImport
{

    enum ACDTFlags
    {
        TalkedToPlayer = 0x4
    };

    /// Actor data, shared by (at least) REFR and CellRef
#pragma pack(push)
#pragma pack(1)
    struct ACDT
    {
        // Note, not stored at *all*:
        // - Level changes are lost on reload, except for the player (there it's in the NPC record).
        unsigned char mUnknown[12];
        unsigned char mFlags; // ACDTFlags
        unsigned char mUnknown1[3];
        float mBreathMeter; // Seconds left before drowning
        unsigned char mUnknown2[20];
        float mDynamic[3][2];
        unsigned char mUnknown3[16];
        float mAttributes[8][2];
        unsigned char mUnknown4[112];
        unsigned int mGoldPool;
        unsigned char mUnknown5[4];
    };
#pragma pack(pop)

    struct ActorData : public ESM::CellRef
    {
        ACDT mACDT;

        int mSkills[27][2];

        // creature combat stats, base and modified
        // I think these can be ignored in the conversion, because it is not possible
        // to change them ingame
        int mCombatStats[3][2];

        std::string mSelectedSpell;
        std::string mSelectedEnchantItem;

        SCRI mSCRI;

        void load(ESM::ESMReader& esm);
    };

    /// Unknown, shared by (at least) REFR and CellRef
    struct ACSC
    {
        unsigned char unknown[112];
    };

}

#endif
