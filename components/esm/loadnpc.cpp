#include "loadnpc.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int NPC::sRecordId = REC_NPC_;

void NPC::load(ESMReader &esm)
{
    mPersistent = esm.getRecordFlags() & 0x0400;

    mModel = esm.getHNOString("MODL");
    mName = esm.getHNOString("FNAM");

    mRace = esm.getHNString("RNAM");
    mClass = esm.getHNString("CNAM");
    mFaction = esm.getHNString("ANAM");
    mHead = esm.getHNString("BNAM");
    mHair = esm.getHNString("KNAM");

    mScript = esm.getHNOString("SCRI");

    esm.getSubNameIs("NPDT");
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

    esm.getHNT(mFlags, "FLAG");

    mInventory.load(esm);
    mSpells.load(esm);

    if (esm.isNextSub("AIDT"))
    {
        esm.getHExact(&mAiData, sizeof(mAiData));
        mHasAI= true;
    }
    else
        mHasAI = false;

    while (esm.isNextSub("DODT") || esm.isNextSub("DNAM")) {
        if (esm.retSubName() == 0x54444f44) { // DODT struct
            Dest dodt;
            esm.getHExact(&dodt.mPos, 24);
            mTransport.push_back(dodt);
        } else if (esm.retSubName() == 0x4d414e44) { // DNAM struct
            mTransport.back().mCellName = esm.getHString();
        }
    }
    mAiPackage.load(esm);
}
void NPC::save(ESMWriter &esm) const
{
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
    if (mHasAI) {
        esm.writeHNT("AIDT", mAiData, sizeof(mAiData));
    }

    typedef std::vector<Dest>::const_iterator DestIter;
    for (DestIter it = mTransport.begin(); it != mTransport.end(); ++it) {
        esm.writeHNT("DODT", it->mPos, sizeof(it->mPos));
        esm.writeHNOCString("DNAM", it->mCellName);
    }
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
        mNpdtType = 0;
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
        mTransport.clear();
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
}
