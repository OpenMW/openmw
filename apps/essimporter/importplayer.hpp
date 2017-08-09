#ifndef OPENMW_ESSIMPORT_PLAYER_H
#define OPENMW_ESSIMPORT_PLAYER_H

#include <vector>
#include <string>

#include <components/esm/defs.hpp>
#include <components/esm/cellref.hpp>
#include <components/esm/esmcommon.hpp>

#include "importacdt.hpp"

namespace ESM
{
    class ESMReader;
}

namespace ESSImport
{

/// Player-agnostic player data
struct REFR
{
    ActorData mActorData;

    std::string mRefID;
    ESM::Position mPos;
    ESM::RefNum mRefNum;

    void load(ESM::ESMReader& esm);
};

/// Other player data
struct PCDT
{
    int mBounty;
    std::string mBirthsign;

    std::vector<std::string> mKnownDialogueTopics;

    enum PlayerFlags
    {
        PlayerFlags_ViewSwitchDisabled = 0x1,
        PlayerFlags_ControlsDisabled = 0x4,
        PlayerFlags_Sleeping = 0x10,
        PlayerFlags_Waiting = 0x40,
        PlayerFlags_WeaponDrawn = 0x80,
        PlayerFlags_SpellDrawn = 0x100,
        PlayerFlags_InJail = 0x200,
        PlayerFlags_JumpingDisabled = 0x1000,
        PlayerFlags_LookingDisabled = 0x2000,
        PlayerFlags_VanityModeDisabled = 0x4000,
        PlayerFlags_WeaponDrawingDisabled = 0x8000,
        PlayerFlags_SpellDrawingDisabled = 0x10000,
        PlayerFlags_ThirdPerson = 0x20000,
        PlayerFlags_TeleportingDisabled = 0x40000,
        PlayerFlags_LevitationDisabled = 0x80000
    };

#pragma pack(push)
#pragma pack(1)
    struct FNAM
    {
        unsigned char mRank;
        unsigned char mUnknown1[3];
        int mReputation;
        unsigned char mFlags; // 0x1: unknown, 0x2: expelled
        unsigned char mUnknown2[3];
        ESM::NAME32 mFactionName;
    };

    struct PNAM
    {
        struct MarkLocation
        {
            float mX, mY, mZ; // worldspace position
            float mRotZ; // Z angle in radians
            int mCellX, mCellY; // grid coordinates; for interior cells this is always (0, 0)
        };

        int mPlayerFlags; // controls, camera and draw state
        unsigned int mLevelProgress;
        float mSkillProgress[27]; // skill progress, non-uniform scaled
        unsigned char mSkillIncreases[8]; // number of skill increases for each attribute
        int mTelekinesisRangeBonus; // in units; seems redundant
        float mVisionBonus; // range: <0.0, 1.0>; affected by light spells and Get/Mod/SetPCVisionBonus
        int mDetectKeyMagnitude; // seems redundant
        int mDetectEnchantmentMagnitude; // seems redundant
        int mDetectAnimalMagnitude; // seems redundant
        MarkLocation mMarkLocation;
        unsigned char mUnknown3[40];
        unsigned char mSpecIncreases[3]; // number of skill increases for each specialization
        unsigned char mUnknown4;
    };

    struct ENAM
    {
        int mCellX;
        int mCellY;
    };

    struct AADT // 44 bytes
    {
        int animGroupIndex; // See convertANIS() for the mapping.
        unsigned char mUnknown5[40];
    };
#pragma pack(pop)

    std::vector<FNAM> mFactions;
    PNAM mPNAM;

    bool mHasMark;
    std::string mMNAM; // mark cell name; can also be sDefaultCellname or region name

    bool mHasENAM;
    ENAM mENAM; // last exterior cell

    bool mHasAADT;
    AADT mAADT;

    void load(ESM::ESMReader& esm);
};

}

#endif
