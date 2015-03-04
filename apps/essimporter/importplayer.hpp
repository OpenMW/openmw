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

    enum DrawState_
    {
        DrawState_Weapon = 0x80,
        DrawState_Spell = 0x100
    };
    enum CameraState
    {
        CameraState_FirstPerson = 0x8,
        CameraState_ThirdPerson = 0xa
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
        short mDrawState; // DrawState
        short mCameraState; // CameraState
        unsigned int mLevelProgress;
        float mSkillProgress[27]; // skill progress, non-uniform scaled
        unsigned char mSkillIncreases[8]; // number of skill increases for each attribute
        unsigned char mUnknown3[88];
    };
#pragma pack(pop)

    std::vector<FNAM> mFactions;
    PNAM mPNAM;

    void load(ESM::ESMReader& esm);
};

}

#endif
