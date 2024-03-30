#ifndef OPENMW_COMPONENTS_ESM_AISEQUENCE_H
#define OPENMW_COMPONENTS_ESM_AISEQUENCE_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "components/esm/refid.hpp"
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

        struct AiWanderData
        {
            int16_t mDistance;
            int16_t mDuration;
            std::uint8_t mTimeOfDay;
            std::uint8_t mIdle[8];
            std::uint8_t mShouldRepeat;
        };

        struct AiWanderDuration
        {
            float mRemainingDuration;
        };

        struct AiTravelData
        {
            float mX, mY, mZ;
        };

        struct AiEscortData
        {
            float mX, mY, mZ;
            int16_t mDuration;
        };

        struct AiWander : AiPackage
        {
            AiWanderData mData;
            AiWanderDuration mDurationData; // was TimeStamp mStartTime

            bool mStoredInitialActorPosition;
            Vector3 mInitialActorPosition;

            /// \todo add more AiWander state

            void load(ESMReader& esm);
            void save(ESMWriter& esm) const;
        };

        struct AiTravel : AiPackage
        {
            AiTravelData mData;
            bool mHidden;
            bool mRepeat;

            void load(ESMReader& esm);
            void save(ESMWriter& esm) const;
        };

        struct AiEscort : AiPackage
        {
            AiEscortData mData;

            int32_t mTargetActorId;
            ESM::RefId mTargetId;
            std::string mCellId;
            float mRemainingDuration;
            bool mRepeat;

            void load(ESMReader& esm);
            void save(ESMWriter& esm) const;
        };

        struct AiFollow : AiPackage
        {
            AiEscortData mData;

            int32_t mTargetActorId;
            ESM::RefId mTargetId;
            std::string mCellId;
            float mRemainingDuration;

            bool mAlwaysFollow;
            bool mCommanded;

            bool mActive;
            bool mRepeat;

            void load(ESMReader& esm);
            void save(ESMWriter& esm) const;
        };

        struct AiActivate : AiPackage
        {
            ESM::RefId mTargetId;
            bool mRepeat;

            void load(ESMReader& esm);
            void save(ESMWriter& esm) const;
        };

        struct AiCombat : AiPackage
        {
            int32_t mTargetActorId;

            void load(ESMReader& esm);
            void save(ESMWriter& esm) const;
        };

        struct AiPursue : AiPackage
        {
            int32_t mTargetActorId;

            void load(ESMReader& esm);
            void save(ESMWriter& esm) const;
        };

        struct AiPackageContainer
        {
            int32_t mType;

            std::unique_ptr<AiPackage> mPackage;
        };

        struct AiSequence
        {
            AiSequence() { mLastAiPackage = -1; }

            std::vector<AiPackageContainer> mPackages;
            int32_t mLastAiPackage;

            void load(ESMReader& esm);
            void save(ESMWriter& esm) const;

        private:
            AiSequence(const AiSequence&);
            AiSequence& operator=(const AiSequence&);
        };

    }

}

#endif
