#include "loadnpc.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int NPC::sRecordId = REC_NPC_;

    void NPC::load(ESMReader &esm, bool &isDeleted)
    {
        isDeleted = false;

        mPersistent = (esm.getRecordFlags() & 0x0400) != 0;

        mSpells.mList.clear();
        mInventory.mList.clear();
        mTransport.mList.clear();
        mAiPackage.mList.clear();
        mHasAI = false;

        bool hasName = false;
        bool hasNpdt = false;
        bool hasFlags = false;
        while (esm.hasMoreSubs())
        {
            esm.getSubName();
            switch (esm.retSubName().val)
            {
                case ESM::SREC_NAME:
                    mId = esm.getHString();
                    hasName = true;
                    break;
                case ESM::FourCC<'M','O','D','L'>::value:
                    mModel = esm.getHString();
                    break;
                case ESM::FourCC<'F','N','A','M'>::value:
                    mName = esm.getHString();
                    break;
                case ESM::FourCC<'R','N','A','M'>::value:
                    mRace = esm.getHString();
                    break;
                case ESM::FourCC<'C','N','A','M'>::value:
                    mClass = esm.getHString();
                    break;
                case ESM::FourCC<'A','N','A','M'>::value:
                    mFaction = esm.getHString();
                    break;
                case ESM::FourCC<'B','N','A','M'>::value:
                    mHead = esm.getHString();
                    break;
                case ESM::FourCC<'K','N','A','M'>::value:
                    mHair = esm.getHString();
                    break;
                case ESM::FourCC<'S','C','R','I'>::value:
                    mScript = esm.getHString();
                    break;
                case ESM::FourCC<'N','P','D','T'>::value:
                    hasNpdt = true;
                    esm.getSubHeader();
                    if (esm.getSubSize() == 52)
                    {
                        mNpdtType = NPC_DEFAULT;
                        esm.getExact(&mNpdt52, 52);
                    }
                    else if (esm.getSubSize() == 12)
                    {
                        mNpdtType = NPC_WITH_AUTOCALCULATED_STATS;
                        esm.getExact(&mNpdt12, 12);
                    }
                    else
                        esm.fail("NPC_NPDT must be 12 or 52 bytes long");
                    break;
                case ESM::FourCC<'F','L','A','G'>::value:
                    hasFlags = true;
                    esm.getHT(mFlags);
                    break;
                case ESM::FourCC<'N','P','C','S'>::value:
                    mSpells.add(esm);
                    break;
                case ESM::FourCC<'N','P','C','O'>::value:
                    mInventory.add(esm);
                    break;
                case ESM::FourCC<'A','I','D','T'>::value:
                    esm.getHExact(&mAiData, sizeof(mAiData));
                    mHasAI= true;
                    break;
                case ESM::FourCC<'D','O','D','T'>::value:
                case ESM::FourCC<'D','N','A','M'>::value:
                    mTransport.add(esm);
                    break;
                case AI_Wander:
                case AI_Activate:
                case AI_Escort:
                case AI_Follow:
                case AI_Travel:
                case AI_CNDT:
                    mAiPackage.add(esm);
                    break;
                case ESM::SREC_DELE:
                    esm.skipHSub();
                    isDeleted = true;
                    break;
                default:
                    esm.fail("Unknown subrecord");
                    break;
            }
        }

        if (!hasName)
            esm.fail("Missing NAME subrecord");
        if (!hasNpdt && !isDeleted)
            esm.fail("Missing NPDT subrecord");
        if (!hasFlags && !isDeleted)
            esm.fail("Missing FLAG subrecord");
    }
    void NPC::save(ESMWriter &esm, bool isDeleted) const
    {
        esm.writeHNCString("NAME", mId);

        if (isDeleted)
        {
            esm.writeHNCString("DELE", "");
            return;
        }

        esm.writeHNOCString("MODL", mModel);
        esm.writeHNOCString("FNAM", mName);
        esm.writeHNCString("RNAM", mRace);
        esm.writeHNCString("CNAM", mClass);
        esm.writeHNCString("ANAM", mFaction);
        esm.writeHNCString("BNAM", mHead);
        esm.writeHNCString("KNAM", mHair);
        esm.writeHNOCString("SCRI", mScript);

        if (mNpdtType == NPC_DEFAULT)
            esm.writeHNT("NPDT", mNpdt52, 52);
        else if (mNpdtType == NPC_WITH_AUTOCALCULATED_STATS)
            esm.writeHNT("NPDT", mNpdt12, 12);

        esm.writeHNT("FLAG", mFlags);

        mInventory.save(esm);
        mSpells.save(esm);
        if (mAiData.mHello != 0
            || mAiData.mFight != 0
            || mAiData.mFlee != 0
            || mAiData.mAlarm != 0
            || mAiData.mServices != 0)
        {
            esm.writeHNT("AIDT", mAiData, sizeof(mAiData));
        }

        mTransport.save(esm);

        mAiPackage.save(esm);
    }

    bool NPC::isMale() const {
        return (mFlags & Female) == 0;
    }

    void NPC::setIsMale(bool value) {
        mFlags |= Female;
        if (value) {
            mFlags ^= Female;
        }
    }

    void NPC::blank()
    {
        mNpdtType = NPC_DEFAULT;
        mNpdt52.mLevel = 0;
        mNpdt52.mStrength = mNpdt52.mIntelligence = mNpdt52.mWillpower = mNpdt52.mAgility =
            mNpdt52.mSpeed = mNpdt52.mEndurance = mNpdt52.mPersonality = mNpdt52.mLuck = 0;
        for (int i=0; i< Skill::Length; ++i) mNpdt52.mSkills[i] = 0;
        mNpdt52.mReputation = 0;
        mNpdt52.mHealth = mNpdt52.mMana = mNpdt52.mFatigue = 0;
        mNpdt52.mDisposition = 0;
        mNpdt52.mFactionID = 0;
        mNpdt52.mRank = 0;
        mNpdt52.mUnknown = 0;
        mNpdt52.mGold = 0;
        mNpdt12.mLevel = 0;
        mNpdt12.mDisposition = 0;
        mNpdt12.mReputation = 0;
        mNpdt12.mRank = 0;
        mNpdt12.mUnknown1 = 0;
        mNpdt12.mUnknown2 = 0;
        mNpdt12.mUnknown3 = 0;
        mNpdt12.mGold = 0;
        mFlags = 0;
        mInventory.mList.clear();
        mSpells.mList.clear();
        mAiData.blank();
        mHasAI = false;
        mTransport.mList.clear();
        mAiPackage.mList.clear();
        mName.clear();
        mModel.clear();
        mRace.clear();
        mClass.clear();
        mFaction.clear();
        mScript.clear();
        mHair.clear();
        mHead.clear();
    }

    int NPC::getFactionRank() const
    {
        if (mFaction.empty())
            return -1;
        else if (mNpdtType == ESM::NPC::NPC_WITH_AUTOCALCULATED_STATS)
            return mNpdt12.mRank;
        else // NPC_DEFAULT
            return mNpdt52.mRank;
    }

    const std::vector<Transport::Dest>& NPC::getTransport() const
    {
        return mTransport.mList;
    }
}
