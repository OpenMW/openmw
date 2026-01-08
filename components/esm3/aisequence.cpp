#include "aisequence.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

#include <components/misc/concepts.hpp>

#include <algorithm>
#include <memory>

namespace ESM
{
    template <Misc::SameAsWithoutCvref<AiSequence::AiWanderData> T>
    void decompose(T&& v, const auto& f)
    {
        f(v.mDistance, v.mDuration, v.mTimeOfDay, v.mIdle, v.mShouldRepeat);
    }

    template <Misc::SameAsWithoutCvref<AiSequence::AiWanderDuration> T>
    void decompose(T&& v, const auto& f)
    {
        std::uint32_t unused = 0;
        f(v.mRemainingDuration, unused);
    }

    template <Misc::SameAsWithoutCvref<AiSequence::AiTravelData> T>
    void decompose(T&& v, const auto& f)
    {
        f(v.mX, v.mY, v.mZ);
    }

    template <Misc::SameAsWithoutCvref<AiSequence::AiEscortData> T>
    void decompose(T&& v, const auto& f)
    {
        f(v.mX, v.mY, v.mZ, v.mDuration);
    }

    namespace
    {
        void loadActorId(ESMReader& esm, ESM::NAME name, RefNum& refNum)
        {
            if (esm.getFormatVersion() <= MaxActorIdSaveGameFormatVersion)
            {
                refNum.mIndex = static_cast<uint32_t>(-1);
                esm.getHNOT(refNum.mIndex, name);
            }
            else if (esm.peekNextSub(name))
                refNum = esm.getFormId(true, name);
            else
                refNum = {};
        }
    }

    namespace AiSequence
    {
        void AiWander::load(ESMReader& esm)
        {
            esm.getNamedComposite("DATA", mData);
            esm.getNamedComposite("STAR", mDurationData); // was mStartTime
            mStoredInitialActorPosition = esm.getHNOT("POS_", mInitialActorPosition.mValues);
        }

        void AiWander::save(ESMWriter& esm) const
        {
            esm.writeNamedComposite("DATA", mData);
            esm.writeNamedComposite("STAR", mDurationData); // was mStartTime
            if (mStoredInitialActorPosition)
                esm.writeHNT("POS_", mInitialActorPosition.mValues);
        }

        void AiTravel::load(ESMReader& esm)
        {
            esm.getNamedComposite("DATA", mData);
            esm.getHNT(mHidden, "HIDD");
            mRepeat = false;
            esm.getHNOT(mRepeat, "REPT");
        }

        void AiTravel::save(ESMWriter& esm) const
        {
            esm.writeNamedComposite("DATA", mData);
            esm.writeHNT("HIDD", mHidden);
            if (mRepeat)
                esm.writeHNT("REPT", mRepeat);
        }

        void AiEscort::load(ESMReader& esm)
        {
            esm.getNamedComposite("DATA", mData);
            mTargetId = esm.getHNRefId("TARG");
            loadActorId(esm, "TAID", mTargetActor);
            esm.getHNT(mRemainingDuration, "DURA");
            mCellId = esm.getHNOString("CELL");
            mRepeat = false;
            esm.getHNOT(mRepeat, "REPT");
            if (esm.getFormatVersion() <= MaxOldAiPackageFormatVersion)
            {
                // mDuration isn't saved in the save file, so just giving it "1" for now if the package has a duration.
                // The exact value of mDuration only matters for repeating packages.
                // Previously mRemainingDuration could be negative even when mDuration was 0. Checking for > 0 should
                // fix old saves.
                mData.mDuration = static_cast<int16_t>(std::max<float>(mRemainingDuration > 0, mRemainingDuration));
            }
        }

        void AiEscort::save(ESMWriter& esm) const
        {
            esm.writeNamedComposite("DATA", mData);
            esm.writeHNRefId("TARG", mTargetId);
            if (esm.getFormatVersion() > MaxActorIdSaveGameFormatVersion)
                esm.writeFormId(mTargetActor, true, "TAID");
            esm.writeHNT("DURA", mRemainingDuration);
            if (!mCellId.empty())
                esm.writeHNString("CELL", mCellId);
            if (mRepeat)
                esm.writeHNT("REPT", mRepeat);
        }

        void AiFollow::load(ESMReader& esm)
        {
            esm.getNamedComposite("DATA", mData);
            mTargetId = esm.getHNRefId("TARG");
            loadActorId(esm, "TAID", mTargetActor);
            esm.getHNT(mRemainingDuration, "DURA");
            mCellId = esm.getHNOString("CELL");
            esm.getHNT(mAlwaysFollow, "ALWY");
            mCommanded = false;
            esm.getHNOT(mCommanded, "CMND");
            mActive = false;
            esm.getHNOT(mActive, "ACTV");
            mRepeat = false;
            esm.getHNOT(mRepeat, "REPT");
            if (esm.getFormatVersion() <= MaxOldAiPackageFormatVersion)
            {
                // mDuration isn't saved in the save file, so just giving it "1" for now if the package has a duration.
                // The exact value of mDuration only matters for repeating packages.
                // Previously mRemainingDuration could be negative even when mDuration was 0. Checking for > 0 should
                // fix old saves.
                mData.mDuration = static_cast<int16_t>(std::max<float>(mRemainingDuration > 0, mRemainingDuration));
            }
        }

