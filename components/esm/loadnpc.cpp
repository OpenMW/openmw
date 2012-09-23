#include "loadnpc.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{

void NPC::load(ESMReader &esm)
{
    mNpdt52.mGold = -10;

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
        mNpdtType = 52;
        esm.getExact(&mNpdt52, 52);
    }
    else if (esm.getSubSize() == 12)
    {
        mNpdtType = 12;
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
    esm.skipRecord();
}
void NPC::save(ESMWriter &esm)
{
    esm.writeHNOCString("MODL", mModel);
    esm.writeHNOCString("FNAM", mName);
    esm.writeHNCString("RNAM", mRace);
    esm.writeHNCString("CNAM", mClass);
    esm.writeHNCString("ANAM", mFaction);
    esm.writeHNCString("BNAM", mHead);
    esm.writeHNCString("KNAM", mHair);
    esm.writeHNOCString("SCRI", mScript);
    
    if (mNpdtType == 52)
        esm.writeHNT("NPDT", mNpdt52, 52);
    else if (mNpdtType == 12)
        esm.writeHNT("NPDT", mNpdt12, 12);

    esm.writeHNT("FLAG", mFlags);
    
    mInventory.save(esm);
    mSpells.save(esm);
    if (mHasAI) {
        esm.writeHNT("AIDT", mAiData, sizeof(mAiData));
    }

    typedef std::vector<Dest>::iterator DestIter;
    for (DestIter it = mTransport.begin(); it != mTransport.end(); ++it) {
        esm.writeHNT("DODT", it->mPos, sizeof(it->mPos));
        esm.writeHNOCString("DNAM", it->mCellName);
    }
    mAiPackage.save(esm);
}

}
