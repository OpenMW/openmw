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
        mRecordFlags = esm.getRecordFlags();

        mSpells.mList.clear();
        mInventory.mList.clear();
        mTransport.mList.clear();
        mAiPackage.mList.clear();
        mAiData.blank();
        mAiData.mHello = mAiData.mFight = mAiData.mFlee = 30;

        bool hasName = false;
        bool hasNpdt = false;
        bool hasFlags = false;
        while (esm.hasMoreSubs())
        {
            esm.getSubName();
            switch (esm.retSubName().intval)
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
                        esm.getExact(&mNpdt, 52);
                    }
                    else if (esm.getSubSize() == 12)
                    {
                        //Reading into temporary NPDTstruct12 object
                        NPDTstruct12 npdt12;
                        mNpdtType = NPC_WITH_AUTOCALCULATED_STATS;
                        esm.getExact(&npdt12, 12);

                        //Clearing the mNdpt struct to initialize all values
                        blankNpdt();
                        //Swiching to an internal representation
                        mNpdt.mLevel = npdt12.mLevel;
                        mNpdt.mDisposition = npdt12.mDisposition;
                        mNpdt.mReputation = npdt12.mReputation;
                        mNpdt.mRank = npdt12.mRank;
                        mNpdt.mGold = npdt12.mGold;
                    }
                    else
                        esm.fail("NPC_NPDT must be 12 or 52 bytes long");
                    break;
                case ESM::FourCC<'F','L','A','G'>::value:
                    hasFlags = true;
                    int flags;
                    esm.getHT(flags);
                    mFlags = flags & 0xFF;
                    mBloodType = ((flags >> 8) & 0xFF) >> 2;
                    break;
                case ESM::FourCC<'N','P','C','S'>::value:
                    mSpells.add(esm);
                    break;
                case ESM::FourCC<'N','P','C','O'>::value:
                    mInventory.add(esm);
                    break;
                case ESM::FourCC<'A','I','D','T'>::value:
                    esm.getHExact(&mAiData, sizeof(mAiData));
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
            esm.writeHNString("DELE", "", 3);
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
        {
            esm.writeHNT("NPDT", mNpdt, 52);
        }
        else if (mNpdtType == NPC_WITH_AUTOCALCULATED_STATS)
        {
            NPDTstruct12 npdt12;
            npdt12.mLevel = mNpdt.mLevel;
            npdt12.mDisposition = mNpdt.mDisposition;
            npdt12.mReputation = mNpdt.mReputation;
            npdt12.mRank = mNpdt.mRank;
            npdt12.mGold = mNpdt.mGold;
            esm.writeHNT("NPDT", npdt12, 12);
        }

        esm.writeHNT("FLAG", ((mBloodType << 10) + mFlags));

        mInventory.save(esm);
        mSpells.save(esm);
        esm.writeHNT("AIDT", mAiData, sizeof(mAiData));

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
        blankNpdt();
        mBloodType = 0;
        mFlags = 0;
        mInventory.mList.clear();
        mSpells.mList.clear();
        mAiData.blank();
        mAiData.mHello = mAiData.mFight = mAiData.mFlee = 30;
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

    void NPC::blankNpdt()
    {
        mNpdt.mLevel = 0;
        mNpdt.mStrength = mNpdt.mIntelligence = mNpdt.mWillpower = mNpdt.mAgility =
            mNpdt.mSpeed = mNpdt.mEndurance = mNpdt.mPersonality = mNpdt.mLuck = 0;
        for (int i=0; i< Skill::Length; ++i) mNpdt.mSkills[i] = 0;
        mNpdt.mReputation = 0;
        mNpdt.mHealth = mNpdt.mMana = mNpdt.mFatigue = 0;
        mNpdt.mDisposition = 0;
        mNpdt.mUnknown1 = 0;
        mNpdt.mRank = 0;
        mNpdt.mUnknown2 = 0;
        mNpdt.mGold = 0;
    }

    int NPC::getFactionRank() const
    {
        if (mFaction.empty())
            return -1;
        else
            return mNpdt.mRank;
    }

    const std::vector<Transport::Dest>& NPC::getTransport() const
    {
        return mTransport.mList;
    }
}
