#ifndef OPENMW_COMPONENTS_ESM_AISEQUENCE_H
#define OPENMW_COMPONENTS_ESM_AISEQUENCE_H

#include <vector>
#include <string>

#include "defs.hpp"

#include "util.hpp"

namespace ESM
{
    class ESMReader;
    class ESMWriter;

    namespace AiSequence
    {

    // format 0, saved games only
    // As opposed to AiPackageList, this stores the "live" version of AI packages.

    enum AiPackages
    {
        Ai_Wander = ESM::FourCC<'W','A','N','D'>::value,
        Ai_Travel = ESM::FourCC<'T','R','A','V'>::value,
        Ai_Escort = ESM::FourCC<'E','S','C','O'>::value,
        Ai_Follow = ESM::FourCC<'F','O','L','L'>::value,
        Ai_Activate = ESM::FourCC<'A','C','T','I'>::value,
        Ai_Combat = ESM::FourCC<'C','O','M','B'>::value,
        Ai_Pursue = ESM::FourCC<'P','U','R','S'>::value
    };


    struct AiPackage
    {
        virtual ~AiPackage() {}
    };


#pragma pack(push,1)
    struct AiWanderData
    {
        short   mDistance;
        short   mDuration;
        unsigned char mTimeOfDay;
        unsigned char mIdle[8];
        unsigned char mShouldRepeat;
    };
    struct AiWanderDuration
    {
        float mRemainingDuration;
        int unused;
    };
    struct AiTravelData
    {
        float   mX, mY, mZ;
    };
    struct AiEscortData
    {
        float mX, mY, mZ;
        short   mDuration;
    };

#pragma pack(pop)

    struct AiWander : AiPackage
    {
        AiWanderData mData;
        AiWanderDuration mDurationData; // was ESM::TimeStamp mStartTime

        bool mStoredInitialActorPosition;
        ESM::Vector3 mInitialActorPosition;

        /// \todo add more AiWander state

        void load(ESMReader &esm);
        void save(ESMWriter &esm) const;
    };

    struct AiTravel : AiPackage
    {
        AiTravelData mData;
        bool mHidden;

        void load(ESMReader &esm);
        void save(ESMWriter &esm) const;
    };

    struct AiEscort : AiPackage
    {
        AiEscortData mData;

        int mTargetActorId;
        std::string mTargetId;
        std::string mCellId;
        float mRemainingDuration;

        void load(ESMReader &esm);
        void save(ESMWriter &esm) const;
    };

    struct AiFollow : AiPackage
    {
        AiEscortData mData;

        int mTargetActorId;
        std::string mTargetId;
        std::string mCellId;
        float mRemainingDuration;

        bool mAlwaysFollow;
        bool mCommanded;

        bool mActive;

        void load(ESMReader &esm);
        void save(ESMWriter &esm) const;
    };

    struct AiActivate : AiPackage
    {
        std::string mTargetId;

        void load(ESMReader &esm);
        void save(ESMWriter &esm) const;
    };

    struct AiCombat : AiPackage
    {
        int mTargetActorId;

        void load(ESMReader &esm);
        void save(ESMWriter &esm) const;
    };

    struct AiPursue : AiPackage
    {
        int mTargetActorId;

        void load(ESMReader &esm);
        void save(ESMWriter &esm) const;
    };

    struct AiPackageContainer
    {
        int mType;

        AiPackage* mPackage;
    };

    struct AiSequence
    {
        AiSequence()
        {
            mLastAiPackage = -1;
        }
        ~AiSequence();

        std::vector<AiPackageContainer> mPackages;
        int mLastAiPackage;

        void load (ESMReader &esm);
        void save (ESMWriter &esm) const;

    private:
        AiSequence(const AiSequence&);
        AiSequence& operator=(const AiSequence&);
    };

    }

}

#endif
