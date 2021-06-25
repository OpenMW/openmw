#include "aisequence.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

#include <memory>

namespace ESM
{
namespace AiSequence
{

    void AiWander::load(ESMReader &esm)
    {
        esm.getHNT (mData, "DATA");
        esm.getHNT(mDurationData, "STAR"); // was mStartTime
        mStoredInitialActorPosition = false;
        if (esm.isNextSub("POS_"))
        {
            mStoredInitialActorPosition = true;
            esm.getHT(mInitialActorPosition);
        }
    }

    void AiWander::save(ESMWriter &esm) const
    {
        esm.writeHNT ("DATA", mData);
        esm.writeHNT ("STAR", mDurationData);
        if (mStoredInitialActorPosition)
            esm.writeHNT ("POS_", mInitialActorPosition);
    }

    void AiTravel::load(ESMReader &esm)
    {
        esm.getHNT (mData, "DATA");
        esm.getHNOT (mHidden, "HIDD");
    }

    void AiTravel::save(ESMWriter &esm) const
    {
        esm.writeHNT ("DATA", mData);
        esm.writeHNT ("HIDD", mHidden);
    }

    void AiEscort::load(ESMReader &esm)
    {
        esm.getHNT (mData, "DATA");
        mTargetId = esm.getHNString("TARG");
        mTargetActorId = -1;
        esm.getHNOT (mTargetActorId, "TAID");
        esm.getHNT (mRemainingDuration, "DURA");
        mCellId = esm.getHNOString ("CELL");
    }

    void AiEscort::save(ESMWriter &esm) const
    {
        esm.writeHNT ("DATA", mData);
        esm.writeHNString ("TARG", mTargetId);
        esm.writeHNT ("TAID", mTargetActorId);
        esm.writeHNT ("DURA", mRemainingDuration);
        if (!mCellId.empty())
            esm.writeHNString ("CELL", mCellId);
    }

    void AiFollow::load(ESMReader &esm)
    {
        esm.getHNT (mData, "DATA");
        mTargetId = esm.getHNString("TARG");
        mTargetActorId = -1;
        esm.getHNOT (mTargetActorId, "TAID");
        esm.getHNT (mRemainingDuration, "DURA");
        mCellId = esm.getHNOString ("CELL");
        esm.getHNT (mAlwaysFollow, "ALWY");
        mCommanded = false;
        esm.getHNOT (mCommanded, "CMND");
        mActive = false;
        esm.getHNOT (mActive, "ACTV");
    }

    void AiFollow::save(ESMWriter &esm) const
    {
        esm.writeHNT ("DATA", mData);
        esm.writeHNString("TARG", mTargetId);
        esm.writeHNT ("TAID", mTargetActorId);
        esm.writeHNT ("DURA", mRemainingDuration);
        if (!mCellId.empty())
            esm.writeHNString ("CELL", mCellId);
        esm.writeHNT ("ALWY", mAlwaysFollow);
        esm.writeHNT ("CMND", mCommanded);
        if (mActive)
            esm.writeHNT("ACTV", mActive);
    }

    void AiActivate::load(ESMReader &esm)
    {
        mTargetId = esm.getHNString("TARG");
    }

    void AiActivate::save(ESMWriter &esm) const
    {
        esm.writeHNString("TARG", mTargetId);
    }

    void AiCombat::load(ESMReader &esm)
    {
        esm.getHNT (mTargetActorId, "TARG");
    }

    void AiCombat::save(ESMWriter &esm) const
    {
        esm.writeHNT ("TARG", mTargetActorId);
    }

    void AiPursue::load(ESMReader &esm)
    {
        esm.getHNT (mTargetActorId, "TARG");
    }

    void AiPursue::save(ESMWriter &esm) const
    {
        esm.writeHNT ("TARG", mTargetActorId);
    }

    AiSequence::~AiSequence()
    {
        for (std::vector<AiPackageContainer>::iterator it = mPackages.begin(); it != mPackages.end(); ++it)
            delete it->mPackage;
    }

    void AiSequence::save(ESMWriter &esm) const
    {
        for (std::vector<AiPackageContainer>::const_iterator it = mPackages.begin(); it != mPackages.end(); ++it)
        {
            esm.writeHNT ("AIPK", it->mType);
            switch (it->mType)
            {
            case Ai_Wander:
                static_cast<const AiWander*>(it->mPackage)->save(esm);
                break;
            case Ai_Travel:
                static_cast<const AiTravel*>(it->mPackage)->save(esm);
                break;
            case Ai_Escort:
                static_cast<const AiEscort*>(it->mPackage)->save(esm);
                break;
            case Ai_Follow:
                static_cast<const AiFollow*>(it->mPackage)->save(esm);
                break;
            case Ai_Activate:
                static_cast<const AiActivate*>(it->mPackage)->save(esm);
                break;
            case Ai_Combat:
                static_cast<const AiCombat*>(it->mPackage)->save(esm);
                break;
            case Ai_Pursue:
                static_cast<const AiPursue*>(it->mPackage)->save(esm);
                break;

            default:
                break;
            }
        }

        esm.writeHNT ("LAST", mLastAiPackage);
    }

    void AiSequence::load(ESMReader &esm)
    {
        while (esm.isNextSub("AIPK"))
        {
            int type;
            esm.getHT(type);

            mPackages.emplace_back();
            mPackages.back().mType = type;

            switch (type)
            {
            case Ai_Wander:
            {
                std::unique_ptr<AiWander> ptr = std::make_unique<AiWander>();
                ptr->load(esm);
                mPackages.back().mPackage = ptr.release();
                break;
            }
            case Ai_Travel:
            {
                std::unique_ptr<AiTravel> ptr = std::make_unique<AiTravel>();
                ptr->load(esm);
                mPackages.back().mPackage = ptr.release();
                break;
            }
            case Ai_Escort:
            {
                std::unique_ptr<AiEscort> ptr = std::make_unique<AiEscort>();
                ptr->load(esm);
                mPackages.back().mPackage = ptr.release();
                break;
            }
            case Ai_Follow:
            {
                std::unique_ptr<AiFollow> ptr = std::make_unique<AiFollow>();
                ptr->load(esm);
                mPackages.back().mPackage = ptr.release();
                break;
            }
            case Ai_Activate:
            {
                std::unique_ptr<AiActivate> ptr = std::make_unique<AiActivate>();
                ptr->load(esm);
                mPackages.back().mPackage = ptr.release();
                break;
            }
            case Ai_Combat:
            {
                std::unique_ptr<AiCombat> ptr = std::make_unique<AiCombat>();
                ptr->load(esm);
                mPackages.back().mPackage = ptr.release();
                break;
            }
            case Ai_Pursue:
            {
                std::unique_ptr<AiPursue> ptr = std::make_unique<AiPursue>();
                ptr->load(esm);
                mPackages.back().mPackage = ptr.release();
                break;
            }
            default:
                return;
            }
        }

        esm.getHNOT (mLastAiPackage, "LAST");
    }
}
}
