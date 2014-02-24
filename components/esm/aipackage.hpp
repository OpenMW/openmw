#ifndef OPENMW_ESM_AIPACKAGE_H
#define OPENMW_ESM_AIPACKAGE_H

#include <vector>
#include <string>

#include "esmcommon.hpp"

namespace ESM
{
    class ESMReader;
    class ESMWriter;

    #pragma pack(push)
    #pragma pack(1)

    struct AIData
    {
        // These are probabilities
        char mHello, mU1, mFight, mFlee, mAlarm, mU2, mU3, mU4;
        int mServices; // See the Services enum

        void blank();
        ///< Set record to default state (does not touch the ID).
    }; // 12 bytes

    struct AIWander
    {
        short   mDistance;
        short   mDuration;
        unsigned char mTimeOfDay;
        unsigned char mIdle[8];
        unsigned char mShouldRepeat;
    };

    struct AITravel
    {
        float   mX, mY, mZ;
        int     mUnk;
    };

    struct AITarget
    {
        float   mX, mY, mZ;
        short   mDuration;
        NAME32  mId;
        short   mUnk;
    };

    struct AIActivate
    {
        NAME32 mName;
        unsigned char mUnk;
    };

    #pragma pack(pop)

    enum
    {
        AI_Wander = 0x575f4941,
        AI_Travel = 0x545f4941,
        AI_Follow = 0x465f4941,
        AI_Escort = 0x455f4941,
        AI_Activate = 0x415f4941
    };

    /// \note Used for storaging packages in a single container
    /// w/o manual memory allocation accordingly to policy standards
    struct AIPackage
    {
        int mType;

        // Anonymous union
        union
        {
            AIWander mWander;
            AITravel mTravel;
            AITarget mTarget;
            AIActivate mActivate;
        };

        /// \note for AITarget only, placed here to stick with union,
        /// overhead should be not so awful
        std::string mCellName;
    };

    struct AIPackageList
    {
        std::vector<AIPackage> mList;

        /// \note This breaks consistency of subrecords reading:
        /// after calling it subrecord name is already read, so
        /// it needs to use retSubName() if needed. But, hey, there
        /// is only one field left (XSCL) and only two records uses AI
        void load(ESMReader &esm);
        void save(ESMWriter &esm) const;
    };
}

#endif

