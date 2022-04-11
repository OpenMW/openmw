#ifndef OPENMW_COMPONENTS_ESM_AISEQUENCE_H
#define OPENMW_COMPONENTS_ESM_AISEQUENCE_H

#include <vector>
#include <string>
#include <memory>

#include "components/esm/defs.hpp"

#include "components/esm/util.hpp"

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
        Ai_Wander = fourCC("WAND"),
        Ai_Travel = fourCC("TRAV"),
        Ai_Escort = fourCC("ESCO"),
        Ai_Follow = fourCC("FOLL"),
        Ai_Activate = fourCC("ACTI"),
        Ai_Combat = fourCC("COMB"),
        Ai_Pursue = fourCC("PURS")
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
        AiWanderDuration mDurationData; // was TimeStamp mStartTime

        bool mStoredInitialActorPosition;
        Vector3 mInitialActorPosition;

        /// \todo add more AiWander state

        void load(ESMReader &esm);
        void save(ESMWriter &esm) const;
    };

    struct AiTravel : AiPackage
    {
        AiTravelData mData;
        bool mHidden;
        bool mRepeat;

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
        bool mRepeat;

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
        bool mRepeat;

        void load(ESMReader &esm);
        void save(ESMWriter &esm) const;
    };

    struct AiActivate : AiPackage
    {
        std::string mTargetId;
        bool mRepeat;

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

        std::unique_ptr<AiPackage> mPackage;
    };

    struct AiSequence
    {
        AiSequence()
        {
            mLastAiPackage = -1;
        }

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