        void AiFollow::save(ESMWriter& esm) const
        {
            esm.writeNamedComposite("DATA", mData);
            esm.writeHNRefId("TARG", mTargetId);
            if (esm.getFormatVersion() > MaxActorIdSaveGameFormatVersion)
                esm.writeFormId(mTargetActor, true, "TAID");
            esm.writeHNT("DURA", mRemainingDuration);
            if (!mCellId.empty())
                esm.writeHNString("CELL", mCellId);
            esm.writeHNT("ALWY", mAlwaysFollow);
            esm.writeHNT("CMND", mCommanded);
            if (mActive)
                esm.writeHNT("ACTV", mActive);
            if (mRepeat)
                esm.writeHNT("REPT", mRepeat);
        }

        void AiActivate::load(ESMReader& esm)
        {
            mTargetId = esm.getHNRefId("TARG");
            mRepeat = false;
            esm.getHNOT(mRepeat, "REPT");
        }

        void AiActivate::save(ESMWriter& esm) const
        {
            esm.writeHNRefId("TARG", mTargetId);
            if (mRepeat)
                esm.writeHNT("REPT", mRepeat);
        }

        void AiCombat::load(ESMReader& esm)
        {
            loadActorId(esm, "TARG", mTargetActor);
        }

        void AiCombat::save(ESMWriter& esm) const
        {
            esm.writeFormId(mTargetActor, true, "TARG");
        }

        void AiPursue::load(ESMReader& esm)
        {
            loadActorId(esm, "TARG", mTargetActor);
        }

        void AiPursue::save(ESMWriter& esm) const
        {
            esm.writeFormId(mTargetActor, true, "TARG");
        }

        void AiSequence::save(ESMWriter& esm) const
        {
            for (std::vector<AiPackageContainer>::const_iterator it = mPackages.begin(); it != mPackages.end(); ++it)
            {
                esm.writeHNT("AIPK", it->mType);
                switch (it->mType)
                {
                    case Ai_Wander:
                        static_cast<const AiWander&>(*it->mPackage).save(esm);
                        break;
                    case Ai_Travel:
                        static_cast<const AiTravel&>(*it->mPackage).save(esm);
                        break;
                    case Ai_Escort:
                        static_cast<const AiEscort&>(*it->mPackage).save(esm);
                        break;
                    case Ai_Follow:
                        static_cast<const AiFollow&>(*it->mPackage).save(esm);
                        break;
                    case Ai_Activate:
                        static_cast<const AiActivate&>(*it->mPackage).save(esm);
                        break;
                    case Ai_Combat:
                        static_cast<const AiCombat&>(*it->mPackage).save(esm);
                        break;
                    case Ai_Pursue:
                        static_cast<const AiPursue&>(*it->mPackage).save(esm);
                        break;

                    default:
                        break;
                }
            }

            esm.writeHNT("LAST", mLastAiPackage);
        }

        void AiSequence::load(ESMReader& esm)
        {
            mActorIdConverter = esm.getActorIdConverter();
            int count = 0;
            while (esm.isNextSub("AIPK"))
            {
                int32_t type;
                esm.getHT(type);

                mPackages.emplace_back();
                mPackages.back().mType = type;

                switch (type)
                {
                    case Ai_Wander:
                    {
                        std::unique_ptr<AiWander> ptr = std::make_unique<AiWander>();
                        ptr->load(esm);
                        mPackages.back().mPackage = std::move(ptr);
                        ++count;
                        break;
                    }
                    case Ai_Travel:
                    {
                        std::unique_ptr<AiTravel> ptr = std::make_unique<AiTravel>();
                        ptr->load(esm);
                        mPackages.back().mPackage = std::move(ptr);
                        ++count;
                        break;
                    }
                    case Ai_Escort:
                    {
                        std::unique_ptr<AiEscort> ptr = std::make_unique<AiEscort>();
                        ptr->load(esm);
                        mPackages.back().mPackage = std::move(ptr);
                        ++count;
                        break;
                    }
                    case Ai_Follow:
                    {
                        std::unique_ptr<AiFollow> ptr = std::make_unique<AiFollow>();
                        ptr->load(esm);
                        mPackages.back().mPackage = std::move(ptr);
                        ++count;
                        break;
                    }
                    case Ai_Activate:
                    {
                        std::unique_ptr<AiActivate> ptr = std::make_unique<AiActivate>();
                        ptr->load(esm);
                        mPackages.back().mPackage = std::move(ptr);
                        ++count;
                        break;
                    }
                    case Ai_Combat:
                    {
                        std::unique_ptr<AiCombat> ptr = std::make_unique<AiCombat>();
                        ptr->load(esm);
                        mPackages.back().mPackage = std::move(ptr);
                        break;
                    }
                    case Ai_Pursue:
                    {
                        std::unique_ptr<AiPursue> ptr = std::make_unique<AiPursue>();
                        ptr->load(esm);
                        mPackages.back().mPackage = std::move(ptr);
                        break;
                    }
                    default:
                        return;
                }
            }

            esm.getHNT(mLastAiPackage, "LAST");

            if (count > 1 && esm.getFormatVersion() <= MaxOldAiPackageFormatVersion)
            {
                for (auto& pkg : mPackages)
                {
                    if (pkg.mType == Ai_Wander)
                        static_cast<AiWander&>(*pkg.mPackage).mData.mShouldRepeat = true;
                    else if (pkg.mType == Ai_Travel)
                        static_cast<AiTravel&>(*pkg.mPackage).mRepeat = true;
                    else if (pkg.mType == Ai_Escort)
                        static_cast<AiEscort&>(*pkg.mPackage).mRepeat = true;
                    else if (pkg.mType == Ai_Follow)
                        static_cast<AiFollow&>(*pkg.mPackage).mRepeat = true;
                    else if (pkg.mType == Ai_Activate)
                        static_cast<AiActivate&>(*pkg.mPackage).mRepeat = true;
                }
            }
        }
    }
}
