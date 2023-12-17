#ifndef OPENMW_ESSIMPORT_ACDT_H
#define OPENMW_ESSIMPORT_ACDT_H

#include <cstdint>
#include <string>

#include "importscri.hpp"

namespace ESM
{
    class ESMReader;
}

namespace ESSImport
{

    enum ACDTFlags
    {
        TalkedToPlayer = 0x4,
        Attacked = 0x100,
        Unknown = 0x200
    };
    enum ACSCFlags
    {
        Dead = 0x2
    };

    /// Actor data, shared by (at least) REFR and CellRef
    struct ACDT
    {
        // Note, not stored at *all*:
        // - Level changes are lost on reload, except for the player (there it's in the NPC record).
        unsigned char mUnknown[12];
        uint32_t mFlags;
        float mBreathMeter; // Seconds left before drowning
        unsigned char mUnknown2[20];
        float mDynamic[3][2];
        unsigned char mUnknown3[16];
        float mAttributes[8][2];
        float mMagicEffects[27]; // Effect attributes:
                                 // https://wiki.openmw.org/index.php?title=Research:Magic#Effect_attributes
        unsigned char mUnknown4[4];
        uint32_t mGoldPool;
        unsigned char mCountDown; // seen the same value as in ACSC.mCorpseClearCountdown, maybe
                                  // this one is for respawning?
        unsigned char mUnknown5[3];
    };
    struct ACSC
    {
        unsigned char mUnknown1[17];
        unsigned char mFlags; // ACSCFlags
        unsigned char mUnknown2[22];
        unsigned char mCorpseClearCountdown; // hours?
        unsigned char mUnknown3[71];
    };
    struct ANIS
    {
        unsigned char mGroupIndex;
        unsigned char mUnknown[3];
        float mTime;
    };

    struct ActorData
    {
        bool mHasACDT;
        ACDT mACDT;

        bool mHasACSC;
        ACSC mACSC;

        int mSkills[27][2]; // skills, base and modified

        // creature combat stats, base and modified
        // I think these can be ignored in the conversion, because it is not possible
        // to change them ingame
        int mCombatStats[3][2];

        std::string mSelectedSpell;
        std::string mSelectedEnchantItem;

        SCRI mSCRI;

        bool mHasANIS;
        ANIS mANIS; // scripted animation state
    };

}

#endif
